#include "representation/boss_bullet_frames.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace representation {

namespace {

bool png_file(const std::filesystem::path& p) {
    std::string ext = p.extension().string();
    for (char& c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return ext == ".png";
}

} // namespace

void BossBulletFrameResources::load_from_directory(const std::string& root) {
    strips_.assign(static_cast<std::size_t>(kNumStrips), {});
    const std::filesystem::path base(root);
    for (int s = 0; s < kNumStrips; ++s) {
        const auto dir = base / std::to_string(s + 1);
        std::error_code ec;
        if (!std::filesystem::is_directory(dir, ec) || ec) {
            continue;
        }
        std::vector<std::filesystem::path> files;
        std::filesystem::directory_iterator it(dir, ec);
        if (ec) {
            continue;
        }
        const std::filesystem::directory_iterator end_it{};
        while (it != end_it) {
            const std::filesystem::directory_entry& entry = *it;
            if (entry.is_regular_file() && png_file(entry.path())) {
                files.push_back(entry.path());
            }
            it.increment(ec);
            if (ec) {
                break;
            }
        }
        std::sort(files.begin(), files.end());
        if (files.empty()) {
            continue;
        }
        std::vector<sf::Texture> textures;
        textures.reserve(files.size());
        bool ok = true;
        sf::Vector2u first_size{0, 0};
        for (const auto& f : files) {
            sf::Texture t;
            const std::string path_str = f.string();
            if (!t.loadFromFile(path_str)) {
                ok = false;
                break;
            }
            const auto sz = t.getSize();
            if (sz.x == 0u || sz.y == 0u) {
                ok = false;
                break;
            }
            if (first_size.x == 0u) {
                first_size = sz;
            } else if (sz != first_size) {
                ok = false;
                break;
            }
            textures.push_back(std::move(t));
        }
        if (ok && !textures.empty()) {
            strips_[static_cast<std::size_t>(s)] = std::move(textures);
        }
    }
}

bool BossBulletFrameResources::valid() const noexcept {
    for (const auto& v : strips_) {
        if (!v.empty()) {
            return true;
        }
    }
    return false;
}

bool BossBulletFrameResources::strip_ready(std::uint8_t strip) const noexcept {
    if (strip >= static_cast<std::uint8_t>(kNumStrips)) {
        return false;
    }
    return !strips_[static_cast<std::size_t>(strip)].empty();
}

int BossBulletFrameResources::frame_count(std::uint8_t strip) const noexcept {
    if (strip >= static_cast<std::uint8_t>(kNumStrips)) {
        return 0;
    }
    return static_cast<int>(strips_[static_cast<std::size_t>(strip)].size());
}

const sf::Texture* BossBulletFrameResources::frame(std::uint8_t strip, int frame_index) const noexcept {
    if (!strip_ready(strip)) {
        return nullptr;
    }
    const auto& v = strips_[static_cast<std::size_t>(strip)];
    const int n = static_cast<int>(v.size());
    if (n <= 0) {
        return nullptr;
    }
    int i = frame_index % n;
    if (i < 0) {
        i += n;
    }
    return &v[static_cast<std::size_t>(i)];
}

} // namespace representation
