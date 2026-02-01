# Helbreath Client - C++20 Modernization Guide

## Project Overview

This is the **Helbreath** game client, a 2D MMORPG client originally developed circa 2002-2003. The codebase is written in pre-C++11 style (approximately C++98) using DirectX 7 for graphics, input, and audio, with WinSock2 for networking.

### Current State

- **Language**: Pre-C++11 (C++98 era)
- **Graphics API**: DirectX 7 (DirectDraw)
- **Input**: DirectInput 7
- **Audio**: DirectSound 7
- **Networking**: WinSock2 with custom binary protocol
- **Architecture**: Monolithic with a single 48,000+ line `CGame` class
- **Build System**: None (legacy Visual C++ 6.0 / VS .NET era project)

### Key Statistics

| Component | Count |
|-----------|-------|
| Header files | 41 |
| Source files | 27 |
| Lines in Game.cpp | ~48,500 |
| Dialogue box types | 41 |
| Magic spell types | 100+ |
| Skill types | 60 |
| Supported languages | 5 |

---

## Modernization Goals

### Primary Objectives

1. **C++20 Standard Compliance** - Leverage modern language features
2. **Subsystem Architecture** - Clear separation of concerns
3. **Memory Safety** - RAII and smart pointers throughout
4. **Type Safety** - Strong typing, concepts, and compile-time checks
5. **Maintainability** - Small, focused classes with single responsibilities
6. **Testability** - Decoupled components that can be unit tested
7. **Cross-Platform Potential** - Abstract platform-specific code

### Secondary Objectives

1. **Modern Build System** - CMake with presets
2. **Dependency Management** - vcpkg or Conan integration
3. **Structured Logging** - Replace ad-hoc logging
4. **Configuration System** - Runtime configuration vs compile-time #defines
5. **Documentation** - Doxygen-compatible comments

---

## Target Architecture

### Directory Structure

