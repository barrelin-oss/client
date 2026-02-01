# Combat System

## Overview

The Helbreath client combat system is a **server-authoritative** design where the client initiates attack commands and the server handles damage calculations and state management. The system features multiple combat modes, weapon-based attack types, range-based targeting, and faction-based friendly fire prevention.

The client's role is primarily:
- Validating attack feasibility (range, cooldown, weapon type)
- Sending attack requests to the server
- Rendering attack/damage animations based on server responses
- Displaying combat feedback (damage numbers, knockback)

## Source Files

| File | Purpose |
|------|---------|
| `Game.cpp` | Contains all combat logic (~lines 8691-10480 for rendering, ~35701+ for command processing) |
| `Game.h` | Combat state variables and function declarations |
| `ActionID.h` | Object action state definitions (attack, damage, dying, etc.) |
| `NetMessages.h` | Combat-related packet definitions |

## Key Data Structures

### Combat State Variables (Game.h)

```cpp
// Combat mode flags
BOOL m_bIsCombatMode;        // line 711 - PvP enabled
BOOL m_bIsSafeAttackMode;    // line 712 - Friendly fire prevention
BOOL m_bSuperAttackMode;     // line 716 - Extended range attacks
BOOL m_bForceAttack;         // line 735 - Override faction checks

// Attack tracking
DWORD m_dwCommandTime;       // line 672 - Last command timestamp (anti-speedhack)
BOOL m_bCommandAvailable;    // line 702 - Can accept new commands
int m_iSuperAttackLeft;      // Super attack points remaining

// Target information
short m_sMCX, m_sMCY;        // line 802 - Mouse cursor tile coordinates
short m_sCommX, m_sCommY;    // line 803 - Command target coordinates
WORD m_wCommObjectID;        // line 811 - Target object network ID

// Damage state
short m_sDamageMove;         // line 805 - Knockback distance in tiles
short m_sDamageMoveAmount;   // Actual HP damage value

// Player status flags
short m_sPlayerStatus;       // line 801 - Encoded status bits
int m_iPKCount;              // Player kill count (criminal status)
```

### Player Status Bit Flags

```cpp
0x8000  // PK status (criminal/red name)
0x4000  // Citizen flag
0x2000  // Aresden faction (vs Elvine)
0x1000  // Hunter status
```

### Object Action States (ActionID.h)

```cpp
#define DEF_OBJECTSTOP        0    // Idle/Standing
#define DEF_OBJECTMOVE        1    // Walking
#define DEF_OBJECTRUN         2    // Running
#define DEF_OBJECTATTACK      3    // Melee attack
#define DEF_OBJECTMAGIC       4    // Casting spell
#define DEF_OBJECTGETITEM     5    // Picking up item
#define DEF_OBJECTDAMAGE      6    // Taking damage
#define DEF_OBJECTDAMAGEMOVE  7    // Knockback animation
#define DEF_OBJECTATTACKMOVE  8    // Attack while moving (charge)
#define DEF_OBJECTDYING      10    // Death animation
#define DEF_OBJECTDEAD      101    // Corpse state
```

## Core Functions

### Attack Initiation

#### CommandProcessor() - line 35701
Main input handler for all combat commands.

```cpp
void CGame::CommandProcessor(...)
```

- Processes mouse clicks to determine attack targets
- Validates attack feasibility based on distance and weapon type
- Sets `m_cCommand` to combat action
- Sends request to server via `bSendCommand()`

**Cooldown Check (line 35916):**
```cpp
if ((dwTime - m_dwCommandTime) < 300) return;  // 300ms minimum between commands
```

### Attack Type Determination

#### _iGetAttackType() - line 23713
Returns attack code based on equipped weapon and skill mastery.

```cpp
int CGame::_iGetAttackType()
```

