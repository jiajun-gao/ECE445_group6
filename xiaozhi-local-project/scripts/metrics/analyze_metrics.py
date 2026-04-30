#!/usr/bin/env python3
"""Jarvis robot — server log → performance metrics + plots.

Reads a `xiaozhi-esp32-server` log file (either the file-rotated
``tmp/server.log`` or a captured terminal session) and extracts the
voice-pipeline metrics that the server already prints on every turn:

  * ASR final-text arrival      — ``ASR arrived chars=NN text=...``
  * Gemini TTFT (time-to-first-token) — ``Gemini TTFT NNNms model=...``
  * MCP tool execution time     — ``工具完成 tool=robot_move elapsed_ms=YYY action=...``
  * MCP tool timeouts           — ``工具超时 tool=...``
  * End-to-end ASR→first-TTS    — ``发送第一段语音: ... | e2e_asr_to_tts_ms=NNN``
  * Wake greetings              — ``Playing wake greeting: ...``
  * Aborts / barge-ins          — ``Abort message received``
  * Echo-guard drops            — ``忽略疑似回声短句 (echo guard)``

It then computes summary statistics (count / mean / p50 / p90 / p95 / max),
writes them out as CSV + JSON, and renders a small set of plots:

  1.  TTFT histogram + CDF                     ``ttft_distribution.png``
  2.  End-to-end latency histogram + CDF       ``e2e_distribution.png``
  3.  Per-tool elapsed-time bar chart          ``mcp_tool_latency.png``
  4.  Tool-call frequency bar chart            ``mcp_tool_frequency.png``
  5.  Latency-vs-time scatter (timeline)       ``latency_timeline.png``
  6.  Success-rate stacked bar                 ``success_rate.png``

Usage::

    python scripts/metrics/analyze_metrics.py \\
        --log server/xiaozhi-esp32-server/main/xiaozhi-server/tmp/server.log \\
        --out docs/metrics_report

The output directory will contain the PNGs, a ``metrics.csv`` (one row per
turn), a ``summary.json`` (the aggregate stats), and a ``REPORT.md`` that
embeds the plots — drop it into a slide deck as-is.

This script has only one external dependency beyond the stdlib:
``matplotlib`` (already installed in the server's .venv).  Numpy is used if
available, but we fall back to pure-Python percentile math when it isn't.
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import os
import re
import sys
from dataclasses import dataclass, field, asdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Tuple

# --- Optional numpy (only used for histogram bin selection) -----------------
try:
    import numpy as _np  # type: ignore
    _HAS_NUMPY = True
except Exception:  # pragma: no cover
    _np = None
    _HAS_NUMPY = False

import matplotlib

matplotlib.use("Agg")  # we never display interactively from this script
import matplotlib.pyplot as plt  # noqa: E402


# ---------------------------------------------------------------------------
# Log-line parsing
# ---------------------------------------------------------------------------

# The server emits two log formats. Both end the line with the same tag and
# message, so we pull a (timestamp, tag, level, message) tuple out and let
# the per-pattern matchers handle the message body.

# Console format from the IDE terminal capture / stdout::
#   260424 15:09:57[0.9.2_SiShGeEdnocaCh][plugins_func.functions.play_music]-INFO-播放完成
_RE_CONSOLE = re.compile(
    r"^(?P<ts>\d{6}\s+\d{2}:\d{2}:\d{2})"
    r"\[(?P<ver>[^_]+)_(?P<mod>[^\]]+)\]"
    r"\[(?P<tag>[^\]]+)\]-(?P<level>[A-Z]+)-(?P<msg>.*)$"
)

# File-rotated format from tmp/server.log::
#   2026-04-02 21:28:59 - 0.9.2_00000000000000 - core.utils.gc_manager - INFO - core.utils.gc_manager - 启动全局GC管理器，间隔300秒
_RE_FILE = re.compile(
    r"^(?P<ts>\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2})"
    r"\s+-\s+(?P<ver>[^_]+)_(?P<mod>\S+)"
    r"\s+-\s+(?P<name>\S+)"
    r"\s+-\s+(?P<level>[A-Z]+)"
    r"\s+-\s+(?P<tag>\S+)"
    r"\s+-\s+(?P<msg>.*)$"
)


def _parse_console_ts(s: str) -> Optional[datetime]:
    try:
        return datetime.strptime(s, "%y%m%d %H:%M:%S")
    except ValueError:
        return None


def _parse_file_ts(s: str) -> Optional[datetime]:
    try:
        return datetime.strptime(s, "%Y-%m-%d %H:%M:%S")
    except ValueError:
        return None


@dataclass
class LogLine:
    ts: datetime
    tag: str
    level: str
    msg: str


def parse_log_line(line: str) -> Optional[LogLine]:
    """Return a LogLine parsed from either console or file format, or None."""
    line = line.rstrip()
    m = _RE_CONSOLE.match(line)
    if m:
        ts = _parse_console_ts(m.group("ts"))
        if ts is None:
            return None
        return LogLine(ts=ts, tag=m.group("tag"), level=m.group("level"),
                       msg=m.group("msg"))
    m = _RE_FILE.match(line)
    if m:
        ts = _parse_file_ts(m.group("ts"))
        if ts is None:
            return None
        return LogLine(ts=ts, tag=m.group("tag"), level=m.group("level"),
                       msg=m.group("msg"))
    return None


# ---------------------------------------------------------------------------
# Per-event extractors
# ---------------------------------------------------------------------------

_RE_ASR = re.compile(
    r"^ASR arrived chars=(?P<chars>\d+) text=(?P<text>.*)$"
)
_RE_TTFT = re.compile(
    r"^Gemini TTFT (?P<ms>\d+(?:\.\d+)?)ms\s+model=(?P<model>\S+)\s+"
    r"think_budget=(?P<think>\S+)\s+tools=(?P<tools>\S+)$"
)
_RE_TOOL_DONE = re.compile(
    r"^工具完成 tool=(?P<tool>\S+) elapsed_ms=(?P<ms>\d+(?:\.\d+)?)\s+"
    r"action=(?P<action>\S+)\s+result_size=(?P<size>\d+)$"
)
_RE_TOOL_TIMEOUT = re.compile(
    r"^工具超时 tool=(?P<tool>\S+) elapsed_ms=(?P<ms>\d+(?:\.\d+)?)\s+"
    r"timeout=(?P<to>\S+)$"
)
_RE_TTS_FIRST = re.compile(
    r"^发送第一段语音:\s*(?P<text>.*?)(?:\s*\|\s*e2e_asr_to_tts_ms=(?P<ms>\d+(?:\.\d+)?))?\s*$"
)
_RE_WAKE_GREETING = re.compile(
    r"^Playing wake greeting:\s*(?P<text>.*)$"
)
_RE_ABORT = re.compile(r"^Abort message received(?:-end)?$")
_RE_ECHO_DROP = re.compile(r"^忽略疑似回声短句")


# ---------------------------------------------------------------------------
# Aggregated record types
# ---------------------------------------------------------------------------


@dataclass
class Turn:
    """One voice turn: ASR commit -> TTS first chunk on the wire.

    Tools fired during the turn are linked back here by chronological
    proximity (the server log does not propagate sentence_id consistently
    across all events), which is good enough for a desk-robot session of
    a few hundred turns."""
    idx: int
    asr_ts: datetime
    asr_text: str
    ttft_ms: Optional[float] = None
    e2e_ms: Optional[float] = None
    tools: List[Dict[str, Any]] = field(default_factory=list)
    tts_text: Optional[str] = None
    aborted: bool = False


@dataclass
class ToolCall:
    ts: datetime
    name: str
    elapsed_ms: float
    action: str
    result_size: int
    timed_out: bool = False


@dataclass
class ParsedSession:
    turns: List[Turn] = field(default_factory=list)
    tool_calls: List[ToolCall] = field(default_factory=list)
    wake_greetings: int = 0
    aborts: int = 0
    echo_drops: int = 0
    log_start: Optional[datetime] = None
    log_end: Optional[datetime] = None


# ---------------------------------------------------------------------------
# Parsing pipeline
# ---------------------------------------------------------------------------


# Marker that prefixes the very first log line of every fresh server boot
# (`./.venv/bin/python app.py`). It's emitted by core.utils.gc_manager and is
# unique enough that locating its last occurrence in the file gives us a
# clean "start of the most recent session" boundary.
_SESSION_START_MARKER = "启动全局GC管理器"


def find_last_session_start(path: Path) -> Optional[int]:
    """Return the byte offset of the last line that contains the session
    start marker, or None if the marker isn't present in this log."""
    last_offset: Optional[int] = None
    with path.open("rb") as f:
        offset = 0
        for raw in f:
            if _SESSION_START_MARKER.encode("utf-8") in raw:
                last_offset = offset
            offset += len(raw)
    return last_offset


