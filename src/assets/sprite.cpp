#include "assets/sprite.hpp"
#include "assets/pak_file.hpp"
#include <spdlog/spdlog.h>

namespace hb {

const sprite_frame sprite::null_frame_ = {sf::IntRect({0, 0}, {0, 0}), 0, 0};

// === Metadata-only loading ===

bool sprite::load_metadata_from_pak(pak_file& pak, uint32_t index) {
    auto metadata = pak.read_sprite_metadata(index);
    if (!metadata) {
        spdlog::error("Failed to read sprite metadata {} from PAK", index);
        return false;
    }

    // Store PAK source for on-demand bitmap loading
    pak_source_ = &pak;
    pak_index_ = index;

    return load_metadata(*metadata);
}

bool sprite::load_metadata(const pak_sprite_metadata& metadata) {
    frames_.clear();
    frames_.reserve(metadata.frames.size());

    for (const auto& src_frame : metadata.frames) {
        sprite_frame frame;
        frame.source_rect = sf::IntRect(
            {src_frame.source_x, src_frame.source_y},
            {src_frame.width, src_frame.height}
        );
        frame.pivot_x = src_frame.pivot_x;
        frame.pivot_y = src_frame.pivot_y;
        frames_.push_back(frame);
    }

    // If no frames defined, create a single frame for the whole bitmap
    if (frames_.empty()) {
        sprite_frame frame;
        frame.source_rect = sf::IntRect({0, 0},
            {static_cast<int>(metadata.bitmap_width),
             static_cast<int>(metadata.bitmap_height)});
        frame.pivot_x = 0;
        frame.pivot_y = 0;
        frames_.push_back(frame);
    }

    bitmap_width_ = metadata.bitmap_width;
    bitmap_height_ = metadata.bitmap_height;
    metadata_loaded_ = true;

    spdlog::trace("Loaded sprite metadata: {} frames, {}x{} bitmap",
        frames_.size(), bitmap_width_, bitmap_height_);

    return true;
}

// === Full loading (metadata + bitmap) ===

bool sprite::load_from_pak(pak_file& pak, uint32_t index) {
    auto data = pak.read_sprite(index);
    if (!data) {
        spdlog::error("Failed to read sprite {} from PAK", index);
        return false;
    }

    // Store PAK source in case we need to reload
    pak_source_ = &pak;
    pak_index_ = index;

    return load_from_data(*data);
}

bool sprite::load_from_file(std::string_view path) {
    if (!texture_.loadFromFile(std::string(path))) {
        spdlog::error("Failed to load sprite from file: {}", path);
        return false;
    }

    // Create a single frame covering the entire texture
    frames_.clear();
    sprite_frame frame;
    frame.source_rect = sf::IntRect({0, 0},
        {static_cast<int>(texture_.getSize().x),
         static_cast<int>(texture_.getSize().y)});
    frame.pivot_x = 0;
    frame.pivot_y = 0;
    frames_.push_back(frame);

    bitmap_width_ = texture_.getSize().x;
    bitmap_height_ = texture_.getSize().y;
    metadata_loaded_ = true;
    bitmap_loaded_ = true;
    touch();
    return true;
}

bool sprite::load_from_data(const pak_sprite_data& data) {
    if (data.bitmap_data.empty()) {
        spdlog::error("Empty bitmap data");
        return false;
    }

    if (data.bitmap_width == 0 || data.bitmap_height == 0) {
        spdlog::error("Invalid bitmap dimensions: {}x{}", data.bitmap_width, data.bitmap_height);
        return false;
    }

    // Store frame info (metadata)
    frames_.clear();
    frames_.reserve(data.frames.size());

    for (const auto& src_frame : data.frames) {
        sprite_frame frame;
        frame.source_rect = sf::IntRect(
            {src_frame.source_x, src_frame.source_y},
            {src_frame.width, src_frame.height}
        );
        frame.pivot_x = src_frame.pivot_x;
        frame.pivot_y = src_frame.pivot_y;
        frames_.push_back(frame);
    }

    if (frames_.empty()) {
        sprite_frame frame;
        frame.source_rect = sf::IntRect({0, 0},
            {static_cast<int>(data.bitmap_width),
             static_cast<int>(data.bitmap_height)});
        frame.pivot_x = 0;
        frame.pivot_y = 0;
        frames_.push_back(frame);
    }

    bitmap_width_ = data.bitmap_width;
    bitmap_height_ = data.bitmap_height;
    metadata_loaded_ = true;

    // Load bitmap into texture
    size_t expected_size = data.bitmap_width * data.bitmap_height * 4;

    sf::Image image;

    if (data.bitmap_data.size() == expected_size) {
        // Data is in RGBA8888 format from pak_file
        image.resize({data.bitmap_width, data.bitmap_height});

        for (uint32_t y = 0; y < data.bitmap_height; ++y) {
            for (uint32_t x = 0; x < data.bitmap_width; ++x) {
                size_t idx = (y * data.bitmap_width + x) * 4;
                uint8_t r = data.bitmap_data[idx + 0];
                uint8_t g = data.bitmap_data[idx + 1];
                uint8_t b = data.bitmap_data[idx + 2];
                uint8_t a = data.bitmap_data[idx + 3];
                image.setPixel({x, y}, sf::Color(r, g, b, a));
            }
        }
    } else if (data.bitmap_data.size() >= 2 &&
               data.bitmap_data[0] == 'B' && data.bitmap_data[1] == 'M') {
        // Standard BMP file
        if (!image.loadFromMemory(data.bitmap_data.data(), data.bitmap_data.size())) {
            spdlog::error("Failed to load BMP from memory");
            return false;
        }
    } else {
        if (!image.loadFromMemory(data.bitmap_data.data(), data.bitmap_data.size())) {
            spdlog::error("Failed to load unknown bitmap format");
            return false;
        }
    }

    // Store color key from top-left pixel
    if (image.getSize().x > 0 && image.getSize().y > 0) {
        color_key_ = image.getPixel({0, 0});
    }

    // Create texture WITHOUT color key first
    if (!texture_no_colorkey_.loadFromImage(image)) {
        spdlog::error("Failed to create no-colorkey texture from image");
        return false;
    }

    // Apply color key mask for transparent texture (sets alpha=0 for color key pixels)
    image.createMaskFromColor(color_key_);

    if (!texture_.loadFromImage(image)) {
        spdlog::error("Failed to create texture from image");
        return false;
    }

    bitmap_loaded_ = true;
    touch();

    return true;
}

// === On-demand bitmap loading ===

bool sprite::load_bitmap() {
    if (bitmap_loaded_) {
        touch();
        return true;
    }

    if (!metadata_loaded_) {
        spdlog::error("Cannot load bitmap: metadata not loaded");
        return false;
    }

    if (!pak_source_) {
        spdlog::error("Cannot load bitmap: no PAK source set (index was {})", pak_index_);
        return false;
    }

    // Read bitmap data from PAK
    auto bitmap_data = pak_source_->read_sprite_bitmap(pak_index_);
    if (!bitmap_data) {
        spdlog::error("Failed to read bitmap from PAK (index {}, pak open: {})",
                      pak_index_, pak_source_->is_open());
        return false;
    }

    // Validate data size
    size_t expected_size = bitmap_width_ * bitmap_height_ * 4;
    if (bitmap_data->size() != expected_size) {
        spdlog::error("Bitmap data size mismatch: got {}, expected {} ({}x{})",
                      bitmap_data->size(), expected_size, bitmap_width_, bitmap_height_);
        return false;
    }

    // Create image from RGBA data
    sf::Image image;
    image.resize({bitmap_width_, bitmap_height_});

    for (uint32_t y = 0; y < bitmap_height_; ++y) {
        for (uint32_t x = 0; x < bitmap_width_; ++x) {
            size_t idx = (y * bitmap_width_ + x) * 4;
            uint8_t r = (*bitmap_data)[idx + 0];
            uint8_t g = (*bitmap_data)[idx + 1];
            uint8_t b = (*bitmap_data)[idx + 2];
            uint8_t a = (*bitmap_data)[idx + 3];
            image.setPixel({x, y}, sf::Color(r, g, b, a));
        }
    }

    // Store color key
    if (image.getSize().x > 0 && image.getSize().y > 0) {
        color_key_ = image.getPixel({0, 0});
    }

    // Create no-colorkey texture
    if (!texture_no_colorkey_.loadFromImage(image)) {
        spdlog::error("Failed to create no-colorkey texture");
        return false;
    }

    // Create color-keyed texture (sets alpha=0 for color key pixels)
    image.createMaskFromColor(color_key_);

    if (!texture_.loadFromImage(image)) {
        spdlog::error("Failed to create texture");
        return false;
    }

    bitmap_loaded_ = true;
    touch();

    return true;
}

void sprite::unload_bitmap() {
    if (!bitmap_loaded_) return;

    // Create empty 1x1 textures to release GPU memory
    sf::Image empty;
    empty.resize({1, 1});
    (void)texture_.loadFromImage(empty);
    (void)texture_no_colorkey_.loadFromImage(empty);

    bitmap_loaded_ = false;
}

bool sprite::ensure_loaded() const {
    if (bitmap_loaded_) {
        touch();
        return true;
    }

    // Need to cast away const for on-demand loading
    return const_cast<sprite*>(this)->load_bitmap();
}

// === Memory management ===

void sprite::touch() const {
    last_used_ = std::chrono::steady_clock::now();
}

float sprite::seconds_since_use() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_used_);
    return duration.count() / 1000.0f;
}

