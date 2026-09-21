#pragma once

#include "abntpiano/Song.hpp"
#include <string>
#include <vector>

namespace abntpiano {

struct SongEntry {
    std::string filePath;
    std::string title;
    std::string composer;
};

// Catálogo curado de músicas (Content Pipeline / Domain).
// Garante músicas únicas, autênticas e sem redundâncias.
class SongCatalog {
public:
    SongCatalog() = default;

    // Carrega o catálogo padrão sem redundâncias nem duplicatas
    bool loadDefaultCatalog();

    bool addSong(const std::string& filePath, const std::string& title, const std::string& composer);

    size_t size() const { return songs_.size(); }
    size_t songCount() const { return songs_.size(); }
    bool empty() const { return songs_.empty(); }

    const Song& getSong(size_t index) const;
    const std::vector<Song>& allSongs() const { return songs_; }

    size_t nextIndex(size_t currentIndex) const;
    size_t prevIndex(size_t currentIndex) const;

private:
    std::vector<Song> songs_;
};

} // namespace abntpiano