**Return Values:**
| Code | Attack Type |
|------|-------------|
| 1 | Normal melee attack |
| 2 | Bow attack (ranged) |
| 20 | Unarmed super attack |
| 21 | Blunt super attack |
| 22 | Dual axe super attack |
| 23 | Axe super attack |
| 24 | Spear super attack |
| 25 | Bow super attack |
| 26 | Guard super attack |
| 27 | Staff/Wand super attack |

**Super Attack Requirements:**
- `m_bSuperAttackMode == TRUE`
- `m_iSuperAttackLeft > 0`
- Weapon skill mastery >= 100

#### _iGetWeaponSkillType() - line 23769
Maps equipped weapon to skill ID.

```cpp
int CGame::_iGetWeaponSkillType()
```

**Skill Mappings:**
| Skill ID | Weapon Type |
|----------|-------------|
| 5 | Sword (Unarmed) |
| 6 | Bow |
| 7 | Blunt Weapon (Club) |
| 8 | Axe |
| 9 | Dual Axe |
| 10 | Spear |
| 14 | Shield/Guard |
| 21 | Staff/Wand |

### Friendly Fire Prevention

#### _iGetFOE() - line 24628
Determines if target is attackable based on faction/status.

```cpp
int CGame::_iGetFOE(short sStatus)
```

**Return Values:**
| Code | Meaning |
|------|---------|
| -2 | Target is PK (always attackable) |
| -1 | Cannot attack (friendly/faction) |
| 0 | Can attack (neutral/enemy) |
| 1 | Allied faction |

**Validation Logic:**
1. If attacker is PK (`m_iPKCount != 0`) → Can always attack
2. If target is PK → Always attackable
3. If target not citizen → Cannot attack
4. If attacker not citizen → Cannot attack (unless PK)
5. If crusade mode → Check faction alignment
6. If same faction → Blocked (unless force attack)
7. Hunter status → Special rules apply

### Damage Handling

#### NotifyMsg_HP() - line 43675
Processes HP change notifications from server.

```cpp
void CGame::NotifyMsg_HP(...)
```

- Minimum 10 HP change required to display healing messages
- Logs HP decrease events with timestamp for visual feedback

### Attack Rendering

#### DrawObject_OnAttack() - line 8691
Renders attacker in attack animation.

```cpp
void CGame::DrawObject_OnAttack(...)
```

- Parses equipment appearance bits (`sAppr1-4`)
- Extracts weapon type from `sAppr2 & 0x0FF0`
- Selects animation frame offset:
  - Frame +6: Attack with weapon (6 frames)
  - Frame +5: Idle with weapon
- Renders all sprite layers with directional rotation

#### DrawObject_OnDamage() - line 10480
Renders target receiving damage.

```cpp
void CGame::DrawObject_OnDamage(...)
```

- Applies knockback movement
- Color shift for invulnerability period
- Damage number display with magnitude-based colors

#### DrawObject_OnAttackMove() - line 9245
Renders charge attack (movement + attack combined).

```cpp
void CGame::DrawObject_OnAttackMove(...)
```

- Used for bow charge attacks
- Requires skill mastery 100
- Consumes stamina (SP)

## Constants & Limits

### Attack Range by Weapon

| Weapon Type | Normal Range | Super Range |
|-------------|--------------|-------------|
| Melee/Sword | 1 tile | 2 tiles |
| Blunt | 1 tile | 3 tiles |
| Axe | 1 tile | 3 tiles |
| Spear | 1 tile | 3 tiles |
| Bow | 4 tiles | 4 tiles |
| Staff | 2-3 tiles | 3 tiles |
| Guard | 2 tiles | 2 tiles |

### Timing Constants

```cpp
#define COMMAND_COOLDOWN 300          // ms between commands (anti-speedhack)
#define ATTACK_ANIMATION_DURATION 500 // approximate ms
```

### Damage Display Thresholds

| Damage Range | Font Type | Description |
|--------------|-----------|-------------|
| 0-11 | Type 21 | Small numbers |
| 12-39 | Type 22 | Medium numbers |
| 40+ | Type 23 | Large numbers |

