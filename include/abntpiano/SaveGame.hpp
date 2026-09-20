#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace abntpiano {

enum class Grade {
    S,
    A,
    B,
    C,
    D
};

inline std::string gradeToString(Grade g) {
    switch (g) {
        case Grade::S: return "S";
        case Grade::A: return "A";
        case Grade::B: return "B";
        case Grade::C: return "C";
        case Grade::D: return "D";
    }
    return "D";
}

inline Grade stringToGrade(const std::string& str) {
    if (str == "S") return Grade::S;
    if (str == "A") return Grade::A;
    if (str == "B") return Grade::B;
    if (str == "C") return Grade::C;
    return Grade::D;
}

struct PlayerSettings {
    float volume = 0.8f;
    int audioBufferFrames = 256;
    bool keyRepeatFilter = true;
};

struct ScoreRecord {
    std::string chartId;   // songId + "_" + difficultyName
    std::string profileId;
    float accuracy = 0.0f;
    int maxCombo = 0;
    Grade grade = Grade::D;
    int64_t playedAt = 0;  // Unix timestamp em segundos
};

struct PlayerProfile {
    std::string id = "default_player";
    std::string name = "Player";
    std::vector<std::string> unlockedSongs = {"prelude_op28_no4"};
    std::map<std::string, std::vector<std::string>> unlockedDifficulties = {
        {"prelude_op28_no4", {"Easy", "Normal"}}
    };
    PlayerSettings settings;
};

struct SaveGame {
    int version = 1;
    PlayerProfile profile;
    std::vector<ScoreRecord> scores;
};

} // namespace abntpiano
