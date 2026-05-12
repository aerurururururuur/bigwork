#include "representation/bg_music_controller.hpp"

#include <algorithm>
#include <string>

namespace representation {

BgMusicController::~BgMusicController() { stop(); }

bool BgMusicController::open(const std::filesystem::path& absPath) {
    stop();
    opened_ = false;
    std::error_code ec;
    if (absPath.empty() || !std::filesystem::exists(absPath, ec)) {
        return false;
    }
    const std::string file = absPath.string();
    if (!music_.openFromFile(file)) {
        return false;
    }
    music_.setLoop(true);
    opened_ = true;
    return true;
}

void BgMusicController::setVolume(float zeroToHundred) {
    const float v = std::clamp(zeroToHundred, 0.f, 100.f);
    music_.setVolume(v);
}

void BgMusicController::playLooping() {
    if (!opened_) {
        return;
    }
    music_.setLoop(true);
    if (music_.getStatus() != sf::Music::Playing) {
        music_.play();
    }
}

void BgMusicController::pause() {
    if (!opened_) {
        return;
    }
    if (music_.getStatus() == sf::Music::Playing) {
        music_.pause();
    }
}

void BgMusicController::stop() {
    if (!opened_) {
        return;
    }
    music_.stop();
}

void BgMusicController::syncBattleOnly(bool inBattle) {
    if (!opened_) {
        return;
    }
    if (inBattle) {
        playLooping();
    } else {
        pause();
    }
}

} // namespace representation