### Animation Frame Indices

```cpp
#define ATTACK_IDLE_FRAME    5
#define ATTACK_FRAME_START   6
#define ATTACK_FRAME_END     7
#define ATTACK_MOVE_FRAME    8
#define DYING_FRAME          7
#define DEAD_FRAME          15
```

## Combat Modes

### Normal Combat Mode
- Toggled via `DEF_COMMONTYPE_TOGGLECOMBATMODE` (0x0A0B)
- `m_bIsCombatMode` flag
- When OFF: Cannot attack players (monsters still attackable)
- When ON: Full PvP enabled
- Auto-disabled if no weapon equipped

### Safe Attack Mode
- Toggled via `DEF_COMMONTYPE_TOGGLESAFEATTACKMODE` (0x0A18)
- `m_bIsSafeAttackMode` flag
- Prevents friendly fire on guild/party members
- Combined with FOE system for comprehensive protection

### Super Attack Mode
- Activated by holding **Alt key** during attack
- `m_bSuperAttackMode` flag
- Extended attack range (2-4 tiles instead of 1-2)
- Requires weapon skill mastery >= 100
- Consumes super attack points (`m_iSuperAttackLeft`)

### Force Attack Mode
- `m_bForceAttack` flag
- Allows attacking non-hostile players
- Overrides normal faction checks
- Toggle via UI button or command

## Network Protocol

### Attack Request

**MSGID_COMMAND_MOTION** (0x0FA314D5)
- Sent when player initiates attack
- Contains: direction, target position, attack type
- Server responds with `MSGID_RESPONSE_MOTION`

### Combat Events

**MSGID_EVENT_MOTION** (0x0FA314D7)
- Server broadcasts attack/damage events to all players in range
- Parsed in `MotionEventHandler()` (line 46077)
- Contains:
  - Attacker object ID
  - Target position (X, Y)
  - Attack type code
  - Direction facing
  - Damage values (`sV1` = damage, `sV2` = hit effect index)

### Combat Notifications

| Packet | ID | Purpose |
|--------|-----|---------|
| `DEF_NOTIFY_GLOBALATTACKMODE` | 0x0B73 | Global attack mode toggle |
| `DEF_NOTIFY_KILLED` | 0x0B09 | Player death notification |
| `DEF_NOTIFY_ENEMYKILLREWARD` | 0x0B1C | Kill reward (gold) |
| `DEF_NOTIFY_DAMAGEMOVE` | 0x0B74 | Knockback + damage amount |
| `DEF_NOTIFY_SUPERATTACKLEFT` | 0x0B52 | Remaining super attack points |

### Damage Move Packet Structure

```cpp
// DEF_NOTIFY_DAMAGEMOVE (0x0B74)
m_sDamageMove       // Knockback distance in tiles
m_sDamageMoveAmount // HP damage value
```

## Integration Points

### Input System
- Mouse clicks trigger `CommandProcessor()`
- Alt key activates super attack mode
- Ctrl+click for point attack

### Sprite System
- `DrawObject_OnAttack()` uses sprite rendering
- Weapon type determines animation offset
- Direction affects sprite selection

### Network System
- `bSendCommand()` sends attack packets
- `MotionEventHandler()` receives combat events
- Server-authoritative damage calculation

### UI System
- Event list displays combat messages (`m_stEventHistory[]`)
- Damage numbers rendered over targets
- Super attack gauge in icon panel

### Magic System
- Magic attacks use `DEF_OBJECTMAGIC` action
- Separate packet: `DEF_COMMONTYPE_MAGIC` (0x0A0D)
- Shares FOE system for targeting

## State Management

### Command Processing Flow

