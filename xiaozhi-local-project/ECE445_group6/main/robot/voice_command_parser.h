#ifndef ROBOT_VOICE_COMMAND_PARSER_H_
#define ROBOT_VOICE_COMMAND_PARSER_H_

#include <string>
#include "robot_types.h"

namespace robot {

// Two entry paths for AI-produced intent:
//   1. If the AI just gives a raw phrase, call ParsePhrase().
//   2. If the AI uses MCP tool calls (preferred), RobotApp registers tools
//      that build Command structs directly. See robot_app.cc.
class VoiceCommandParser {
public:
    // Returns Command with motion or mode set; NONE if not recognized.
    static Command ParsePhrase(const std::string& phrase);

private:
    static std::string Normalize(const std::string& s);
};

}  // namespace robot

#endif  // ROBOT_VOICE_COMMAND_PARSER_H_