```
client/
├── CMakeLists.txt
├── cmake/
│   ├── CompilerWarnings.cmake
│   └── StaticAnalyzers.cmake
├── src/
│   ├── main.cpp                    # Entry point only
│   ├── Application.cpp/h           # Application lifecycle
│   │
│   ├── core/                       # Core utilities
│   │   ├── Types.h                 # Common type definitions
│   │   ├── Assert.h                # Debug assertions
│   │   ├── Logger.cpp/h            # Logging system
│   │   ├── Config.cpp/h            # Configuration management
│   │   ├── Timer.cpp/h             # High-resolution timing
│   │   ├── EventBus.cpp/h          # Publish/subscribe events
│   │   └── ResourceHandle.h        # RAII resource wrapper
│   │
│   ├── math/                       # Math utilities
│   │   ├── Vec2.h                  # 2D vector
│   │   ├── Rect.h                  # Rectangle
│   │   └── Color.h                 # RGBA color
│   │
│   ├── platform/                   # Platform abstraction
│   │   ├── Platform.h              # Platform detection
│   │   ├── Window.cpp/h            # Window management
│   │   └── FileSystem.cpp/h        # File I/O abstraction
│   │
│   ├── graphics/                   # Rendering subsystem
│   │   ├── Renderer.cpp/h          # Render interface
│   │   ├── RenderContext.cpp/h     # Render state management
│   │   ├── Texture.cpp/h           # Texture resource
│   │   ├── Sprite.cpp/h            # Sprite rendering
│   │   ├── SpriteSheet.cpp/h       # Sprite atlas
│   │   ├── TextRenderer.cpp/h      # Text rendering
│   │   ├── EffectRenderer.cpp/h    # Visual effects
│   │   └── backends/
│   │       ├── D3D11Renderer.cpp/h # Direct3D 11 backend
│   │       └── SDLRenderer.cpp/h   # SDL2 backend (optional)
│   │
│   ├── audio/                      # Audio subsystem
│   │   ├── AudioSystem.cpp/h       # Audio manager
│   │   ├── SoundBuffer.cpp/h       # Sound resource
│   │   ├── MusicPlayer.cpp/h       # Background music
│   │   └── backends/
│   │       └── XAudio2Backend.cpp/h
│   │
│   ├── input/                      # Input subsystem
│   │   ├── InputSystem.cpp/h       # Input manager
│   │   ├── InputState.h            # Current input state
│   │   ├── KeyCodes.h              # Key code definitions
│   │   └── InputBindings.cpp/h     # Configurable bindings
│   │
│   ├── network/                    # Networking subsystem
│   │   ├── NetworkSystem.cpp/h     # Network manager
│   │   ├── Connection.cpp/h        # Socket connection
│   │   ├── PacketBuilder.cpp/h     # Packet construction
│   │   ├── PacketReader.cpp/h      # Packet parsing
│   │   ├── Protocol.h              # Protocol definitions
│   │   └── MessageHandler.cpp/h    # Message dispatch
│   │
│   ├── assets/                     # Asset management
│   │   ├── AssetManager.cpp/h      # Central asset registry
│   │   ├── PakFile.cpp/h           # PAK archive reader
│   │   ├── SpriteLoader.cpp/h      # Sprite asset loader
│   │   └── SoundLoader.cpp/h       # Sound asset loader
│   │
│   ├── world/                      # World/Map subsystem
│   │   ├── World.cpp/h             # World state
│   │   ├── Map.cpp/h               # Map data structure
│   │   ├── Tile.cpp/h              # Tile definition
│   │   ├── MapRenderer.cpp/h       # Map rendering
│   │   └── MapLoader.cpp/h         # Map file loading
│   │
│   ├── entity/                     # Entity subsystem
│   │   ├── Entity.cpp/h            # Base entity
│   │   ├── EntityManager.cpp/h     # Entity registry
│   │   ├── Player.cpp/h            # Player entity
│   │   ├── Character.cpp/h         # NPC/other players
│   │   ├── Monster.cpp/h           # Monster entity
│   │   └── components/             # Entity components
│   │       ├── Transform.h
│   │       ├── Sprite.h
│   │       ├── Animation.h
│   │       ├── Combat.h
│   │       └── Movement.h
│   │
│   ├── gameplay/                   # Game mechanics
│   │   ├── GameState.cpp/h         # Game state machine
│   │   ├── Combat.cpp/h            # Combat system
│   │   ├── Magic.cpp/h             # Magic/spell system
│   │   ├── Skills.cpp/h            # Skill system
│   │   ├── Inventory.cpp/h         # Inventory management
│   │   ├── Equipment.cpp/h         # Equipment system
│   │   ├── Crafting.cpp/h          # Item crafting
│   │   ├── Guild.cpp/h             # Guild system
│   │   ├── Party.cpp/h             # Party system
│   │   └── Crusade.cpp/h           # Crusade/war system
│   │
│   ├── ui/                         # UI subsystem
│   │   ├── UISystem.cpp/h          # UI manager
│   │   ├── UIElement.cpp/h         # Base UI element
│   │   ├── UIRenderer.cpp/h        # UI rendering
│   │   ├── widgets/                # UI widgets
│   │   │   ├── Button.cpp/h
│   │   │   ├── Label.cpp/h
│   │   │   ├── TextInput.cpp/h
│   │   │   ├── ScrollBar.cpp/h
│   │   │   ├── ListBox.cpp/h
│   │   │   └── ProgressBar.cpp/h
│   │   └── dialogs/                # Game dialogs
│   │       ├── DialogBase.cpp/h
│   │       ├── CharacterDialog.cpp/h
│   │       ├── InventoryDialog.cpp/h
│   │       ├── SpellbookDialog.cpp/h
│   │       ├── SkillsDialog.cpp/h
│   │       ├── ChatDialog.cpp/h
│   │       ├── ShopDialog.cpp/h
│   │       ├── GuildDialog.cpp/h
│   │       ├── BankDialog.cpp/h
│   │       └── ... (other dialogs)
│   │
│   ├── chat/                       # Chat subsystem
│   │   ├── ChatSystem.cpp/h        # Chat manager
│   │   ├── ChatMessage.h           # Message structure
│   │   ├── ChatFilter.cpp/h        # Profanity filter
│   │   └── ChatHistory.cpp/h       # Message history
│   │
│   └── localization/               # Localization
│       ├── Localization.cpp/h      # String manager
│       └── strings/
│           ├── en.json
│           ├── ko.json
│           ├── zh_cn.json
│           ├── zh_tw.json
│           └── ja.json
│
├── include/                        # Public headers (if library)
├── tests/                          # Unit tests
│   ├── CMakeLists.txt
│   ├── core/
│   ├── network/
│   └── ...
├── assets/                         # Game assets (PAK files, etc.)
└── docs/                           # Documentation
```

