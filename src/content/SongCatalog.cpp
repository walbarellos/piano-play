#include "abntpiano/SongCatalog.hpp"
#include "abntpiano/MidiImporter.hpp"
#include <iostream>

namespace abntpiano {

static const std::vector<SongEntry> kDefaultCatalogEntries = {
    {"assets/songs/beethoven_fur_elise.mid",       "Für Elise (WoO 59)",                 "L.v. Beethoven"},
    {"assets/songs/chopin_prelude_op28_no4.mid",   "Prelude Op. 28 No. 4",               "F. Chopin"},
    {"assets/songs/mozart_alla_turca.mid",         "Rondo Alla Turca (KV 331, III)",     "W.A. Mozart"},
    {"assets/songs/chopin_ballade_no1_op23.mid",   "Ballade No. 1 em Sol menor, Op. 23", "F. Chopin"},
    {"assets/songs/beethoven_kreutzer_presto.mid", "Sonata No. 9 'Kreutzer' - I. Presto","L.v. Beethoven"}
};

bool SongCatalog::loadDefaultCatalog() {
    songs_.clear();
    for (const auto& entry : kDefaultCatalogEntries) {
        addSong(entry.filePath, entry.title, entry.composer);
    }
    return !songs_.empty();
}

bool SongCatalog::addSong(const std::string& filePath, const std::string& title, const std::string& composer) {
    MidiImporter importer;
    auto res = importer.importFromFile(filePath);
    if (!res.success || res.song.chordGroups.empty()) {
        std::cerr << "[SongCatalog] Aviso: falha ao carregar " << filePath << ": " << res.errorMessage << "\n";
        return false;
    }

    Song s = std::move(res.song);
    s.title = title;
    s.composer = composer;
    songs_.push_back(std::move(s));
    return true;
}

const Song& SongCatalog::getSong(size_t index) const {
    static const Song kEmptySong;
    if (songs_.empty()) return kEmptySong;
    return songs_[index % songs_.size()];
}

size_t SongCatalog::nextIndex(size_t currentIndex) const {
    if (songs_.empty()) return 0;
    return (currentIndex + 1) % songs_.size();
}

size_t SongCatalog::prevIndex(size_t currentIndex) const {
    if (songs_.empty()) return 0;
    return (currentIndex + songs_.size() - 1) % songs_.size();
}

} // namespace abntpiano