```
User clicks on target
       ↓
CommandProcessor() validates input
       ↓
Calculate distance (absX, absY)
       ↓
_iGetAttackType() determines attack code
       ↓
_iGetWeaponSkillType() checks weapon skill
       ↓
Validate range based on weapon
       ↓
_iGetFOE() checks friendly fire
       ↓
Set m_cCommand and m_sCommX/Y
       ↓
bSendCommand(MSGID_COMMAND_MOTION)
       ↓
Server validates and executes
       ↓
Server broadcasts MSGID_EVENT_MOTION
       ↓
MotionEventHandler() updates all clients
       ↓
DrawObject_OnAttack()/OnDamage() renders
```

### Client vs Server Responsibility

**Client Validates (UI/Feedback Only):**
- Attack range feasibility
- Weapon equipped status
- Cooldown timer (300ms)
- Skill mastery level display
- Target orientation

**Server Validates & Executes:**
- Actual hit/miss calculation
- Damage amount calculation
- Target status (alive, invulnerable)
- Friendly fire prevention (final check)
- Experience/reward distribution
- Death/kill announcements

## Equipment Appearance Encoding

### m_sPlayerAppr2 (Weapon/Shield)

```cpp
Bits 15-12: Reserved
Bits 11-4:  Weapon type (0-59)
  0:        Unarmed
  1-2:      Blunt weapons
  3-19:     Swords/Axes
  20-29:    Spears
  30-34:    Shields
  35-39:    Staves/Wands
  40-59:    Bows
Bits 3-0:   Shield type
```

### Object Type Classifications

| Type Range | Entity Type |
|------------|-------------|
| 1-6 | Player characters (PvP targets) |
| 7-9 | Special players (observers, GM) |
| 10-29 | NPCs (not attackable) |
| 30+ | Monsters (always attackable) |

## Known Issues / Technical Debt

1. **Monolithic Structure**: All combat logic embedded in 48k+ line CGame class
2. **Magic Numbers**: Attack codes (1-27), status bits hardcoded throughout
3. **Client-Side Validation**: Some checks duplicated between client/server
4. **Animation Coupling**: Combat rendering tightly coupled to game logic
5. **No Combat Events**: No event system for combat state changes
6. **Hardcoded Ranges**: Weapon ranges embedded in conditionals, not configurable
7. **Mixed Concerns**: FOE checks combine faction, guild, party, PK logic

## Modernization Notes

### Recommended Architecture

```cpp
namespace hb::combat {
    class CombatSystem {
    public:
        void update(float deltaTime);

        // Attack initiation
        AttackResult tryAttack(EntityId attacker, EntityId target);
        AttackResult tryAttack(EntityId attacker, Vec2 position);

        // Mode management
        void setCombatMode(bool enabled);
        void setSafeAttackMode(bool enabled);
        void setSuperAttackMode(bool enabled);
        void setForceAttackMode(bool enabled);

        // Queries
        [[nodiscard]] bool canAttack(EntityId target) const;
        [[nodiscard]] int getAttackRange() const;
        [[nodiscard]] AttackType getAttackType() const;

    private:
        bool validateAttack(EntityId target) const;
        FoeStatus checkFoeStatus(EntityId target) const;
    };

    enum class AttackType : uint8_t {
        Melee = 1,
        Ranged = 2,
        // Super attacks 20-27
        SuperUnarmed = 20,
        SuperBlunt = 21,
        // ...
    };

    enum class FoeStatus {
        Enemy,      // Can attack
        Friendly,   // Cannot attack (same faction)
        Criminal,   // Always attackable (PK)
        Neutral     // Depends on mode
    };
}
```

### Key Improvements

1. **Separate Combat System**: Extract from CGame into dedicated class
2. **Event-Based**: Publish attack/damage events for UI and other systems
3. **Data-Driven**: Load weapon ranges/types from configuration
4. **Strong Typing**: Use enums for attack types, status flags
5. **Clean Interface**: Clear separation between validation and execution
6. **Testable**: Combat logic can be unit tested independently
