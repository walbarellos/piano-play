#pragma once

#include "abntpiano/Song.hpp"
#include "abntpiano/Chart.hpp"
#include "abntpiano/KeyboardMapper.hpp"

namespace abntpiano {

// Gerador de Chart determinístico e puro: (Song, Difficulty) -> Chart (RF12–RF15, ADR-03, ADR-06)
class ChartGenerator {
public:
    explicit ChartGenerator(KeyboardMapper mapper = KeyboardMapper())
        : mapper_(mapper) {}

    // Função pura de geração de Chart a partir de Song e Difficulty
    Chart generateChart(const Song& song, const DifficultyConfig& difficulty) const;

    const KeyboardMapper& mapper() const { return mapper_; }
    void setMapper(const KeyboardMapper& mapper) { mapper_ = mapper; }

private:
    KeyboardMapper mapper_;
};

} // namespace abntpiano