---

## Subsystem Specifications

### 1. Core Subsystem (`src/core/`)

**Purpose**: Provide foundational utilities used by all other subsystems.

**Key Components**:

```cpp
// Types.h - Common type definitions
namespace hb {
    using u8  = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;
    using i8  = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;
    using f32 = float;
    using f64 = double;
}

// EventBus.h - Type-safe event system
namespace hb {
    template<typename Event>
    concept EventType = std::is_class_v<Event> && std::is_copy_constructible_v<Event>;

    class EventBus {
    public:
        template<EventType E>
        using Handler = std::function<void(const E&)>;

        template<EventType E>
        SubscriptionId subscribe(Handler<E> handler);

        template<EventType E>
        void publish(const E& event);

        void unsubscribe(SubscriptionId id);
    };
}

// Logger.h - Structured logging
namespace hb::log {
    enum class Level { Trace, Debug, Info, Warn, Error, Fatal };

    void init(std::string_view logFile);
    void setLevel(Level level);

    template<typename... Args>
    void trace(std::format_string<Args...> fmt, Args&&... args);
    // ... other levels
}
```

**C++20 Features Used**:
- `std::format` for type-safe formatting
- Concepts for type constraints
- `std::span` for non-owning views
- `std::source_location` for logging

---

### 2. Graphics Subsystem (`src/graphics/`)

**Purpose**: Handle all rendering operations with backend abstraction.

**Interface**:

```cpp
namespace hb::gfx {
    // Abstract renderer interface
    class IRenderer {
    public:
        virtual ~IRenderer() = default;

        virtual bool initialize(Window& window) = 0;
        virtual void shutdown() = 0;

        virtual void beginFrame() = 0;
        virtual void endFrame() = 0;

        virtual TextureHandle createTexture(const TextureDesc& desc) = 0;
        virtual void destroyTexture(TextureHandle handle) = 0;

        virtual void drawSprite(const SpriteDrawCall& call) = 0;
        virtual void drawText(const TextDrawCall& call) = 0;
        virtual void drawRect(const Rect& rect, Color color) = 0;
    };

    // Sprite with modern semantics
    class Sprite {
    public:
        void draw(IRenderer& renderer, Vec2 position,
                  std::optional<Color> tint = std::nullopt,
                  BlendMode blend = BlendMode::Alpha) const;

        [[nodiscard]] Vec2 size() const noexcept;
        [[nodiscard]] std::span<const Frame> frames() const noexcept;
    };
}
```

**Migration from DirectDraw**:
- Replace DirectDraw surfaces with Direct3D 11 textures
- Implement sprite batching for performance
- Use vertex buffers instead of direct surface blitting
- Maintain alpha blending compatibility (100%, 70%, 50%, 25%, 2%)

---

### 3. Audio Subsystem (`src/audio/`)

**Purpose**: Manage all sound effects and music playback.

**Interface**:

```cpp
namespace hb::audio {
    class AudioSystem {
    public:
        bool initialize();
        void shutdown();
        void update();

        // Sound effects
        SoundHandle loadSound(std::string_view path);
        void playSound(SoundHandle handle, const PlayParams& params = {});
        void stopSound(SoundHandle handle);

        // Music
        void playMusic(std::string_view path, bool loop = true);
        void stopMusic(float fadeOutSeconds = 0.0f);

        // Volume control
        void setMasterVolume(float volume);
        void setSfxVolume(float volume);
        void setMusicVolume(float volume);
    };

    struct PlayParams {
        float volume = 1.0f;
        float pan = 0.0f;      // -1.0 (left) to 1.0 (right)
        bool loop = false;
        std::optional<Vec2> position;  // For 3D audio
    };
}
```

**Migration from DirectSound**:
- Replace DirectSound with XAudio2 (Windows) or SDL_mixer (cross-platform)
- Implement proper resource management with RAII
- Add audio streaming for music files

---

### 4. Input Subsystem (`src/input/`)