def parse_log(path: Path,
              start_offset: int = 0,
              since: Optional[datetime] = None) -> ParsedSession:
    """Stream the log line-by-line and build a ParsedSession.

    Strategy: keep a "current open turn" that begins on every ASR-arrived
    line and ends on the next ASR-arrived line OR on a TTS-first-chunk
    that carries an e2e_ms (we treat that as the canonical end of the
    turn). Tools / TTFT lines that land inside the open window are
    attached to the current turn.

    Args:
        path: log file path
        start_offset: byte offset to seek to before reading. Use this to
            scope to "the latest session" by passing the offset returned
            by ``find_last_session_start``.
        since: only keep events at or after this timestamp. Applied AFTER
            the byte-offset seek; either or both filters can be used.
    """
    session = ParsedSession()
    open_turn: Optional[Turn] = None
    turn_idx = 0

    with path.open("r", encoding="utf-8", errors="replace") as f:
        if start_offset:
            f.seek(start_offset)
        for raw in f:
            ev = parse_log_line(raw)
            if ev is None:
                continue
            if since is not None and ev.ts < since:
                continue
            if session.log_start is None:
                session.log_start = ev.ts
            session.log_end = ev.ts

            msg = ev.msg

            m = _RE_ASR.match(msg)
            if m:
                # New turn starts. Close any previously-open turn.
                if open_turn is not None:
                    session.turns.append(open_turn)
                turn_idx += 1
                open_turn = Turn(
                    idx=turn_idx,
                    asr_ts=ev.ts,
                    asr_text=m.group("text"),
                )
                continue

            m = _RE_TTFT.match(msg)
            if m and open_turn is not None and open_turn.ttft_ms is None:
                open_turn.ttft_ms = float(m.group("ms"))
                continue

            m = _RE_TOOL_DONE.match(msg)
            if m:
                tc = ToolCall(
                    ts=ev.ts,
                    name=m.group("tool"),
                    elapsed_ms=float(m.group("ms")),
                    action=m.group("action"),
                    result_size=int(m.group("size")),
                )
                session.tool_calls.append(tc)
                if open_turn is not None:
                    open_turn.tools.append({
                        "name": tc.name,
                        "elapsed_ms": tc.elapsed_ms,
                        "action": tc.action,
                        "timed_out": False,
                    })
                continue

            m = _RE_TOOL_TIMEOUT.match(msg)
            if m:
                tc = ToolCall(
                    ts=ev.ts,
                    name=m.group("tool"),
                    elapsed_ms=float(m.group("ms")),
                    action="TIMEOUT",
                    result_size=0,
                    timed_out=True,
                )
                session.tool_calls.append(tc)
                if open_turn is not None:
                    open_turn.tools.append({
                        "name": tc.name,
                        "elapsed_ms": tc.elapsed_ms,
                        "action": "TIMEOUT",
                        "timed_out": True,
                    })
                continue

            m = _RE_TTS_FIRST.match(msg)
            if m and open_turn is not None:
                open_turn.tts_text = (m.group("text") or "").strip() or None
                e2e = m.group("ms")
                if e2e is not None:
                    open_turn.e2e_ms = float(e2e)
                # TTS-first-chunk closes the turn; subsequent tool calls or
                # follow-up sentences belong to the *same* turn though, so
                # we keep open_turn alive until a new ASR arrives or EOF.
                continue

            if _RE_WAKE_GREETING.match(msg):
                session.wake_greetings += 1
                continue
            if _RE_ABORT.match(msg):
                if open_turn is not None:
                    open_turn.aborted = True
                session.aborts += 1
                continue
            if _RE_ECHO_DROP.match(msg):
                session.echo_drops += 1
                continue

    if open_turn is not None:
        session.turns.append(open_turn)

    return session


