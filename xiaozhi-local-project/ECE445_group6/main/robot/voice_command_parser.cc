#include "voice_command_parser.h"

#include <cctype>
#include <string>

namespace robot {

std::string VoiceCommandParser::Normalize(const std::string& s) {
    std::string r; r.reserve(s.size());
    for (char c : s) {
        if (std::isalpha(static_cast<unsigned char>(c))) r += std::tolower(c);
        else if (c == ' ') r += ' ';
    }
    // collapse multi-space
    std::string out; bool sp=false;
    for (char c : r) {
        if (c==' ') { if (!sp) out+=c; sp=true; }
        else { out+=c; sp=false; }
    }
    while (!out.empty() && out.front()==' ') out.erase(out.begin());
    while (!out.empty() && out.back()==' ')  out.pop_back();
    return out;
}

Command VoiceCommandParser::ParsePhrase(const std::string& phrase) {
    std::string p = Normalize(phrase);
    Command c = {};
    c.source = CommandSource::AI_VOICE;

    // Longer, multi-word phrases first to avoid "left" eating "spin left".
    // Primary user-facing name is "human control" / "human control mode".
    // Legacy "manual"/"manual mode" phrasings are kept as aliases so the
    // existing docs, bench-test scripts, and any LLM turn that still says
    // "manual" keep working.
    if (p == "human control" || p == "human control mode" ||
        p == "switch to human control" || p == "switch to human control mode" ||
        p == "human mode" || p == "switch to human" || p == "switch to human mode" ||
        p == "manual mode" || p == "switch to manual" ||
        p == "switch to manual mode" || p == "manual control" || p == "manual") {
        c.mode = ModeCmd::SWITCH_MANUAL; return c;
    }
    if (p == "ai mode" || p == "switch to ai" ||
        p == "switch to ai mode" || p == "voice mode" || p == "switch to voice mode") {
        c.mode = ModeCmd::SWITCH_AI; return c;
    }
    if (p == "spin left"  || p == "turn left in place" || p == "rotate left")  { c.motion = MotionCmd::SPIN_LEFT;  return c; }
    if (p == "spin right" || p == "turn right in place" || p == "rotate right") { c.motion = MotionCmd::SPIN_RIGHT; return c; }
    if (p == "forward" || p == "go forward" || p == "go forwards" ||
        p == "move forward" || p == "move forwards" ||
        p == "moving forward" || p == "ahead") {
        c.motion = MotionCmd::FORWARD; return c;
    }
    if (p == "backward" || p == "backwards" || p == "go back" ||
        p == "go backward" || p == "go backwards" ||
        p == "move back" || p == "move backward" || p == "move backwards" ||
        p == "moving backward" || p == "reverse" || p == "back") {
        c.motion = MotionCmd::BACKWARD; return c;
    }
    if (p == "left"  || p == "turn left" || p == "go left" ||
        p == "move left" || p == "moving left")  { c.motion = MotionCmd::LEFT;  return c; }
    if (p == "right" || p == "turn right" || p == "go right" ||
        p == "move right" || p == "moving right") { c.motion = MotionCmd::RIGHT; return c; }
    if (p == "stop" || p == "halt" || p == "brake") { c.motion = MotionCmd::STOP; return c; }
    return c;
}

}  // namespace robot