**Purpose**: Handle keyboard, mouse, and gamepad input with configurable bindings.

**Interface**:

```cpp
namespace hb::input {
    // Input actions (game-specific)
    enum class Action {
        MoveUp, MoveDown, MoveLeft, MoveRight,
        Attack, UseSkill, OpenInventory, OpenSpellbook,
        Chat, Screenshot, ToggleFullscreen,
        // ... etc
    };

    class InputSystem {
    public:
        void update();

        // Raw input state
        [[nodiscard]] bool isKeyDown(KeyCode key) const;
        [[nodiscard]] bool isKeyPressed(KeyCode key) const;  // Just pressed this frame
        [[nodiscard]] bool isKeyReleased(KeyCode key) const;

        [[nodiscard]] Vec2 mousePosition() const;
        [[nodiscard]] Vec2 mouseDelta() const;
        [[nodiscard]] float mouseWheel() const;
        [[nodiscard]] bool isMouseButtonDown(MouseButton btn) const;

        // Action-based input (for configurable bindings)
        [[nodiscard]] bool isActionActive(Action action) const;
        [[nodiscard]] bool isActionPressed(Action action) const;

        void bindAction(Action action, KeyCode key);
        void bindAction(Action action, MouseButton btn);
    };
}
```

**Migration from DirectInput**:
- Replace DirectInput with Raw Input API or SDL2
- Implement input binding system for user configuration
- Add support for multiple input devices

---

### 5. Network Subsystem (`src/network/`)

**Purpose**: Handle all server communication with the existing protocol.

**Interface**:

```cpp
namespace hb::net {
    // Connection state
    enum class ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Reconnecting,
        Failed
    };

    // Packet types (from existing protocol)
    enum class PacketType : u16 {
        // Login packets
        Login = 0x0001,
        LoginResponse = 0x0002,
        CharacterList = 0x0003,
        // ... (all existing packet types)
    };

    class NetworkSystem {
    public:
        bool initialize();
        void shutdown();
        void update();  // Process incoming/outgoing packets

        // Connection management
        std::future<bool> connectToLoginServer(std::string_view host, u16 port);
        std::future<bool> connectToGameServer(std::string_view host, u16 port);
        void disconnect();

        [[nodiscard]] ConnectionState loginServerState() const;
        [[nodiscard]] ConnectionState gameServerState() const;

        // Packet handling
        void sendPacket(const Packet& packet);

        template<typename Handler>
        void registerHandler(PacketType type, Handler&& handler);
    };

    // Type-safe packet builder
    class PacketBuilder {
    public:
        explicit PacketBuilder(PacketType type);

        PacketBuilder& write(u8 value);
        PacketBuilder& write(u16 value);
        PacketBuilder& write(u32 value);
        PacketBuilder& write(std::string_view str);
        PacketBuilder& write(std::span<const std::byte> data);

        [[nodiscard]] Packet build();
    };
}
```

**Migration Notes**:
- Maintain exact protocol compatibility with existing servers
- Use `std::async`/`std::future` for async operations
- Consider using `asio` for modern async networking
- Implement proper packet serialization with endianness handling

---

### 6. Asset Subsystem (`src/assets/`)

**Purpose**: Centralized asset loading and caching.

**Interface**:

```cpp
namespace hb::assets {
    // Asset handle with reference counting
    template<typename T>
    class AssetHandle {
    public:
        [[nodiscard]] T* get() const noexcept;
        [[nodiscard]] bool valid() const noexcept;
        explicit operator bool() const noexcept;
        T* operator->() const noexcept;
    };

    class AssetManager {
    public:
        bool initialize(std::filesystem::path assetRoot);
        void shutdown();

        // Synchronous loading
        AssetHandle<gfx::Texture> loadTexture(std::string_view path);
        AssetHandle<gfx::Sprite> loadSprite(std::string_view pakFile, u32 index);
        AssetHandle<audio::Sound> loadSound(std::string_view path);

        // Async loading
        std::future<AssetHandle<gfx::Texture>> loadTextureAsync(std::string_view path);

        // Cache management
        void preload(std::span<const std::string_view> paths);
        void releaseUnused();

        [[nodiscard]] AssetStats stats() const;
    };

    // PAK file reader
    class PakFile {
    public:
        static std::expected<PakFile, Error> open(const std::filesystem::path& path);

        [[nodiscard]] u32 entryCount() const;
        [[nodiscard]] std::expected<std::vector<std::byte>, Error> readEntry(u32 index);
    };
}
```

