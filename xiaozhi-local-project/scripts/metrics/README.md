# Voice-pipeline metrics scripts

Server-log-driven analysis for the Jarvis robot. The Python server already
logs every metric we need on every voice turn — this script just parses
those lines, computes statistics, and renders plots.

## Quick start

```bash
# Use the venv that already has matplotlib installed
cd /Users/gaojiajun/Desktop/xiaozhi-local-project
./server/xiaozhi-esp32-server/main/xiaozhi-server/.venv/bin/python \
    scripts/metrics/analyze_metrics.py
```

Outputs land in `docs/metrics_report/`:

| File | What it is |
|------|------------|
| `REPORT.md` | Markdown summary with embedded plots — drop into slides |
| `metrics.csv` | One row per voice turn (idx, ts, ASR text, TTFT, e2e, tools, …) |
| `summary.json` | Aggregate stats (n / mean / p50 / p90 / p95 / max) |
| `ttft_distribution.png` | LLM time-to-first-token histogram + CDF |
| `e2e_distribution.png` | ASR → first TTS chunk histogram + CDF |
| `mcp_tool_latency.png` | Per-tool p50 / p95 latency bar chart |
| `mcp_tool_frequency.png` | Tool-call frequency (with timeouts in red) |
| `latency_timeline.png` | TTFT and e2e plotted vs time |
| `success_rate.png` | Per-stage success rate (ASR / LLM / TTS / MCP) |

## Session window

By default the script only analyzes the **most recent server boot** —
i.e. everything after the last `启动全局GC管理器` line in the log. That
matches what you usually want after a demo run: stop the server, run
the analyzer, get numbers for *this* session.

| Flag | Meaning |
|------|---------|
| _none / `--latest-session`_ | (default) From the most recent server-start marker to end-of-log. |
| `--all` | Parse the entire log file (every session ever logged). |
| `--since "YYYY-MM-DD HH:MM:SS"` | Only events at or after this timestamp. Also accepts `YYMMDD HH:MM:SS` (the server's console format). |

## Custom log path / output dir

```bash
.venv/bin/python scripts/metrics/analyze_metrics.py \
    --log /path/to/captured/session.log \
    --out /tmp/jarvis_report_2026_04 \
    --since "2026-04-25 17:00:00"
```

The parser handles **both** server log formats:

- File-rotated (`tmp/server.log`):
  `2026-04-25 14:32:00 - 0.9.2_… - core.providers.llm.gemini.gemini - INFO - … - Gemini TTFT 1011ms model=…`
- Console / terminal capture (e.g. the IDE terminal `.txt` file):
  `260424 15:09:57[0.9.2_…][plugins_func.functions.play_music]-INFO-播放完成`

So you can point it at either the server's rotating log file or a captured
terminal session and get the same result.

## What metrics it pulls

Every line the server already prints. No instrumentation changes required.

- `ASR arrived chars=NN text=…` → turn boundary + ASR text
- `Gemini TTFT NNNms model=… think_budget=… tools=…` → LLM TTFT
- `工具完成 tool=NAME elapsed_ms=… action=… result_size=…` → MCP tool elapsed
- `工具超时 tool=NAME elapsed_ms=… timeout=…` → MCP tool timeout
- `发送第一段语音: <text> | e2e_asr_to_tts_ms=NNN` → end-to-end latency
- `Playing wake greeting: …` → wake count
- `Abort message received` → barge-in / abort count
- `忽略疑似回声短句 (echo guard)` → echo-guard drop count

Turns are stitched together by chronological proximity (each ASR-arrived
line opens a new turn; each first-TTS-chunk line closes it). All tool
calls and the TTFT line that fall inside the open window are attached to
the current turn — good enough for the demo-scale sessions we run.

## Dependencies

- Python 3.10+
- `matplotlib` (already installed in
  `server/xiaozhi-esp32-server/main/xiaozhi-server/.venv`)
- `numpy` is **optional** — used only for slightly nicer histogram-bin
  selection. Falls back to pure-Python percentiles otherwise.
