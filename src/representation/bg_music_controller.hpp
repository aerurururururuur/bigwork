#pragma once

#include <SFML/Audio/Music.hpp>

#include <filesystem>

namespace representation {

/** SFML-backed BGM: two optional loop streams (normal / boss), pause/stop without blocking tick. */
class BgMusicController {
public:
    BgMusicController() = default;
    ~BgMusicController();

    BgMusicController(const BgMusicController&) = delete;
    BgMusicController& operator=(const BgMusicController&) = delete;
    BgMusicController(BgMusicController&&) = delete;
    BgMusicController& operator=(BgMusicController&&) = delete;

    bool openNormalTrack(const std::filesystem::path& absPath);
    bool openBossTrack(const std::filesystem::path& absPath);

    bool hasAnyTrack() const { return normal_ready_ || boss_ready_; }

    /** SFML volume scale 0-100; applied to every opened stream. */
    void setVolume(float zeroToHundred);

    void stop();

    /**
     * When not in battle: pause both (keeps playback offsets).
     * In battle: if bossOnField and boss track opened, play boss; else if normal track opened, play normal;
     * if only one track opened, that track is used for the whole battle.
     */
    void syncBattleTracks(bool inBattle, bool bossOnField);

private:
    static bool tryOpen(sf::Music& out, const std::filesystem::path& absPath);
    static void pauseIfPlaying(sf::Music& m, bool ready);
    static void playLooping(sf::Music& m, bool ready);

    sf::Music music_normal_{};
    sf::Music music_boss_{};
    bool normal_ready_{false};
    bool boss_ready_{false};
};

} // namespace representation