**Migration Notes**:
- Maintain PAK file format compatibility
- Implement lazy loading and LRU cache
- Support both synchronous and asynchronous loading

---

### 7. World Subsystem (`src/world/`)

**Purpose**: Manage map data, terrain, and world state.

**Interface**:

```cpp
namespace hb::world {
    // Tile data
    struct Tile {
        u16 terrainId;
        u16 objectId;
        TileFlags flags;

        [[nodiscard]] bool isWalkable() const noexcept;
        [[nodiscard]] bool isWater() const noexcept;
        [[nodiscard]] bool blocksVision() const noexcept;
    };

    class Map {
    public:
        static constexpr i32 TileWidth = 32;
        static constexpr i32 TileHeight = 32;
        static constexpr i32 VisibleTilesX = 25;  // From original (800/32)
        static constexpr i32 VisibleTilesY = 19;  // From original (600/32)

        [[nodiscard]] const Tile& at(i32 x, i32 y) const;
        [[nodiscard]] Vec2 worldToTile(Vec2 worldPos) const;
        [[nodiscard]] Vec2 tileToWorld(i32 tileX, i32 tileY) const;

        void setTile(i32 x, i32 y, const Tile& tile);
    };

    class World {
    public:
        void update(f32 deltaTime);
        void render(gfx::IRenderer& renderer);

        void loadMap(std::string_view mapName);

        [[nodiscard]] Map& currentMap();
        [[nodiscard]] Vec2 cameraPosition() const;

        void setCameraTarget(Vec2 target);
    };
}
```

---

### 8. Entity Subsystem (`src/entity/`)

**Purpose**: Manage all game entities (players, NPCs, monsters, items).

**Interface**:

```cpp
namespace hb::entity {
    using EntityId = u32;
    constexpr EntityId InvalidEntityId = 0;

    // Component-based entity (simplified ECS)
    class Entity {
    public:
        [[nodiscard]] EntityId id() const noexcept;
        [[nodiscard]] EntityType type() const noexcept;

        template<typename T>
        [[nodiscard]] T* getComponent();

        template<typename T>
        [[nodiscard]] const T* getComponent() const;

        template<typename T, typename... Args>
        T& addComponent(Args&&... args);

        template<typename T>
        void removeComponent();
    };

    class EntityManager {
    public:
        Entity& createEntity(EntityType type);
        void destroyEntity(EntityId id);

        [[nodiscard]] Entity* findEntity(EntityId id);
        [[nodiscard]] std::span<Entity* const> entitiesOfType(EntityType type);

        void update(f32 deltaTime);
        void render(gfx::IRenderer& renderer);

        // Query helpers
        [[nodiscard]] std::vector<Entity*> entitiesInRadius(Vec2 center, f32 radius);
        [[nodiscard]] Entity* entityAtPosition(Vec2 pos);
    };

    // Common components
    struct TransformComponent {
        Vec2 position;
        f32 rotation = 0.0f;
        Vec2 scale = {1.0f, 1.0f};
    };

    struct SpriteComponent {
        assets::AssetHandle<gfx::Sprite> sprite;
        i32 currentFrame = 0;
        Color tint = Color::White;
        gfx::BlendMode blendMode = gfx::BlendMode::Alpha;
    };

    struct AnimationComponent {
        i32 currentAnimation = 0;
        f32 frameTime = 0.0f;
        f32 frameRate = 10.0f;
        bool playing = true;
        bool loop = true;
    };
}
```

---

### 9. Gameplay Subsystem (`src/gameplay/`)

**Purpose**: Implement game mechanics and rules.

**Interface**:

