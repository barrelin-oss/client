#include "assets/pak_file.hpp"
#include <spdlog/spdlog.h>
#include <cstring>
#include <algorithm>

namespace hb {

namespace {

// Windows BMP structures (packed)
#pragma pack(push, 1)
struct bmp_file_header {
    uint16_t type;          // 'BM' signature
    uint32_t size;          // File size
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset_bits;   // Offset to pixel data
};

struct bmp_info_header {
    uint32_t size;          // Header size (40 for BITMAPINFOHEADER)
    int32_t width;
    int32_t height;         // Negative = top-down, positive = bottom-up
    uint16_t planes;        // Always 1
    uint16_t bit_count;     // 1, 4, 8, 16, 24, or 32
    uint32_t compression;   // 0=RGB, 3=BITFIELDS
    uint32_t size_image;
    int32_t x_pels_per_meter;
    int32_t y_pels_per_meter;
    uint32_t clr_used;
    uint32_t clr_important;
};
#pragma pack(pop)

// Convert 16-bit RGB565 to RGBA8888
inline void rgb565_to_rgba(uint16_t pixel, uint8_t& r, uint8_t& g, uint8_t& b) {
    // RGB565: RRRRRGGGGGGBBBBB
    r = static_cast<uint8_t>(((pixel >> 11) & 0x1F) * 255 / 31);
    g = static_cast<uint8_t>(((pixel >> 5) & 0x3F) * 255 / 63);
    b = static_cast<uint8_t>((pixel & 0x1F) * 255 / 31);
}

// Convert 16-bit RGB555 to RGBA8888
inline void rgb555_to_rgba(uint16_t pixel, uint8_t& r, uint8_t& g, uint8_t& b) {
    // RGB555: XRRRRRGGGGGBBBBB
    r = static_cast<uint8_t>(((pixel >> 10) & 0x1F) * 255 / 31);
    g = static_cast<uint8_t>(((pixel >> 5) & 0x1F) * 255 / 31);
    b = static_cast<uint8_t>((pixel & 0x1F) * 255 / 31);
}

} // anonymous namespace

pak_file::~pak_file() {
    close();
}

pak_file::pak_file(pak_file&& other) noexcept
    : file_(std::move(other.file_))
    , path_(std::move(other.path_))
    , sprite_offsets_(std::move(other.sprite_offsets_))
    , sprite_count_(other.sprite_count_) {
    other.sprite_count_ = 0;
}

pak_file& pak_file::operator=(pak_file&& other) noexcept {
    if (this != &other) {
        close();
        file_ = std::move(other.file_);
        path_ = std::move(other.path_);
        sprite_offsets_ = std::move(other.sprite_offsets_);
        sprite_count_ = other.sprite_count_;
        other.sprite_count_ = 0;
    }
    return *this;
}

// Find a file by case-insensitive match when exact path doesn't exist.
// Returns the actual path on disk, or empty if not found.
static std::string find_case_insensitive(const std::filesystem::path& target)
{
    auto parent = target.parent_path();
    auto filename = target.filename().string();

    std::error_code ec;
    if (!std::filesystem::exists(parent, ec))
    {
        return {};
    }

    // Convert target filename to lowercase for comparison
    std::string lower_target = filename;
    std::transform(lower_target.begin(), lower_target.end(), lower_target.begin(),
        [](unsigned char c) { return std::tolower(c); });

    for (const auto& entry : std::filesystem::directory_iterator(parent, ec))
    {
        auto candidate = entry.path().filename().string();
        std::string lower_candidate = candidate;
        std::transform(lower_candidate.begin(), lower_candidate.end(), lower_candidate.begin(),
            [](unsigned char c) { return std::tolower(c); });

        if (lower_candidate == lower_target)
        {
            return entry.path().string();
        }
    }

    return {};
}

bool pak_file::open(std::string_view path) {
    close();

    std::string path_str(path);
    file_.open(path_str, std::ios::binary);

    // Fallback: case-insensitive match (handles SLM.PAK vs slm.pak on Linux)
    if (!file_.is_open()) {
        auto resolved = find_case_insensitive(std::filesystem::path(path_str));
        if (!resolved.empty()) {
            file_.open(resolved, std::ios::binary);
            if (file_.is_open()) {
                path_ = resolved;
                spdlog::trace("PAK case-insensitive match: {} -> {}", path, resolved);
            }
        }
    }

    if (!file_.is_open()) {
        spdlog::error("Failed to open PAK file: {}", path);
        return false;
    }

    if (path_.empty()) {
        path_ = path;
    }

    if (!read_header()) {
        close();
        return false;
    }

    spdlog::trace("Opened PAK file: {} ({} sprites)", path, sprite_count_);
    return true;
}

void pak_file::close() {
    if (file_.is_open()) {
        file_.close();
    }
    path_.clear();
    sprite_offsets_.clear();
    sprite_count_ = 0;
}

bool pak_file::read_header() {
    // Read first 4 bytes to get sprite count
    // PAK format: first 24 bytes are header, then sprite_count * 8 bytes of offset table

    // Seek to beginning and read initial data
    file_.seekg(0, std::ios::end);
    auto file_size = file_.tellg();
    file_.seekg(0, std::ios::beg);

    if (file_size < 24) {
        spdlog::error("PAK file too small");
        return false;
    }

    // Read header - first 4 bytes might be magic/version, next values are for indexing
    // Based on the code: SetFilePointer(hPakFile, 24 + sNthFile*8, NULL, FILE_BEGIN);
    // This means at offset 24 + (index * 8), there's a 4-byte offset to the sprite data

    // Determine sprite count by reading offsets until we hit invalid data
    // For now, let's try to read the first offset and see how the file is structured

    // Skip to offset 24 (start of sprite offset table)
    file_.seekg(24, std::ios::beg);

    // Read offsets until we hit an invalid one or reach reasonable limit
    std::vector<uint32_t> offsets;
    constexpr uint32_t max_sprites = 10000;

    for (uint32_t i = 0; i < max_sprites; ++i) {
        uint32_t offset = 0;
        uint32_t unknown = 0;

        // Each entry is 8 bytes: 4 bytes offset, 4 bytes unknown
        if (!file_.read(reinterpret_cast<char*>(&offset), 4)) break;
        if (!file_.read(reinterpret_cast<char*>(&unknown), 4)) break;

        // Check if this is a valid offset
        if (offset == 0 || offset >= static_cast<uint32_t>(file_size)) {
            break;
        }

        offsets.push_back(offset);
    }

    if (offsets.empty()) {
        spdlog::error("No valid sprites found in PAK file");
        return false;
    }

    sprite_offsets_ = std::move(offsets);
    sprite_count_ = static_cast<uint32_t>(sprite_offsets_.size());

    return true;
}

std::optional<pak_sprite_data> pak_file::read_sprite(uint32_t index) {
    if (!file_.is_open() || index >= sprite_count_) {
        return std::nullopt;
    }

    // Clear any error flags from previous reads to allow re-reading
    file_.clear();
    return read_sprite_internal(sprite_offsets_[index]);
}

std::optional<pak_sprite_data> pak_file::read_sprite_internal(uint32_t offset) {
    pak_sprite_data result;

    // Seek to sprite data start + 100 bytes (header/padding)
    // Legacy format: First 100 bytes are "Sprite Confirm" header
    file_.seekg(offset + 100, std::ios::beg);
    if (!file_) {
        spdlog::error("Failed to seek to sprite data");
        return std::nullopt;
    }

    // Read total frame count (4 bytes)
    int32_t total_frames = 0;
    if (!file_.read(reinterpret_cast<char*>(&total_frames), 4)) {
        spdlog::error("Failed to read frame count");
        return std::nullopt;
    }

    if (total_frames <= 0 || total_frames > 10000) {
        spdlog::error("Invalid frame count: {}", total_frames);
        return std::nullopt;
    }

    // Read frame brush data (stBrush: 12 bytes each = 6 shorts)
    // sx, sy, szx (width), szy (height), pvx (pivot x), pvy (pivot y)
    result.frames.resize(total_frames);
    for (int32_t i = 0; i < total_frames; ++i) {
        auto& frame = result.frames[i];
        file_.read(reinterpret_cast<char*>(&frame.source_x), 2);
        file_.read(reinterpret_cast<char*>(&frame.source_y), 2);
        file_.read(reinterpret_cast<char*>(&frame.width), 2);
        file_.read(reinterpret_cast<char*>(&frame.height), 2);
        file_.read(reinterpret_cast<char*>(&frame.pivot_x), 2);
        file_.read(reinterpret_cast<char*>(&frame.pivot_y), 2);

        if (!file_) {
            spdlog::error("Failed to read frame data");
            return std::nullopt;
        }
    }

    // Bitmap data starts at: offset + 104 + 4 (frame count) + (12 * total_frames)
    // = offset + 108 + (12 * total_frames)
    auto bitmap_start = offset + 108 + (12 * total_frames);
    file_.seekg(bitmap_start, std::ios::beg);

    // Read BMP file header (14 bytes)
    bmp_file_header file_header;
    if (!file_.read(reinterpret_cast<char*>(&file_header), sizeof(file_header))) {
        spdlog::error("Failed to read BMP file header");
        return std::nullopt;
    }

    // Verify BMP signature
    if (file_header.type != 0x4D42) { // 'BM' in little-endian
        spdlog::warn("Invalid BMP signature: 0x{:04X}", file_header.type);
        // Try to proceed anyway - might be raw DIB
    }

    // Read BMP info header
    bmp_info_header info_header;
    if (!file_.read(reinterpret_cast<char*>(&info_header), sizeof(info_header))) {
        spdlog::error("Failed to read BMP info header");
        return std::nullopt;
    }

    result.bitmap_width = static_cast<uint32_t>(std::abs(info_header.width));
    result.bitmap_height = static_cast<uint32_t>(std::abs(info_header.height));
    bool top_down = info_header.height < 0;

    spdlog::trace("BMP: {}x{}, {} bits, compression: {}",
        result.bitmap_width, result.bitmap_height,
        info_header.bit_count, info_header.compression);

    // Calculate row stride (BMP rows are padded to 4-byte boundary)
    uint32_t bytes_per_pixel = info_header.bit_count / 8;
    uint32_t row_size = (result.bitmap_width * bytes_per_pixel + 3) & ~3;

    // Read color table if needed (for 8-bit or less)
    std::vector<uint32_t> color_table;
    if (info_header.bit_count <= 8) {
        uint32_t num_colors = info_header.clr_used;
        if (num_colors == 0) {
            num_colors = 1u << info_header.bit_count;
        }
        color_table.resize(num_colors);
        file_.read(reinterpret_cast<char*>(color_table.data()), num_colors * 4);
    }

    // Seek to pixel data
    file_.seekg(bitmap_start + file_header.offset_bits, std::ios::beg);

    // Read raw pixel data
    uint32_t pixel_data_size = row_size * result.bitmap_height;
    std::vector<uint8_t> raw_pixels(pixel_data_size);
    if (!file_.read(reinterpret_cast<char*>(raw_pixels.data()), pixel_data_size)) {
        // Partial read may be acceptable
        spdlog::warn("Partial BMP pixel data read: {} of {} bytes",
            file_.gcount(), pixel_data_size);
    }

    // Convert to RGBA format for SFML
    // SFML expects top-down RGBA8888
    result.bitmap_data.resize(result.bitmap_width * result.bitmap_height * 4);

    for (uint32_t y = 0; y < result.bitmap_height; ++y) {
        // Source row (BMP is usually bottom-up unless height is negative)
        uint32_t src_y = top_down ? y : (result.bitmap_height - 1 - y);
        const uint8_t* src_row = raw_pixels.data() + src_y * row_size;

        // Destination row (top-down RGBA)
        uint8_t* dst_row = result.bitmap_data.data() + y * result.bitmap_width * 4;

        for (uint32_t x = 0; x < result.bitmap_width; ++x) {
            uint8_t r, g, b, a = 255;

            switch (info_header.bit_count) {
                case 16: {
                    // 16-bit color (RGB555 or RGB565)
                    uint16_t pixel = *reinterpret_cast<const uint16_t*>(src_row + x * 2);
                    // Helbreath typically uses RGB565 (16-bit high color)
                    if (info_header.compression == 3) {
                        // BI_BITFIELDS - check actual format
                        // For now assume RGB565
                        rgb565_to_rgba(pixel, r, g, b);
                    } else {
                        // Default to RGB555 for uncompressed 16-bit
                        rgb555_to_rgba(pixel, r, g, b);
                    }
                    break;
                }
                case 24: {
                    // 24-bit BGR
                    b = src_row[x * 3 + 0];
                    g = src_row[x * 3 + 1];
                    r = src_row[x * 3 + 2];
                    break;
                }
                case 32: {
                    // 32-bit BGRA
                    b = src_row[x * 4 + 0];
                    g = src_row[x * 4 + 1];
                    r = src_row[x * 4 + 2];
                    a = src_row[x * 4 + 3];
                    break;
                }
                case 8: {
                    // 8-bit indexed
                    uint8_t idx = src_row[x];
                    if (idx < color_table.size()) {
                        uint32_t color = color_table[idx];
                        b = (color >> 0) & 0xFF;
                        g = (color >> 8) & 0xFF;
                        r = (color >> 16) & 0xFF;
                    } else {
                        r = g = b = 0;
                    }
                    break;
                }
                default:
                    r = g = b = 128; // Unsupported format
                    break;
            }

            dst_row[x * 4 + 0] = r;
            dst_row[x * 4 + 1] = g;
            dst_row[x * 4 + 2] = b;
            dst_row[x * 4 + 3] = a;
        }
    }

    return result;
}

std::optional<pak_sprite_metadata> pak_file::read_sprite_metadata(uint32_t index) {
    if (!file_.is_open() || index >= sprite_count_) {
        return std::nullopt;
    }
    // Clear any error flags from previous reads to allow re-reading
    file_.clear();
    return read_metadata_internal(sprite_offsets_[index]);
}

std::optional<std::vector<uint8_t>> pak_file::read_sprite_bitmap(uint32_t index) {
    if (!file_.is_open() || index >= sprite_count_) {
        return std::nullopt;
    }
    // Clear any error flags from previous reads to allow re-reading
    file_.clear();
    return read_bitmap_internal(sprite_offsets_[index]);
}

std::optional<pak_sprite_metadata> pak_file::read_metadata_internal(uint32_t offset) {
    pak_sprite_metadata result;

    // Seek to sprite data start + 100 bytes (header/padding)
    file_.seekg(offset + 100, std::ios::beg);
    if (!file_) {
        spdlog::error("Failed to seek to sprite metadata");
        return std::nullopt;
    }

    // Read total frame count (4 bytes)
    int32_t total_frames = 0;
    if (!file_.read(reinterpret_cast<char*>(&total_frames), 4)) {
        spdlog::error("Failed to read frame count");
        return std::nullopt;
    }

    if (total_frames <= 0 || total_frames > 10000) {
        spdlog::error("Invalid frame count: {}", total_frames);
        return std::nullopt;
    }

    // Read frame brush data (stBrush: 12 bytes each)
    result.frames.resize(total_frames);
    for (int32_t i = 0; i < total_frames; ++i) {
        auto& frame = result.frames[i];
        file_.read(reinterpret_cast<char*>(&frame.source_x), 2);
        file_.read(reinterpret_cast<char*>(&frame.source_y), 2);
        file_.read(reinterpret_cast<char*>(&frame.width), 2);
        file_.read(reinterpret_cast<char*>(&frame.height), 2);
        file_.read(reinterpret_cast<char*>(&frame.pivot_x), 2);
        file_.read(reinterpret_cast<char*>(&frame.pivot_y), 2);

        if (!file_) {
            spdlog::error("Failed to read frame data");
            return std::nullopt;
        }
    }

    // Store bitmap offset for later loading
    result.bitmap_offset = offset + 108 + (12 * total_frames);

    // Read BMP headers to get dimensions (without loading pixel data)
    file_.seekg(result.bitmap_offset, std::ios::beg);

    bmp_file_header file_header;
    if (!file_.read(reinterpret_cast<char*>(&file_header), sizeof(file_header))) {
        spdlog::error("Failed to read BMP file header");
        return std::nullopt;
    }

    bmp_info_header info_header;
    if (!file_.read(reinterpret_cast<char*>(&info_header), sizeof(info_header))) {
        spdlog::error("Failed to read BMP info header");
        return std::nullopt;
    }

    result.bitmap_width = static_cast<uint32_t>(std::abs(info_header.width));
    result.bitmap_height = static_cast<uint32_t>(std::abs(info_header.height));

    return result;
}

std::optional<std::vector<uint8_t>> pak_file::read_bitmap_internal(uint32_t offset) {
    // First read metadata to find bitmap location
    auto metadata = read_metadata_internal(offset);
    if (!metadata) {
        return std::nullopt;
    }

    // Clear any error flags and seek to bitmap data
    file_.clear();
    file_.seekg(metadata->bitmap_offset, std::ios::beg);

    // Read BMP headers
    bmp_file_header file_header;
    if (!file_.read(reinterpret_cast<char*>(&file_header), sizeof(file_header))) {
        return std::nullopt;
    }

    bmp_info_header info_header;
    if (!file_.read(reinterpret_cast<char*>(&info_header), sizeof(info_header))) {
        return std::nullopt;
    }

    uint32_t width = static_cast<uint32_t>(std::abs(info_header.width));
    uint32_t height = static_cast<uint32_t>(std::abs(info_header.height));
    bool top_down = info_header.height < 0;

    // Calculate row stride
    uint32_t bytes_per_pixel = info_header.bit_count / 8;
    uint32_t row_size = (width * bytes_per_pixel + 3) & ~3;

    // Read color table if needed
    std::vector<uint32_t> color_table;
    if (info_header.bit_count <= 8) {
        uint32_t num_colors = info_header.clr_used;
        if (num_colors == 0) {
            num_colors = 1u << info_header.bit_count;
        }
        color_table.resize(num_colors);
        file_.read(reinterpret_cast<char*>(color_table.data()), num_colors * 4);
    }

    // Clear any error flags and seek to pixel data
    file_.clear();
    file_.seekg(metadata->bitmap_offset + file_header.offset_bits, std::ios::beg);

    // Read raw pixel data
    uint32_t pixel_data_size = row_size * height;
    std::vector<uint8_t> raw_pixels(pixel_data_size);
    if (!file_.read(reinterpret_cast<char*>(raw_pixels.data()), pixel_data_size)) {
        spdlog::warn("Partial BMP pixel data read");
    }

    // Convert to RGBA
    std::vector<uint8_t> result(width * height * 4);

    for (uint32_t y = 0; y < height; ++y) {
        uint32_t src_y = top_down ? y : (height - 1 - y);
        const uint8_t* src_row = raw_pixels.data() + src_y * row_size;
        uint8_t* dst_row = result.data() + y * width * 4;

        for (uint32_t x = 0; x < width; ++x) {
            uint8_t r, g, b, a = 255;

            switch (info_header.bit_count) {
                case 16: {
                    uint16_t pixel = *reinterpret_cast<const uint16_t*>(src_row + x * 2);
                    if (info_header.compression == 3) {
                        rgb565_to_rgba(pixel, r, g, b);
                    } else {
                        rgb555_to_rgba(pixel, r, g, b);
                    }
                    break;
                }
                case 24:
                    b = src_row[x * 3 + 0];
                    g = src_row[x * 3 + 1];
                    r = src_row[x * 3 + 2];
                    break;
                case 32:
                    b = src_row[x * 4 + 0];
                    g = src_row[x * 4 + 1];
                    r = src_row[x * 4 + 2];
                    a = src_row[x * 4 + 3];
                    break;
                case 8: {
                    uint8_t idx = src_row[x];
                    if (idx < color_table.size()) {
                        uint32_t color = color_table[idx];
                        b = (color >> 0) & 0xFF;
                        g = (color >> 8) & 0xFF;
                        r = (color >> 16) & 0xFF;
                    } else {
                        r = g = b = 0;
                    }
                    break;
                }
                default:
                    r = g = b = 128;
                    break;
            }

            dst_row[x * 4 + 0] = r;
            dst_row[x * 4 + 1] = g;
            dst_row[x * 4 + 2] = b;
            dst_row[x * 4 + 3] = a;
        }
    }

    return result;
}

} // namespace hb
