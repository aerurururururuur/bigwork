#pragma once

#include <SFML/Audio/Music.hpp>

#include <filesystem>

namespace representation {

/** SFML-backed BGM: stream from disk, loop, pause/stop without blocking the game tick. */
class BgMusicController {
public:
    BgMusicController() = default;
    ~BgMusicController();

    BgMusicController(const BgMusicController&) = delete;
    BgMusicController& operator=(const BgMusicController&) = delete;
    BgMusicController(BgMusicController&&) = delete;
    BgMusicController& operator=(BgMusicController&&) = delete;

    /** Load file from absolute path. Returns false if open fails; then ready() is false. */
    bool open(const std::filesystem::path& absPath);

    bool ready() const { return opened_; }

    /** SFML volume scale 0–100. */
    void setVolume(float zeroToHundred);

    void playLooping();
    void pause();
    void stop();

    /** Battle on: loop play; off: pause (resume position when battle resumes). */
    void syncBattleOnly(bool inBattle);

private:
    sf::Music music_;
    bool opened_{false};
};

} // namespace representation