```cpp
namespace hb::gameplay {
    // Game state machine
    enum class GameStateType {
        Login,
        CharacterSelect,
        Loading,
        Playing,
        Paused,
        Disconnected
    };

    class GameStateMachine {
    public:
        void update(f32 deltaTime);
        void render(gfx::IRenderer& renderer);

        void changeState(GameStateType newState);
        [[nodiscard]] GameStateType currentState() const;

        template<typename T>
        T& getState();
    };

    // Combat system
    class CombatSystem {
    public:
        void update(f32 deltaTime);

        void attack(entity::EntityId attacker, entity::EntityId target);
        void castSpell(entity::EntityId caster, SpellId spell, Vec2 target);
        void useSkill(entity::EntityId user, SkillId skill);

        [[nodiscard]] DamageResult calculateDamage(const AttackParams& params);
    };

    // Magic system
    class MagicSystem {
    public:
        [[nodiscard]] const SpellInfo& getSpellInfo(SpellId id) const;
        [[nodiscard]] bool canCast(entity::EntityId caster, SpellId spell) const;
        [[nodiscard]] std::span<const SpellId> availableSpells(entity::EntityId entity) const;
    };

    // Inventory system
    class InventorySystem {
    public:
        static constexpr i32 MaxInventorySlots = 50;
        static constexpr i32 EquipmentSlots = 15;

        [[nodiscard]] const Item* getItem(entity::EntityId entity, i32 slot) const;
        [[nodiscard]] const Item* getEquipped(entity::EntityId entity, EquipSlot slot) const;

        bool moveItem(entity::EntityId entity, i32 fromSlot, i32 toSlot);
        bool equipItem(entity::EntityId entity, i32 inventorySlot);
        bool unequipItem(entity::EntityId entity, EquipSlot slot);
        bool dropItem(entity::EntityId entity, i32 slot);
        bool useItem(entity::EntityId entity, i32 slot);
    };
}
```

---

### 10. UI Subsystem (`src/ui/`)

**Purpose**: Handle all user interface rendering and interaction.

**Interface**:

```cpp
namespace hb::ui {
    // UI event types
    struct ClickEvent { Vec2 position; MouseButton button; };
    struct HoverEvent { Vec2 position; };
    struct DragEvent { Vec2 start; Vec2 current; Vec2 delta; };
    struct TextInputEvent { std::string_view text; };

    // Base UI element
    class UIElement {
    public:
        virtual ~UIElement() = default;

        virtual void update(f32 deltaTime) {}
        virtual void render(gfx::IRenderer& renderer) = 0;
        virtual bool handleInput(const input::InputSystem& input) { return false; }

        void setPosition(Vec2 pos);
        void setSize(Vec2 size);
        void setVisible(bool visible);
        void setEnabled(bool enabled);

        [[nodiscard]] Rect bounds() const;
        [[nodiscard]] bool visible() const;
        [[nodiscard]] bool enabled() const;
        [[nodiscard]] bool containsPoint(Vec2 point) const;
    };

    // Dialog base class
    class Dialog : public UIElement {
    public:
        void open();
        void close();
        [[nodiscard]] bool isOpen() const;

        void setTitle(std::string_view title);
        void setDraggable(bool draggable);
    };

    // UI system manager
    class UISystem {
    public:
        void update(f32 deltaTime, const input::InputSystem& input);
        void render(gfx::IRenderer& renderer);

        template<typename T, typename... Args>
        T& createDialog(Args&&... args);

        void openDialog(DialogType type);
        void closeDialog(DialogType type);
        void toggleDialog(DialogType type);

        [[nodiscard]] Dialog* getDialog(DialogType type);
        [[nodiscard]] bool isDialogOpen(DialogType type) const;
    };
}
```

> **Note**: The dialog system has been modernized with a data-driven architecture supporting YAML definitions and hot-reload. See `docs/dialog_system.md` for comprehensive documentation and `docs/dialog_quick_reference.md` for a cheat sheet.

---

### 11. Chat Subsystem (`src/chat/`)

**Purpose**: Handle chat messaging, filtering, and history.

**Interface**:

```cpp
namespace hb::chat {
    enum class MessageType {
        Normal,
        Shout,
        Whisper,
        Guild,
        Party,
        System,
        GM
    };

    struct ChatMessage {
        MessageType type;
        std::string sender;
        std::string content;
        std::chrono::system_clock::time_point timestamp;
        Color color;
    };

    class ChatSystem {
    public:
        void update();

        void sendMessage(std::string_view content, MessageType type = MessageType::Normal);
        void sendWhisper(std::string_view target, std::string_view content);

        void addMessage(ChatMessage message);

        [[nodiscard]] std::span<const ChatMessage> recentMessages(size_t count = 50) const;
        [[nodiscard]] std::span<const ChatMessage> messagesByType(MessageType type) const;

        // Filtering
        void setFilterEnabled(bool enabled);
        [[nodiscard]] bool isFiltered(std::string_view content) const;
    };
}
```

