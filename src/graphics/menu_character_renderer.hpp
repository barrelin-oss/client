#pragma once

#include <cstdint>
#include <vector>

namespace hb
{

class renderer;
class sprite_manager;

// PAK loading entry for incremental initialization
struct equipment_pak_entry
{
    const char* pak_name;
    uint32_t sprite_id;
    uint32_t sprite_count;
    uint32_t pak_start_index; // Starting index within the PAK file (for multi-variant PAKs)
};

// Full appearance data for rendering a character preview with equipment
struct character_appearance
{
    // Base appearance
    uint8_t gender = 1;          // 1 = male, 2 = female
    uint8_t skin_color = 1;      // 1-3 (Black, White, Yellow)
    uint8_t hair_style = 0;      // 0-7
    uint8_t hair_color = 0;      // 0-15
    uint8_t underwear_color = 0; // 0-7

    // Equipment (0 = not equipped)
    uint8_t body_armor = 0; // Body armor/shirt type (1-15)
    uint8_t arm_armor = 0;  // Arm armor type (1-15)
    uint8_t pants = 0;      // Pants/skirt type (1-15, 1=skirt for female)
    uint8_t boots = 0;      // Boots type (1-15)
    uint8_t helmet = 0;     // Helmet type (1-15)
    uint8_t mantle = 0;     // Cape/mantle type (1-15)
    uint8_t weapon = 0;     // Weapon type (1-255)
    uint8_t shield = 0;     // Shield type (1-9)

    // Effects
    uint8_t weapon_glow = 0; // Weapon glow effect (0=none, 1=red, 2=green, 3=blue)
    uint8_t shield_glow = 0; // Shield glow effect (0=none, 1=red, 2=green, 3=blue)

    // Returns true if character has any equipment
    bool has_equipment() const
    {
        return body_armor > 0 || arm_armor > 0 || pants > 0 || boots > 0 || helmet > 0 || mantle > 0 || weapon > 0 ||
               shield > 0;
    }
};

// Renders character previews for menu screens (character select/create)
// Full implementation of DrawObject_OnMove_ForMenu with equipment support
class menu_character_renderer
{
public:
    menu_character_renderer() = default;

    // Load required sprites for character rendering (loads all PAKs at once)
    bool initialize(sprite_manager& sprites);

    // Get list of all equipment PAKs needed (for incremental loading)
    static std::vector<equipment_pak_entry> get_pak_load_list();

    // Mark as initialized (call after all PAKs from get_pak_load_list are loaded)
    void set_initialized() { initialized_ = true; }

    // Draw a character at the given position
    // x, y: Screen position (character center/feet)
    // appearance: Character appearance settings (including equipment)
    // direction: 1-8 (compass directions, 1=N, 2=NE, 3=E, 4=SE, 5=S, 6=SW, 7=W, 8=NW)
    // frame: Animation frame (0-7)
    void draw(renderer& rend,
              sprite_manager& sprites,
              int32_t x,
              int32_t y,
              const character_appearance& appearance,
              int32_t direction,
              int32_t frame);

    // Draw with shadow (effect sprite 0, frame 1)
    void draw_with_shadow(renderer& rend,
                          sprite_manager& sprites,
                          int32_t x,
                          int32_t y,
                          const character_appearance& appearance,
                          int32_t direction,
                          int32_t frame);

private:
    // Helper to determine if female based on gender
    bool is_female(uint8_t gender) const { return gender == 2; }

    // Get owner type (1-3 male, 4-6 female based on skin color)
    int32_t get_owner_type(uint8_t gender, uint8_t skin_color) const;

    // Calculate frame index: (direction - 1) * 8 + frame
    int32_t calc_frame(int32_t direction, int32_t frame) const;