size_t sprite::memory_usage() const {
    if (!bitmap_loaded_) return 0;

    // Approximate: 2 textures * width * height * 4 bytes per pixel
    return 2 * bitmap_width_ * bitmap_height_ * 4;
}

// === Frame access ===

const sprite_frame& sprite::get_frame(uint32_t index) const {
    if (index < frames_.size()) {
        return frames_[index];
    }
    return null_frame_;
}

// === Drawing ===

void sprite::draw(sf::RenderTarget& target, int32_t x, int32_t y, uint32_t frame) const {
    if (!ensure_loaded() || frame >= frames_.size()) {
        return;
    }

    const auto& f = frames_[frame];

    sf::Sprite spr(texture_, f.source_rect);
    float draw_x = static_cast<float>(x + f.pivot_x);
    float draw_y = static_cast<float>(y + f.pivot_y);
    spr.setPosition({draw_x, draw_y});
    target.draw(spr);
}

void sprite::draw_alpha(sf::RenderTarget& target, int32_t x, int32_t y,
                        uint32_t frame, float alpha) const {
    if (!ensure_loaded() || frame >= frames_.size()) {
        return;
    }

    const auto& f = frames_[frame];

    sf::Sprite spr(texture_, f.source_rect);
    spr.setPosition({
        static_cast<float>(x + f.pivot_x),
        static_cast<float>(y + f.pivot_y)
    });

    uint8_t alpha_byte = static_cast<uint8_t>(std::clamp(alpha * 255.0f, 0.0f, 255.0f));
    spr.setColor(sf::Color(255, 255, 255, alpha_byte));

    target.draw(spr);
}