# ---------------------------------------------------------------------------
# Statistics helpers (numpy-optional)
# ---------------------------------------------------------------------------


def _percentile(xs: List[float], q: float) -> Optional[float]:
    if not xs:
        return None
    if _HAS_NUMPY:
        return float(_np.percentile(xs, q))
    s = sorted(xs)
    k = (len(s) - 1) * (q / 100.0)
    lo = int(math.floor(k))
    hi = int(math.ceil(k))
    if lo == hi:
        return float(s[lo])
    return float(s[lo] + (s[hi] - s[lo]) * (k - lo))


def _stats(xs: List[float]) -> Dict[str, Optional[float]]:
    if not xs:
        return {"n": 0, "mean": None, "p50": None, "p90": None,
                "p95": None, "max": None, "min": None}
    return {
        "n": len(xs),
        "mean": sum(xs) / len(xs),
        "p50": _percentile(xs, 50),
        "p90": _percentile(xs, 90),
        "p95": _percentile(xs, 95),
        "max": max(xs),
        "min": min(xs),
    }


def summarize(session: ParsedSession) -> Dict[str, Any]:
    ttft = [t.ttft_ms for t in session.turns if t.ttft_ms is not None]
    e2e = [t.e2e_ms for t in session.turns if t.e2e_ms is not None]

    by_tool: Dict[str, List[float]] = {}
    tool_action_counts: Dict[str, Dict[str, int]] = {}
    for tc in session.tool_calls:
        by_tool.setdefault(tc.name, []).append(tc.elapsed_ms)
        tool_action_counts.setdefault(tc.name, {})
        tool_action_counts[tc.name][tc.action] = (
            tool_action_counts[tc.name].get(tc.action, 0) + 1
        )

    per_tool_stats = {
        name: {
            **_stats(latencies),
            "actions": tool_action_counts.get(name, {}),
        }
        for name, latencies in by_tool.items()
    }

    log_seconds = (
        (session.log_end - session.log_start).total_seconds()
        if session.log_start and session.log_end else 0.0
    )

    return {
        "log_window_seconds": log_seconds,
        "turns_total": len(session.turns),
        "turns_with_ttft": len(ttft),
        "turns_with_e2e": len(e2e),
        "turns_aborted": sum(1 for t in session.turns if t.aborted),
        "wake_greetings": session.wake_greetings,
        "aborts": session.aborts,
        "echo_drops": session.echo_drops,
        "ttft_ms": _stats(ttft),
        "e2e_ms": _stats(e2e),
        "tool_calls_total": len(session.tool_calls),
        "tool_timeouts": sum(1 for tc in session.tool_calls if tc.timed_out),
        "per_tool": per_tool_stats,
    }