    // Draw individual components (returns true if drawn)
    void draw_body(renderer& rend,
                   sprite_manager& sprites,
                   int32_t x,
                   int32_t y,
                   uint8_t gender,
                   uint8_t skin_color,
                   int32_t action,
                   int32_t dir,
                   int32_t frame);
    void draw_underwear(renderer& rend,
                        sprite_manager& sprites,
                        int32_t x,
                        int32_t y,
                        uint8_t gender,
                        uint8_t underwear_color,
                        int32_t action,
                        int32_t dir,
                        int32_t frame);
    void draw_hair(renderer& rend,
                   sprite_manager& sprites,
                   int32_t x,
                   int32_t y,
                   uint8_t gender,
                   uint8_t hair_style,
                   uint8_t hair_color,
                   int32_t action,
                   int32_t dir,
                   int32_t frame);
    void draw_body_armor(renderer& rend,
                         sprite_manager& sprites,
                         int32_t x,
                         int32_t y,
                         uint8_t gender,
                         uint8_t armor_type,
                         int32_t action,
                         int32_t dir,
                         int32_t frame);
    void draw_arm_armor(renderer& rend,
                        sprite_manager& sprites,
                        int32_t x,
                        int32_t y,
                        uint8_t gender,
                        uint8_t armor_type,
                        int32_t action,
                        int32_t dir,
                        int32_t frame);
    void draw_pants(renderer& rend,
                    sprite_manager& sprites,
                    int32_t x,
                    int32_t y,
                    uint8_t gender,
                    uint8_t pants_type,
                    int32_t action,
                    int32_t dir,
                    int32_t frame);
    void draw_boots(renderer& rend,
                    sprite_manager& sprites,
                    int32_t x,
                    int32_t y,
                    uint8_t gender,
                    uint8_t boots_type,
                    int32_t action,
                    int32_t dir,
                    int32_t frame);
    void draw_helmet(renderer& rend,
                     sprite_manager& sprites,
                     int32_t x,
                     int32_t y,
                     uint8_t gender,
                     uint8_t helmet_type,
                     int32_t action,
                     int32_t dir,
                     int32_t frame);
    void draw_mantle(renderer& rend,
                     sprite_manager& sprites,
                     int32_t x,
                     int32_t y,
                     uint8_t gender,
                     uint8_t mantle_type,
                     int32_t action,
                     int32_t dir,
                     int32_t frame);
    void draw_weapon(renderer& rend,
                     sprite_manager& sprites,
                     int32_t x,
                     int32_t y,
                     uint8_t gender,
                     uint8_t weapon_type,
                     int32_t action,
                     int32_t dir,
                     int32_t frame);
    void draw_shield(renderer& rend,
                     sprite_manager& sprites,
                     int32_t x,
                     int32_t y,
                     uint8_t gender,
                     uint8_t shield_type,
                     int32_t action,
                     int32_t dir,
                     int32_t frame);

    // Sprite base indices (from legacy code)
    // Body: Male = 500 + (skin-1)*120, Female = 500 + (3+skin-1)*120
    static constexpr uint16_t body_base = 500;
    static constexpr uint16_t body_stride = 120; // 15 actions * 8 directions

    // Underwear: Male = 4580, Female = 14580 (stride 15 per color)
    static constexpr uint16_t male_underwear_base = 4580;
    static constexpr uint16_t female_underwear_base = 14580;
    static constexpr uint16_t underwear_stride = 15;

    // Hair: Male = 4820, Female = 14820 (stride 15 per style)
    static constexpr uint16_t male_hair_base = 4820;
    static constexpr uint16_t female_hair_base = 14820;
    static constexpr uint16_t hair_stride = 15;

    // Body armor: Male = 5060, Female = 15060 (stride 15 per type)
    static constexpr uint16_t male_body_armor_base = 5060;
    static constexpr uint16_t female_body_armor_base = 15060;
    static constexpr uint16_t body_armor_stride = 15;

    // Arm armor: Male = 5300, Female = 15300 (stride 15 per type)
    static constexpr uint16_t male_arm_armor_base = 5300;
    static constexpr uint16_t female_arm_armor_base = 15300;
    static constexpr uint16_t arm_armor_stride = 15;

    // Pants: Male = 5540, Female = 15540 (stride 15 per type)
    static constexpr uint16_t male_pants_base = 5540;
    static constexpr uint16_t female_pants_base = 15540;
    static constexpr uint16_t pants_stride = 15;

    // Boots: Male = 5780, Female = 15780 (stride 15 per type)
    static constexpr uint16_t male_boots_base = 5780;
    static constexpr uint16_t female_boots_base = 15780;
    static constexpr uint16_t boots_stride = 15;

    // Weapons: Male = 6020, Female = 16020 (stride 64 per type: 8 frames * 8 directions)
    static constexpr uint16_t male_weapon_base = 6020;
    static constexpr uint16_t female_weapon_base = 16020;
    static constexpr uint16_t weapon_stride = 64;

    // Shields: Male = 9100, Female = 19100 (stride 8 per type)
    static constexpr uint16_t male_shield_base = 9100;
    static constexpr uint16_t female_shield_base = 19100;
    static constexpr uint16_t shield_stride = 8;

    // Mantles: Male = 9600, Female = 19600 (stride 15 per type)
    static constexpr uint16_t male_mantle_base = 9600;
    static constexpr uint16_t female_mantle_base = 19600;
    static constexpr uint16_t mantle_stride = 15;

    // Helmets: Male = 9300, Female = 19300 (stride 15 per type)
    static constexpr uint16_t male_helmet_base = 9300;
    static constexpr uint16_t female_helmet_base = 19300;
    static constexpr uint16_t helmet_stride = 15;

    // Action indices
    static constexpr int32_t action_stop = 0;
    static constexpr int32_t action_walk = 2;
    static constexpr int32_t action_move = 3; // Used when moving/animating

    // Drawing order arrays (indexed by direction 1-8)
    // 0 = weapon drawn last (in front), 1 = weapon drawn first (behind)
    static constexpr int8_t drawing_order[9] = {0, 1, 0, 0, 0, 0, 0, 1, 1};
    // Mantle draw position: 0=early, 1=after shield, 2=between shield and armor
    static constexpr int8_t mantle_drawing_order[9] = {0, 1, 1, 1, 0, 0, 0, 2, 2};

    bool initialized_ = false;
};

} // namespace hb