---

### 12. Localization Subsystem (`src/localization/`)

**Purpose**: Runtime string localization (replacing compile-time #defines).

**Interface**:

```cpp
namespace hb::i18n {
    enum class Language {
        English,
        Korean,
        Japanese,
        ChineseSimplified,
        ChineseTraditional
    };

    class Localization {
    public:
        bool loadLanguage(Language lang);
        void setLanguage(Language lang);
        [[nodiscard]] Language currentLanguage() const;

        // Get localized string
        [[nodiscard]] std::string_view get(std::string_view key) const;

        // Formatted string
        template<typename... Args>
        [[nodiscard]] std::string format(std::string_view key, Args&&... args) const;
    };

    // Convenience macro/function
    #define _(key) hb::i18n::Localization::instance().get(key)
}
```

**Migration Notes**:
- Convert `lan_*.h` defines to JSON/YAML files
- Load strings at runtime instead of compile-time
- Support dynamic language switching

---

## C++20 Features to Utilize

### Language Features

| Feature | Use Case |
|---------|----------|
| **Concepts** | Type constraints for templates (e.g., `EventType`, `Component`) |
| **Modules** | (Future) Replace headers for faster compilation |
| **Ranges** | Collection processing without manual loops |
| **`std::format`** | Type-safe string formatting (logging, UI) |
| **`std::span`** | Non-owning array views |
| **`std::expected`** | Error handling without exceptions |
| **Coroutines** | Async operations (networking, asset loading) |
| **`constexpr`** | Compile-time computation |
| **`[[nodiscard]]`** | Prevent ignoring important return values |
| **`[[likely]]`/`[[unlikely]]`** | Branch prediction hints |
| **Three-way comparison** | Simplify comparison operators |
| **Designated initializers** | Cleaner struct initialization |

### Standard Library

| Component | Replaces |
|-----------|----------|
| `std::unique_ptr` | Raw `new`/`delete` |
| `std::shared_ptr` | Reference-counted objects |
| `std::vector` | Fixed-size arrays |
| `std::unordered_map` | Linear searches in arrays |
| `std::string`/`std::string_view` | `char[]` buffers |
| `std::filesystem` | Manual path handling |
| `std::chrono` | Windows timer APIs |
| `std::thread`/`std::jthread` | Windows thread APIs |
| `std::atomic` | Manual synchronization |

---

## Build System (CMake)

### CMakeLists.txt (Root)

```cmake
cmake_minimum_required(VERSION 3.20)
project(HelbreathClient VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(HB_BUILD_TESTS "Build unit tests" ON)
option(HB_USE_SDL "Use SDL2 backend instead of DirectX" OFF)

# Dependencies
find_package(fmt CONFIG REQUIRED)          # Until std::format is fully supported
find_package(spdlog CONFIG REQUIRED)       # Logging
find_package(nlohmann_json CONFIG REQUIRED) # JSON parsing

if(HB_USE_SDL)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(SDL2_mixer CONFIG REQUIRED)
endif()

# Subdirectories
add_subdirectory(src)

if(HB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

---

## Migration Strategy

### Phase 1: Foundation (Weeks 1-2)
1. Set up CMake build system
2. Create `src/core/` utilities (Types, Logger, Config)
3. Create `src/math/` types (Vec2, Rect, Color)
4. Create `src/platform/` abstraction (Window)
5. Ensure project compiles with C++20

### Phase 2: Graphics (Weeks 3-4)
1. Implement `IRenderer` interface
2. Create D3D11 backend (or SDL2)
3. Port `CSprite` to new `Sprite` class
4. Port sprite rendering with batching
5. Implement text rendering

### Phase 3: Input & Audio (Week 5)
1. Implement `InputSystem` with Raw Input
2. Implement `AudioSystem` with XAudio2
3. Port sound loading from PAK files

### Phase 4: Networking (Weeks 6-7)
1. Implement `Connection` class
2. Port packet protocol (maintain compatibility)
3. Implement `PacketBuilder`/`PacketReader`
4. Create message handlers

### Phase 5: Assets & World (Weeks 8-9)
1. Implement `AssetManager`
2. Port `PakFile` reader
3. Implement `Map` and `World` classes
4. Port map rendering

### Phase 6: Entities (Weeks 10-11)
1. Implement component system
2. Port player/character/monster entities
3. Implement `EntityManager`
4. Port entity rendering and animation

### Phase 7: Gameplay (Weeks 12-14)
1. Port combat system
2. Port magic/skill systems
3. Port inventory/equipment
4. Port guild/party systems

### Phase 8: UI (Weeks 15-17)
1. Implement UI widget system
2. Port all 41 dialog boxes
3. Implement chat UI
4. Polish and test UI interactions

### Phase 9: Integration & Polish (Weeks 18-20)
1. Full integration testing
2. Server protocol compatibility testing
3. Performance optimization
4. Bug fixing and polish

---

## Coding Standards

### Naming Conventions

```cpp
namespace hb {                    // lowercase namespace
    class ClassName {};           // PascalCase classes
    struct DataStruct {};         // PascalCase structs
    enum class EnumType {};       // PascalCase enums

    void functionName();          // camelCase functions
    void memberFunction();        // camelCase member functions

    int localVariable;            // camelCase locals
    int m_memberVariable;         // m_ prefix for members (or no prefix)
    static int s_staticVariable;  // s_ prefix for static members
    constexpr int kConstant = 0;  // k prefix for constants

    template<typename T>          // T prefix for type parameters
    concept SomeConcept = ...;    // PascalCase concepts
}
```

### File Organization

```cpp
// Header file (.h)
#pragma once

#include <standard_headers>
#include "project/headers.h"

namespace hb::subsystem {

class ClassName {
public:
    // Types
    // Constructors/Destructor
    // Public methods
    // Public static methods

private:
    // Private methods
    // Private members
};

} // namespace hb::subsystem

// Implementation file (.cpp)
#include "ClassName.h"

#include <additional_headers>

namespace hb::subsystem {

// Implementation

} // namespace hb::subsystem
```

### Error Handling

```cpp
// Prefer std::expected for recoverable errors
std::expected<Result, Error> tryOperation();

// Use exceptions only for truly exceptional cases
// Always document what exceptions a function might throw

// Use assertions for programmer errors (debug only)
HB_ASSERT(ptr != nullptr, "Pointer must not be null");
```

---

## Dependencies

### Required
- **CMake 3.20+**
- **C++20 Compiler** (MSVC 19.29+, GCC 11+, Clang 13+)
- **Windows SDK** (for D3D11, XAudio2)

### Recommended Libraries
- **fmt** - String formatting (fallback for std::format)
- **spdlog** - Fast logging
- **nlohmann/json** - JSON parsing
- **asio** - Async networking (standalone, non-Boost)
- **Catch2** or **GoogleTest** - Unit testing

### Optional
- **SDL2** - Cross-platform alternative
- **Dear ImGui** - Debug UI overlay

---

## Testing Strategy

### Unit Tests
- Test all core utilities
- Test packet serialization/deserialization
- Test game logic calculations (damage, skills)
- Test UI widget behavior

### Integration Tests
- Test asset loading pipeline
- Test network protocol against test server
- Test full game loop

### Manual Testing
- Visual verification of rendering
- Audio playback verification
- Server compatibility testing

---

## Notes for AI Assistants

When working on this codebase:

1. **Always use C++20 features** - Prefer modern alternatives to legacy patterns
2. **Maintain server protocol compatibility** - The network protocol must match existing servers exactly
3. **Follow subsystem boundaries** - Keep dependencies between subsystems minimal and explicit
4. **Write tests** - Add unit tests for new functionality
5. **Document public APIs** - Use Doxygen-style comments for public interfaces
6. **Avoid global state** - Pass dependencies explicitly
7. **Use RAII** - Never use raw `new`/`delete`
8. **Handle errors gracefully** - Use `std::expected` or exceptions appropriately
9. **Keep functions small** - Each function should do one thing well
10. **Preserve game behavior** - The modernized client should behave identically to the original