void sprite::draw_width(sf::RenderTarget& target, int32_t x, int32_t y,
                        uint32_t frame, int32_t width) const {
    if (!ensure_loaded() || frame >= frames_.size() || width <= 0) {
        return;
    }

    const auto& f = frames_[frame];

    // Clamp width to sprite frame width
    int32_t draw_w = std::min(width, f.source_rect.size.x);

    // Create a modified source rect with reduced width
    sf::IntRect partial_rect(
        f.source_rect.position,
        {draw_w, f.source_rect.size.y}
    );

    sf::Sprite spr(texture_, partial_rect);
    spr.setPosition({
        static_cast<float>(x + f.pivot_x),
        static_cast<float>(y + f.pivot_y)
    });

    target.draw(spr);
}

void sprite::draw_no_color_key(sf::RenderTarget& target, int32_t x, int32_t y, uint32_t frame) const {
    if (!ensure_loaded() || frame >= frames_.size()) {
        return;
    }

    const auto& f = frames_[frame];

    sf::Sprite spr(texture_no_colorkey_, f.source_rect);
    float draw_x = static_cast<float>(x + f.pivot_x);
    float draw_y = static_cast<float>(y + f.pivot_y);
    spr.setPosition({draw_x, draw_y});
    target.draw(spr);
}

void sprite::draw_alpha_no_color_key(sf::RenderTarget& target, int32_t x, int32_t y,
                                      uint32_t frame, float alpha) const {
    if (!ensure_loaded() || frame >= frames_.size()) {
        return;
    }

    const auto& f = frames_[frame];

    sf::Sprite spr(texture_no_colorkey_, f.source_rect);
    spr.setPosition({
        static_cast<float>(x + f.pivot_x),
        static_cast<float>(y + f.pivot_y)
    });

    uint8_t alpha_byte = static_cast<uint8_t>(std::clamp(alpha * 255.0f, 0.0f, 255.0f));
    spr.setColor(sf::Color(255, 255, 255, alpha_byte));

    target.draw(spr);
}

sf::IntRect sprite::get_bounds(int32_t x, int32_t y, uint32_t frame) const {
    if (frame >= frames_.size()) {
        return sf::IntRect({0, 0}, {0, 0});
    }

    const auto& f = frames_[frame];
    return sf::IntRect(
        {x + f.pivot_x, y + f.pivot_y},
        {f.source_rect.size.x, f.source_rect.size.y}
    );
}

const sf::Texture& sprite::texture() const {
    ensure_loaded();
    return texture_;
}

} // namespace hb