# ---------------------------------------------------------------------------
# CSV / JSON dumps
# ---------------------------------------------------------------------------


def write_turns_csv(session: ParsedSession, out: Path) -> None:
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w", encoding="utf-8", newline="") as f:
        w = csv.writer(f)
        w.writerow([
            "idx", "asr_ts", "asr_text", "ttft_ms", "e2e_ms",
            "tool_count", "tools", "tts_text", "aborted",
        ])
        for t in session.turns:
            tools_str = ";".join(
                f"{tc['name']}:{tc['elapsed_ms']:.0f}ms:{tc['action']}"
                for tc in t.tools
            )
            w.writerow([
                t.idx,
                t.asr_ts.isoformat(),
                t.asr_text,
                f"{t.ttft_ms:.0f}" if t.ttft_ms is not None else "",
                f"{t.e2e_ms:.0f}" if t.e2e_ms is not None else "",
                len(t.tools),
                tools_str,
                t.tts_text or "",
                "1" if t.aborted else "0",
            ])


def write_summary_json(summary: Dict[str, Any], out: Path) -> None:
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w", encoding="utf-8") as f:
        json.dump(summary, f, indent=2, default=str, ensure_ascii=False)


# ---------------------------------------------------------------------------
# Plot helpers
# ---------------------------------------------------------------------------


def _hist_with_cdf(values: List[float], title: str, xlabel: str,
                   out_path: Path) -> None:
    if not values:
        _placeholder_plot(title + " (no data)", out_path)
        return
    fig, ax1 = plt.subplots(figsize=(8, 4.5))

    n_bins = max(8, min(40, int(math.sqrt(len(values)))))
    counts, bins, _ = ax1.hist(values, bins=n_bins, color="#5a9bd4",
                               edgecolor="white", alpha=0.85)
    ax1.set_xlabel(xlabel)
    ax1.set_ylabel("Count")
    ax1.set_title(title)
    ax1.grid(True, axis="y", linestyle=":", alpha=0.4)

    # CDF on a twin axis
    ax2 = ax1.twinx()
    sorted_v = sorted(values)
    cdf = [(i + 1) / len(sorted_v) for i in range(len(sorted_v))]
    ax2.plot(sorted_v, cdf, color="#cc4125", linewidth=1.6, label="CDF")
    ax2.set_ylabel("CDF")
    ax2.set_ylim(0, 1.02)

    # Reference markers for p50 / p95
    p50 = _percentile(values, 50)
    p95 = _percentile(values, 95)
    if p50 is not None:
        ax1.axvline(p50, color="#666", linestyle="--", linewidth=1)
        ax1.text(p50, ax1.get_ylim()[1] * 0.95, f" p50={p50:.0f}",
                 color="#444", fontsize=9, va="top")
    if p95 is not None:
        ax1.axvline(p95, color="#aa3333", linestyle="--", linewidth=1)
        ax1.text(p95, ax1.get_ylim()[1] * 0.85, f" p95={p95:.0f}",
                 color="#aa3333", fontsize=9, va="top")

    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)


