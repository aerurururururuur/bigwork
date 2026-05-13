#include "representation/bg_music_controller.hpp"

#include <algorithm>
#include <string>

namespace representation {

BgMusicController::~BgMusicController() { stop(); }

bool BgMusicController::tryOpen(sf::Music& out, const std::filesystem::path& absPath) {
    std::error_code ec;
    if (absPath.empty() || !std::filesystem::exists(absPath, ec)) {
        return false;
    }
    const std::string file = absPath.string();
    out.openFromFile(file);
    if (out.getDuration() == sf::Time::Zero) {
        out.stop();
        return false;
    }
    out.setLoop(true);
    return true;
}

bool BgMusicController::openNormalTrack(const std::filesystem::path& absPath) {
    normal_ready_ = false;
    if (music_normal_.getStatus() == sf::Music::Playing || music_normal_.getStatus() == sf::Music::Paused) {
        music_normal_.stop();
    }
    if (!tryOpen(music_normal_, absPath)) {
        return false;
    }
    normal_ready_ = true;
    return true;
}

bool BgMusicController::openBossTrack(const std::filesystem::path& absPath) {
    boss_ready_ = false;
    if (music_boss_.getStatus() == sf::Music::Playing || music_boss_.getStatus() == sf::Music::Paused) {
        music_boss_.stop();
    }
    if (!tryOpen(music_boss_, absPath)) {
        return false;
    }
    boss_ready_ = true;
    return true;
}

void BgMusicController::setVolume(float zeroToHundred) {
    const float v = std::clamp(zeroToHundred, 0.f, 100.f);
    if (normal_ready_) {
        music_normal_.setVolume(v);
    }
    if (boss_ready_) {
        music_boss_.setVolume(v);
    }
}

void BgMusicController::stop() {
    if (normal_ready_) {
        music_normal_.stop();
    }
    if (boss_ready_) {
        music_boss_.stop();
    }
}

void BgMusicController::pauseIfPlaying(sf::Music& m, bool ready) {
    if (!ready) {
        return;
    }
    if (m.getStatus() == sf::Music::Playing) {
        m.pause();
    }
}

void BgMusicController::playLooping(sf::Music& m, bool ready) {
    if (!ready) {
        return;
    }
    m.setLoop(true);
    if (m.getStatus() != sf::Music::Playing) {
        m.play();
    }
}

void BgMusicController::syncBattleTracks(bool inBattle, bool bossOnField) {
    if (!normal_ready_ && !boss_ready_) {
        return;
    }
    if (!inBattle) {
        pauseIfPlaying(music_normal_, normal_ready_);
        pauseIfPlaying(music_boss_, boss_ready_);
        return;
    }

    if (bossOnField) {
        if (boss_ready_) {
            playLooping(music_boss_, boss_ready_);
            pauseIfPlaying(music_normal_, normal_ready_);
            return;
        }
        if (normal_ready_) {
            playLooping(music_normal_, normal_ready_);
            pauseIfPlaying(music_boss_, boss_ready_);
            return;
        }
    } else {
        if (normal_ready_) {
            playLooping(music_normal_, normal_ready_);
            pauseIfPlaying(music_boss_, boss_ready_);
            return;
        }
        if (boss_ready_) {
            playLooping(music_boss_, boss_ready_);
            pauseIfPlaying(music_normal_, normal_ready_);
            return;
        }
    }

    pauseIfPlaying(music_normal_, normal_ready_);
    pauseIfPlaying(music_boss_, boss_ready_);
}

} // namespace representation