def plot_e2e_distribution(values: List[float], out_path: Path) -> None:
    """Histogram of end-to-end ASR→first-TTS latency, in seconds, with
    bars colored by speed bucket and a small bucket-color legend."""
    if not values:
        _placeholder_plot("End-to-end latency (no data)", out_path)
        return

    values_s = [v / 1000.0 for v in values]

    buckets = [
        ("Fast (<2 s)", 0.0, 2.0, "#4caf50"),
        ("OK (2–3.5 s)", 2.0, 3.5, "#ffc107"),
        ("Slow (3.5–6 s)", 3.5, 6.0, "#ff7043"),
        ("Very slow (>6 s)", 6.0, float("inf"), "#c62828"),
    ]

    fig, ax = plt.subplots(figsize=(10.0, 5.0))

    max_v = max(values_s)
    bin_edges = [0.0, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 5.0, 6.0, 8.0, 10.0,
                 max(12.0, math.ceil(max_v) + 1)]
    bin_edges = sorted(set(e for e in bin_edges if e <= max(max_v + 1, 12.0)))

    counts, bins, patches = ax.hist(values_s, bins=bin_edges,
                                    edgecolor="white", linewidth=1.2)
    for patch, left in zip(patches, bins[:-1]):
        for _, lo, hi, color in buckets:
            if lo <= left < hi:
                patch.set_facecolor(color)
                break

    ax.set_xlabel("End-to-end latency (seconds)", fontsize=11)
    ax.set_ylabel("Number of turns", fontsize=11)
    ax.set_title(
        f"How long does the robot take to start replying?  "
        f"({len(values)} turns)",
        fontsize=12,
    )
    ax.grid(True, axis="y", linestyle=":", alpha=0.4)

    for patch, c in zip(patches, counts):
        if c > 0:
            ax.text(patch.get_x() + patch.get_width() / 2,
                    c + max(counts) * 0.02,
                    f"{int(c)}", ha="center", va="bottom",
                    fontsize=9, color="#333")

    ax.set_ylim(0, max(counts) * 1.18 if any(counts) else 1)

    legend_handles = [
        plt.Rectangle((0, 0), 1, 1, color=color, label=label)
        for label, _, _, color in buckets
    ]
    ax.legend(handles=legend_handles, loc="upper right", fontsize=9,
              framealpha=0.9)

    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)


def _placeholder_plot(title: str, out_path: Path) -> None:
    fig, ax = plt.subplots(figsize=(8, 4.5))
    ax.text(0.5, 0.5, "No data in log",
            ha="center", va="center", fontsize=14, color="#888")
    ax.set_title(title)
    ax.set_xticks([])
    ax.set_yticks([])
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)


def plot_tool_latency(session: ParsedSession, out_path: Path) -> None:
    by_tool: Dict[str, List[float]] = {}
    for tc in session.tool_calls:
        by_tool.setdefault(tc.name, []).append(tc.elapsed_ms)
    if not by_tool:
        _placeholder_plot("Per-tool latency (no data)", out_path)
        return

    names = sorted(
        by_tool.keys(),
        key=lambda n: -(sum(by_tool[n]) / len(by_tool[n])),
    )
    avgs_ms = [sum(by_tool[n]) / len(by_tool[n]) for n in names]
    avgs_s = [a / 1000.0 for a in avgs_ms]
    counts = [len(by_tool[n]) for n in names]

    def bar_color(s: float) -> str:
        if s < 0.5:
            return "#4caf50"   # fast
        if s < 1.5:
            return "#ffc107"   # ok
        if s < 3.0:
            return "#ff7043"   # slow
        return "#c62828"       # very slow

    colors = [bar_color(s) for s in avgs_s]

    fig, ax = plt.subplots(figsize=(11, 0.55 * len(names) + 2.2))
    ys = list(range(len(names)))
    ax.barh(ys, avgs_s, color=colors, edgecolor="white", height=0.65)

    xmax = max(avgs_s) if avgs_s else 1.0
    for y, s, ms, c in zip(ys, avgs_s, avgs_ms, counts):
        ax.text(s + xmax * 0.01, y,
                f"{ms:.0f} ms   (n={c})",
                va="center", ha="left", fontsize=9, color="#222")

    ax.set_yticks(ys)
    ax.set_yticklabels(names)
    ax.invert_yaxis()
    ax.set_xlabel("Average tool execution time (seconds)")
    ax.set_title(f"How long does each tool take to run?  "
                 f"(averaged over {sum(counts)} calls)")
    ax.set_xlim(0, xmax * 1.25 if xmax else 1)
    ax.grid(True, axis="x", linestyle=":", alpha=0.4)

    legend_handles = [
        plt.Rectangle((0, 0), 1, 1, color="#4caf50", label="Fast (<0.5 s)"),
        plt.Rectangle((0, 0), 1, 1, color="#ffc107", label="OK (0.5–1.5 s)"),
        plt.Rectangle((0, 0), 1, 1, color="#ff7043", label="Slow (1.5–3 s)"),
        plt.Rectangle((0, 0), 1, 1, color="#c62828", label="Very slow (>3 s)"),
    ]
    ax.legend(handles=legend_handles, loc="lower right", fontsize=9,
              framealpha=0.9)

    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)


def plot_tool_frequency(session: ParsedSession, out_path: Path) -> None:
    counts: Dict[str, int] = {}
    timeouts: Dict[str, int] = {}
    for tc in session.tool_calls:
        counts[tc.name] = counts.get(tc.name, 0) + 1
        if tc.timed_out:
            timeouts[tc.name] = timeouts.get(tc.name, 0) + 1
    if not counts:
        _placeholder_plot("Tool frequency (no data)", out_path)
        return

    names = sorted(counts.keys(), key=lambda n: -counts[n])
    n_calls = [counts[n] for n in names]
    n_timeouts = [timeouts.get(n, 0) for n in names]

    fig, ax = plt.subplots(figsize=(10, 0.55 * len(names) + 2))
    ys = list(range(len(names)))
    ax.barh(ys, n_calls, color="#5a9bd4", edgecolor="white", height=0.6,
            label="calls")
    if any(n_timeouts):
        ax.barh(ys, n_timeouts, color="#cc4125", edgecolor="white",
                height=0.6, label="timeouts")
    ax.set_yticks(ys)
    ax.set_yticklabels(names)
    ax.invert_yaxis()
    ax.set_xlabel("Calls")
    ax.set_title("Tool-call frequency")
    ax.legend(loc="lower right")
    ax.grid(True, axis="x", linestyle=":", alpha=0.4)
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)


def plot_latency_timeline(session: ParsedSession, out_path: Path) -> None:
    """Per-turn stacked-bar view of TTFT vs total end-to-end latency.

    For each turn we draw a bar at x=turn_index. The blue lower segment is
    the LLM TTFT (server received final ASR -> Gemini first token); the
    orange upper segment is the rest of the pipeline (TTS synth + send).
    Bar total height = end-to-end ASR->first-TTS-chunk. p50 / p95 of e2e
    are drawn as horizontal reference lines. Aborted turns get a red X.
    """
    if not session.turns:
        _placeholder_plot("Latency timeline (no data)", out_path)
        return

    idxs = [t.idx for t in session.turns]
    ttft_vals = [t.ttft_ms or 0.0 for t in session.turns]
    e2e_vals = [t.e2e_ms or 0.0 for t in session.turns]
    rest_vals = [max(0.0, e - tt) for e, tt in zip(e2e_vals, ttft_vals)]

    fig_w = max(10.0, min(20.0, 0.45 * len(idxs) + 4.0))
    fig, ax = plt.subplots(figsize=(fig_w, 5.0))

    ax.bar(idxs, ttft_vals, color="#3a78b5", edgecolor="white",
           label="LLM TTFT  (time until Gemini's first token)")
    ax.bar(idxs, rest_vals, bottom=ttft_vals, color="#f4a582",
           edgecolor="white", label="TTS synth + send  (first token → first audio out)")

    ymax = max(e2e_vals) if e2e_vals else 0.0
    for x, e in zip(idxs, e2e_vals):
        if e > 0:
            ax.text(x, e + ymax * 0.015, f"{e:.0f}",
                    ha="center", va="bottom", fontsize=8, color="#333")

    e2e_for_avg = [t.e2e_ms for t in session.turns if t.e2e_ms is not None]
    if e2e_for_avg:
        avg = sum(e2e_for_avg) / len(e2e_for_avg)
        ax.axhline(avg, color="#2e7d32", linestyle="--", linewidth=1.4,
                   alpha=0.85, label=f"average e2e = {avg:.0f} ms")

    aborted_x = [t.idx for t in session.turns if t.aborted]
    if aborted_x:
        y = ymax * 1.08 if ymax else 1
        ax.scatter(aborted_x, [y] * len(aborted_x), marker="x",
                   color="#cc4125", s=60, label="aborted (barge-in)",
                   zorder=5)

    ax.set_xlabel("Turn #")
    ax.set_ylabel("Latency (ms)")
    t0 = session.turns[0].asr_ts
    t1 = session.turns[-1].asr_ts
    ax.set_title(
        f"Per-turn latency breakdown  "
        f"({t0.strftime('%H:%M:%S')}–{t1.strftime('%H:%M:%S')}, "
        f"{len(session.turns)} turns)"
    )
    ax.set_xticks(idxs)
    if len(idxs) > 20:
        ax.tick_params(axis="x", labelsize=8)
    ax.set_ylim(0, ymax * 1.18 if ymax else 1)
    ax.grid(True, axis="y", linestyle=":", alpha=0.4)
    ax.legend(loc="upper right", fontsize=9, framealpha=0.9)

    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)


def plot_success_rate(session: ParsedSession, summary: Dict[str, Any],
                      out_path: Path) -> None:
    # Per-stage success — count of turns that have a non-null marker:
    cats = ["ASR commit", "LLM TTFT", "End-to-end TTS",
            "MCP tool calls", "TTS synth"]
    total_turns = max(1, summary["turns_total"])
    asr_ok = summary["turns_total"]
    llm_ok = summary["turns_with_ttft"]
    e2e_ok = summary["turns_with_e2e"]
    tool_total = summary["tool_calls_total"]
    tool_ok = max(0, tool_total - summary["tool_timeouts"])
    # TTS synth implicitly succeeded for every turn that produced an e2e.
    tts_ok = summary["turns_with_e2e"]

    pct = [
        100.0 * asr_ok / total_turns,
        100.0 * llm_ok / total_turns,
        100.0 * e2e_ok / total_turns,
        100.0 * tool_ok / max(1, tool_total),
        100.0 * tts_ok / total_turns,
    ]
    counts = [
        f"{asr_ok}/{total_turns}",
        f"{llm_ok}/{total_turns}",
        f"{e2e_ok}/{total_turns}",
        f"{tool_ok}/{tool_total}",
        f"{tts_ok}/{total_turns}",
    ]

    fig, ax = plt.subplots(figsize=(8.5, 4.5))
    bars = ax.bar(cats, pct, color="#3a78b5", edgecolor="white")
    for b, p, c in zip(bars, pct, counts):
        ax.text(b.get_x() + b.get_width() / 2, p + 1, f"{p:.0f}%\n{c}",
                ha="center", va="bottom", fontsize=9)
    ax.set_ylim(0, 115)
    ax.set_ylabel("Success rate (%)")
    ax.set_title("Per-stage success rate")
    ax.grid(True, axis="y", linestyle=":", alpha=0.4)
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)


# ---------------------------------------------------------------------------
# Markdown report
# ---------------------------------------------------------------------------


def write_markdown_report(summary: Dict[str, Any], out_dir: Path) -> Path:
    md = out_dir / "REPORT.md"

    def fmt_stat(s: Dict[str, Any]) -> str:
        if not s or s.get("n", 0) == 0:
            return "no data"
        return (f"n={s['n']}, mean={s['mean']:.0f}, "
                f"p50={s['p50']:.0f}, p95={s['p95']:.0f}, "
                f"max={s['max']:.0f}")

    lines: List[str] = []
    lines.append("# Jarvis voice pipeline — performance report\n")
    lines.append(f"_Generated by `analyze_metrics.py` on "
                 f"{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}_\n")
    lines.append("## Summary\n")
    lines.append(f"- Log window: **{summary['log_window_seconds']:.0f} s**")
    lines.append(f"- Turns parsed: **{summary['turns_total']}**")
    lines.append(f"- Wake greetings: **{summary['wake_greetings']}**")
    lines.append(f"- Aborts (barge-in): **{summary['aborts']}**")
    lines.append(f"- Echo-guard drops: **{summary['echo_drops']}**")
    lines.append(f"- Tool calls: **{summary['tool_calls_total']}** "
                 f"(timeouts: {summary['tool_timeouts']})")
    lines.append("")
    lines.append("### LLM TTFT (ms)\n")
    lines.append(f"- {fmt_stat(summary['ttft_ms'])}\n")
    lines.append("### End-to-end ASR→first TTS (ms)\n")
    lines.append(f"- {fmt_stat(summary['e2e_ms'])}\n")
    lines.append("### Per-tool latency (ms)\n")
    if summary["per_tool"]:
        lines.append("| Tool | n | p50 | p95 | max | actions |")
        lines.append("|------|--:|----:|----:|----:|---------|")
        for name, s in sorted(summary["per_tool"].items(),
                              key=lambda kv: -kv[1].get("n", 0)):
            actions = ", ".join(f"{a}:{c}" for a, c in
                                s.get("actions", {}).items())
            lines.append(f"| `{name}` | {s['n']} | "
                         f"{s['p50']:.0f} | {s['p95']:.0f} | "
                         f"{s['max']:.0f} | {actions} |")
    else:
        lines.append("_No tool calls in this log._")
    lines.append("")
    lines.append("## Plots\n")
    for fname, caption in [
        ("ttft_distribution.png", "LLM TTFT distribution"),
        ("e2e_distribution.png", "End-to-end ASR→TTS latency"),
        ("mcp_tool_latency.png", "MCP / plugin tool latency"),
        ("mcp_tool_frequency.png", "Tool call frequency"),
        ("latency_timeline.png", "Latency over time"),
        ("success_rate.png", "Per-stage success rate"),
    ]:
        lines.append(f"### {caption}\n\n![{caption}]({fname})\n")

    md.write_text("\n".join(lines), encoding="utf-8")
    return md


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(
        description="Parse a xiaozhi-esp32-server log and emit "
                    "performance plots + tables."
    )
    parser.add_argument(
        "--log", "-l",
        default=str(Path(__file__).resolve().parents[2]
                    / "server" / "xiaozhi-esp32-server" / "main"
                    / "xiaozhi-server" / "tmp" / "server.log"),
        help="Path to server log (defaults to the project's tmp/server.log).",
    )
    parser.add_argument(
        "--out", "-o",
        default=str(Path(__file__).resolve().parents[2]
                    / "docs" / "metrics_report"),
        help="Output directory for plots, CSV, summary JSON, and REPORT.md.",
    )
    # Session-window flags. Default scopes to the most recent server boot,
    # which matches what people usually want after a demo run. Use --all to
    # parse the entire log, or --since to pin a specific timestamp.
    win = parser.add_mutually_exclusive_group()
    win.add_argument(
        "--latest-session", action="store_true", default=True,
        help="(default) Only analyze events after the last server-start "
             "marker in the log.",
    )
    win.add_argument(
        "--all", action="store_true",
        help="Parse the entire log file (overrides --latest-session).",
    )
    win.add_argument(
        "--since",
        help="Only include events at or after this timestamp. Accepted "
             "formats: 'YYYY-MM-DD HH:MM:SS', 'YYYY-MM-DDTHH:MM:SS', "
             "or 'YYMMDD HH:MM:SS' (server console format).",
    )
    parser.add_argument(
        "--max-turns", type=int, default=30,
        help="Cap the report to the most recent N turns within the chosen "
             "window (default: 30). Use 0 to disable the cap.",
    )
    args = parser.parse_args(argv)

    log_path = Path(args.log)
    out_dir = Path(args.out)

    if not log_path.exists():
        print(f"[err] log file not found: {log_path}", file=sys.stderr)
        return 2

    # Resolve the session window:
    start_offset = 0
    since_dt: Optional[datetime] = None
    window_label = "entire log"
    if args.all:
        window_label = "entire log"
    elif args.since:
        s = args.since.strip()
        for fmt in ("%Y-%m-%d %H:%M:%S",
                    "%Y-%m-%dT%H:%M:%S",
                    "%y%m%d %H:%M:%S"):
            try:
                since_dt = datetime.strptime(s, fmt)
                break
            except ValueError:
                continue
        if since_dt is None:
            print(f"[err] could not parse --since: {args.since!r}",
                  file=sys.stderr)
            return 2
        window_label = f"since {since_dt.isoformat(sep=' ')}"
    else:
        # Default: latest server-restart boundary.
        start_offset = find_last_session_start(log_path) or 0
        if start_offset:
            window_label = f"latest session (byte offset {start_offset})"
        else:
            window_label = ("entire log (no session-start marker found, "
                            "falling back to full file)")

    print(f"[info] log:    {log_path}")
    print(f"[info] window: {window_label}")
    session = parse_log(log_path, start_offset=start_offset, since=since_dt)
    print(f"[info] parsed {len(session.turns)} turns, "
          f"{len(session.tool_calls)} tool calls")

    if args.max_turns and len(session.turns) > args.max_turns:
        kept = session.turns[-args.max_turns:]
        cutoff_ts = kept[0].asr_ts
        session.tool_calls = [tc for tc in session.tool_calls
                              if tc.ts >= cutoff_ts]
        session.turns = kept
        print(f"[info] capped to last {args.max_turns} turns "
              f"(tool calls now {len(session.tool_calls)})")

    summary = summarize(session)

    out_dir.mkdir(parents=True, exist_ok=True)
    write_turns_csv(session, out_dir / "metrics.csv")
    write_summary_json(summary, out_dir / "summary.json")

    print("[info] rendering plots ...")
    ttft_vals = [t.ttft_ms for t in session.turns if t.ttft_ms is not None]
    e2e_vals = [t.e2e_ms for t in session.turns if t.e2e_ms is not None]
    _hist_with_cdf(ttft_vals, "LLM TTFT distribution",
                   "TTFT (ms)", out_dir / "ttft_distribution.png")
    plot_e2e_distribution(e2e_vals, out_dir / "e2e_distribution.png")
    plot_tool_latency(session, out_dir / "mcp_tool_latency.png")
    plot_tool_frequency(session, out_dir / "mcp_tool_frequency.png")
    plot_latency_timeline(session, out_dir / "latency_timeline.png")
    plot_success_rate(session, summary, out_dir / "success_rate.png")

    md = write_markdown_report(summary, out_dir)
    print(f"[ok] wrote report: {md}")
    print(f"[ok] csv:          {out_dir / 'metrics.csv'}")
    print(f"[ok] json:         {out_dir / 'summary.json'}")
    print(f"[ok] plots in:     {out_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
