# Localization System

## Overview

The Helbreath client uses a **compile-time localization system** based on C preprocessor `#define` macros. Each supported language has its own header file containing all translatable strings as preprocessor definitions. Language selection is determined at compile time via the `DEF_LANGUAGE` macro in `GlobalDef.h`, meaning a separate client executable must be compiled for each language region.

This approach was typical for games of the 2002 era but has significant limitations compared to modern runtime localization systems.

---

## Source Files

| File | Description |
|------|-------------|
| `GlobalDef.h` | Language selection via `DEF_LANGUAGE` macro (1-5) |
| `lan_tai.h` | Traditional Chinese (Taiwan) - 1,731 lines |
| `lan_chi.h` | Simplified Chinese (China) - 1,860 lines |
| `lan_kor.h` | Korean - 1,869 lines |
| `lan_eng.h` | English - 1,873 lines |
| `lan_jap.h` | Japanese - 1,946 lines |

**Total localized strings:** ~9,279 lines across all language files

---

## Language Selection Mechanism

### DEF_LANGUAGE Values

```cpp
// GlobalDef.h
#define DEF_LANGUAGE 4  // Currently set to English

// Language codes:
// 1 = Taiwan (Traditional Chinese)
// 2 = China (Simplified Chinese)
// 3 = Korea
// 4 = English
// 5 = Japan
```

### Conditional Compilation

The language header is included based on `DEF_LANGUAGE`:

```cpp
#if DEF_LANGUAGE == 1      // Taiwan
    #include "lan_tai.h"
#elif DEF_LANGUAGE == 2    // China
    #include "lan_chi.h"
#elif DEF_LANGUAGE == 3    // Korea
    #include "lan_kor.h"
#elif DEF_LANGUAGE == 4    // English
    #include "lan_eng.h"
#elif DEF_LANGUAGE == 5    // Japan
    #include "lan_jap.h"
#endif
```

### Region-Specific Features

Each language region also enables different feature flags:

```cpp
#if DEF_LANGUAGE == 1      // Taiwan
    #define DEF_USING_WIN_IME      // Windows IME for input
    #define DEF_ENGLISHITEM        // English item names
    #define DEF_USING_GATEWAY      // Gateway server system
    #define DEF_SHORTCUT           // Shortcut key system
    #define DEF_GIZON              // Gizon currency system

#elif DEF_LANGUAGE == 2    // China
    #define DEF_USING_WIN_IME
    #define DEF_ENGLISHITEM
    #define DEF_SELECTSERVER       // Server selection screen
    #define DEF_MAKE_ACCOUNT       // Account creation
    #define DEF_SHORTCUT
    #define DEF_GIZON
    #define DEF_FEEDBACKCARD       // Feedback system
    #define DEF_HTMLCOMMOM         // HTML common features

#elif DEF_LANGUAGE == 3    // Korea
    #define DEF_SELECTSERVER
    #define DEF_GIZON

#elif DEF_LANGUAGE == 4    // English
    #define DEF_ENGLISHITEM
    #define DEF_SELECTSERVER

#elif DEF_LANGUAGE == 5    // Japan
    #define DEF_USING_WIN_IME
    #define DEF_SELECTSERVER
    #define DEF_ENGLISHITEM
    #ifdef DEF_JAPAN_FOR_TERRA
        #define DEF_ACCOUNTLONG    // Longer account names
        #define DEF_ACCOUNTLEN 16  // 16-char account limit
    #endif
#endif
```

### Version Numbers by Region

Each region may have different version numbers:

```cpp
#if DEF_LANGUAGE == 1      // Taiwan
    #define DEF_UPPERVERSION  2
    #define DEF_LOWERVERSION  19   // v2.19

#elif DEF_LANGUAGE == 2    // China
    #define DEF_UPPERVERSION  2
    #define DEF_LOWERVERSION  19   // v2.19

#elif DEF_LANGUAGE == 3    // Korea
    #define DEF_UPPERVERSION  2
    #define DEF_LOWERVERSION  20   // v2.20

#elif DEF_LANGUAGE == 4    // English
    #define DEF_UPPERVERSION  2
    #define DEF_LOWERVERSION  20   // v2.20

#elif DEF_LANGUAGE == 5    // Japan
    #define DEF_UPPERVERSION  2
    #define DEF_LOWERVERSION  20   // v2.20
#endif
```

---

## String Categories

The localized strings are organized into functional categories:

### 1. Connection & Loading Screens (~50 strings)

```cpp
#define UPDATE_SCREEN_ON_CONNECTING1    "Press ESC key during long time of no"
#define UPDATE_SCREEN_ON_CONNECTING2    "connection and return to the main menu."
#define UPDATE_SCREEN_ON_CONNECTING3    "  Connecting to server. Please wait..."

#define UPDATE_SCREEN_ON_LOADING_PROGRESS1  "Loading game data."
#define UPDATE_SCREEN_ON_LOADING_PROGRESS2  "Please wait a moment."

#define UPDATE_SCREEN_ON_CONNECTION_LOST    "Connection Lost!"
```

### 2. Character Creation (~100 strings)

```cpp
#define UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER1   "Enter a character name."
#define UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER2   "Select character's gender."
#define UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER3   "Select character's skin."
// ... through UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER52

// Stat descriptions
#define UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER7   "Determine your character's initial"
#define UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER8   "strength assigned. As STR is"
#define UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER9   "increased, character's maximum"
#define UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER10  "HP and maximum stamina increases."
```

### 3. Account Management (~80 strings)

```cpp
#define UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT1   "Enter your account ID."
#define UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT2   "The account consists of english"
#define UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT3   "and numbers, no special keywords."
// ... through UPDATE_SCREEN_ON_CREATE_NEW_ACCOUNT82

#define UPDATE_SCREEN_ON_LOG_MSG1    "Account"
#define UPDATE_SCREEN_ON_LOG_MSG2    "Password"
// ... error messages, validation messages
```

### 4. Server Names (~16 strings)

```cpp
#define MSG_WORLDNAME1   "ABADDON Server"
#define MSG_WORLDNAME2   "APOCALYPSE Server"
#define MSG_WORLDNAME3   "3rd Server"
// ... through MSG_WORLDNAME16
```

Regional variations exist (Japanese Terra servers use different names):
```cpp
#ifdef DEF_JAPAN_FOR_TERRA
    #define MSG_WORLDNAME1  "PERSEUS Server"
    #define MSG_WORLDNAME2  "PEGASUS Server"
#endif
```

### 5. NPC Names (~80 strings)

```cpp
#define NPC_NAME_SLIME              "Slime"
#define NPC_NAME_SKELETON           "Skeleton"
#define NPC_NAME_STONEGOLEM         "Stone Golem"
#define NPC_NAME_CYCLOPS            "Cyclops"
#define NPC_NAME_ORC                "Orc"
#define NPC_NAME_SHOP_KEEPER        "Shop Keeper"
#define NPC_NAME_WAREHOUSE_KEEPER   "Warehouse Keeper"
#define NPC_NAME_GUARD              "Guard"
// ... many more monsters and NPCs
```

### 6. Map/Location Names (~64 strings)

```cpp
#define GET_OFFICIAL_MAP_NAME1   "Aresden Farm"
#define GET_OFFICIAL_MAP_NAME2   "Elvine Farm"
#define GET_OFFICIAL_MAP_NAME3   "Beginner Zone"
#define GET_OFFICIAL_MAP_NAME22  "Aresden"
#define GET_OFFICIAL_MAP_NAME24  "Elvine"
#define GET_OFFICIAL_MAP_NAME28  "Middleland"
// ... through GET_OFFICIAL_MAP_NAME64
```

### 7. Dialog Box Text (~500+ strings)

Prefixed by dialog type:

```cpp
// Bank dialog
#define DRAW_DIALOGBOX_BANK1   "Taking back items."
#define DRAW_DIALOGBOX_BANK2   "Please wait until process is finished."

// Character dialog
#define DRAW_DIALOGBOX_CHARACTER1  "Criminal (%d)"
#define DRAW_DIALOGBOX_CHARACTER2  "Contribution (%d)"

// Level-up settings
#define DRAW_DIALOGBOX_LEVELUP_SETTING1  "When level up, your specific stats"
#define DRAW_DIALOGBOX_LEVELUP_SETTING4  "Strength"
#define DRAW_DIALOGBOX_LEVELUP_SETTING5  "Vitality"

// City Hall menu
#define DRAW_DIALOGBOX_CITYHALL_MENU1   "Citizenship request."
#define DRAW_DIALOGBOX_CITYHALL_MENU4   "Take the Prize Gold."

// Guild menu
#define DRAW_DIALOGBOX_GUILDMENU1   "Make a new guild"
#define DRAW_DIALOGBOX_GUILDMENU4   "Break up your guild"

// Exchange dialog
#define DRAW_DIALOGBOX_EXCHANGE1   "My Item"
#define DRAW_DIALOGBOX_EXCHANGE5   "%s's item"

// Fishing dialog
#define DRAW_DIALOGBOX_FISHING1   "Value: %d Gold"
#define DRAW_DIALOGBOX_FISHING2   "Probability:"
```

### 8. Notification Messages (~100+ strings)

Server notification messages displayed to the player:

```cpp
#define NOTIFY_MSG_HANDLER1   "%s joined the party."
#define NOTIFY_MSG_HANDLER2   "%s withdrew from the party."
#define NOTIFY_MSG_HANDLER42  "Item manufacture success!"
#define NOTIFY_MSG_HANDLER43  "Failed on manufacturing item."
#define NOTIFY_MSG_HANDLER44  "Congratulations! You completed your quest!"
#define NOTIFY_MSG_HANDLER55  "You were successful on fishing!!!"
#define NOTIFY_MSG_HANDLER56  "You failed to fish..."
```

### 9. Item Attributes (~35 strings)

```cpp
#define GET_ITEM_NAME1   "Purity: %d%%"
#define GET_ITEM_NAME2   "Completion: %d%%"
#define GET_ITEM_NAME3   "Critical "
#define GET_ITEM_NAME4   "Poisoning "
#define GET_ITEM_NAME14  "Critical Hit Damage+%d"
#define GET_ITEM_NAME15  "Poison Damage+%d"
```

### 10. Hotkey & Input Messages (~50 strings)

```cpp
#define ON_KEY_UP1   " There is no item or magic for hotkey selected."
#define ON_KEY_UP2   " To Equip an item, weapon, or magic in the F2 Key"
#define ON_KEY_UP3   " and press [Control]-[F2]to set."

#define ON_KEY_UP4   "Item(%s %s %s) : set to [F2]"
#define ON_KEY_UP5   "Magic(%s) : set to [F2]"
```

### 11. Special Ability Messages (~30 strings)

```cpp
#define ON_KEY_UP29  "Ability that decreases enemy's HP by 50%: Can be used after %d sec"
#define ON_KEY_UP30  "Ability that freezes enemy: Can be used after %d sec"
#define ON_KEY_UP31  "Ability that paralyzes enemy: Can be used after %d sec"
#define ON_KEY_UP32  "Ability that kills enemy at one time: Can be used after %d sec"
```

### 12. Object/Entity Status Labels (~40 strings)

```cpp
#define DRAW_OBJECT_NAME50  " Berserked"
#define DRAW_OBJECT_NAME51  " Frozen"
#define DRAW_OBJECT_NAME52  "Clairvoyant"
#define DRAW_OBJECT_NAME60  "Traveller"
#define DRAW_OBJECT_NAME62  "Aresden"
#define DRAW_OBJECT_NAME74  "Elvine"
#define DRAW_OBJECT_NAME86  "Criminal,"
```

### 13. Quest System Text (~25 strings)

```cpp
#define NPC_TALK_HANDLER16  "Objective: slay %d %s"
#define NPC_TALK_HANDLER17  "Location : Anywhere"
#define NPC_TALK_HANDLER18  "Map : %s"
#define NPC_TALK_HANDLER23  "Contribution:%dPoints"
#define NPC_TALK_HANDLER35  "Would you try this quest?"
```

### 14. Chat Commands (~10 strings)

```cpp
#define BCHECK_LOCAL_CHAT_COMMAND1  "Character %s has been released from the message refusing list."
#define BCHECK_LOCAL_CHAT_COMMAND2  "You can't refuse your own messages."
#define BCHECK_LOCAL_CHAT_COMMAND6  "Enable to listen to whispers."
#define BCHECK_LOCAL_CHAT_COMMAND7  "Unalbe to listen to whispers."
```

---

## String Format Patterns

### Format Specifiers Used

| Specifier | Usage |
|-----------|-------|
| `%s` | String insertion (names, items) |
| `%d` | Integer values (counts, stats, gold) |
| `%d%%` | Percentage values (escaped percent sign) |
| `%d/%d` | Ratios (current/max values) |
| `%d/%d/%d` | Dates (Y/M/D format) |

### Multi-Line Text Pattern

Long messages are split across multiple numbered defines:

```cpp
#define DRAW_DIALOGBOX_CITYHALL_MENU18  "If you become a citizen, the level"
#define DRAW_DIALOGBOX_CITYHALL_MENU19  "restriction as a traveller is removed."
#define DRAW_DIALOGBOX_CITYHALL_MENU20  "You can buy and sell almost all items"
#define DRAW_DIALOGBOX_CITYHALL_MENU21  "which are produced in town. Also, by"
#define DRAW_DIALOGBOX_CITYHALL_MENU22  "repelling enemies of opposing city you"
#define DRAW_DIALOGBOX_CITYHALL_MENU23  "can get prize gold. You can also"
#define DRAW_DIALOGBOX_CITYHALL_MENU24  "join any guilds."
```

### Japanese Extended Defines

Japanese localization sometimes adds extra variants:

```cpp
#define UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER38    "..."
#define UPDATE_SCREEN_ON_CREATE_NEW_CHARACTER38_1  "..." // Additional line
```

---

## Character Encoding

### Per-Language Encoding

| Language | Encoding | Notes |
|----------|----------|-------|
| English | ASCII/Latin-1 | Standard 8-bit encoding |
| Korean | EUC-KR / CP949 | Korean Windows codepage |
| Japanese | Shift-JIS / CP932 | Japanese Windows codepage |
| Chinese (Simplified) | GB2312 / GBK | Simplified Chinese codepage |
| Chinese (Traditional) | Big5 | Traditional Chinese codepage |

### IME Support

Languages requiring Input Method Editors have `DEF_USING_WIN_IME` defined:
- Taiwan (Traditional Chinese)
- China (Simplified Chinese)
- Japan

This enables Windows IME integration for character input in chat and name fields.

---

## Usage in Code

### Direct String Usage

```cpp
// In Game.cpp drawing functions
DrawText(x, y, UPDATE_SCREEN_ON_CONNECTING1);
DrawText(x, y + 14, UPDATE_SCREEN_ON_CONNECTING2);
```

### Formatted String Usage

```cpp
// Using sprintf with format specifiers
char buffer[256];
sprintf(buffer, NOTIFY_MSG_HANDLER1, playerName);  // "%s joined the party."
DisplayMessage(buffer);

sprintf(buffer, GET_ITEM_NAME1, purityValue);      // "Purity: %d%%"
DrawItemTooltip(buffer);
```

### Conditional Language Code

```cpp
#if DEF_LANGUAGE == 2  // China-specific code
    // Chinese-specific handling
#endif
```

---

## Integration Points

### CGame Class

All localized strings are used directly in `Game.cpp` functions:
- `UpdateScreen_OnConnecting()`
- `UpdateScreen_OnCreateNewCharacter()`
- `UpdateScreen_OnLogin()`
- `DrawDialogBox_*()` (41 dialog drawing functions)
- `NotifyMsgHandler()`
- Various display and UI functions

### Text Rendering

The DirectDraw wrapper (`DXC_ddraw`) handles text rendering with font support for each character encoding.

---

## Constants & Limits

| Constant | Value | Purpose |
|----------|-------|---------|
| Max string length | ~256 chars | Typical buffer size for formatted strings |
| Languages supported | 5 | Taiwan, China, Korea, English, Japan |
| Total string defines | ~1,800+ | Per language file |
| Dialog categories | 41+ | Different dialog box types |

---

## Known Issues / Technical Debt

### Compile-Time Only
- Cannot switch languages at runtime
- Requires separate executable per language
- Increases build complexity for multi-region releases

### Duplicate/Inconsistent Strings
- Some strings are duplicated with slight variations
- Numbering gaps exist (e.g., `ON_KEY_UP20-25` may be missing)
- Inconsistent naming conventions

### Hardcoded Line Breaks
- Multi-line text is split into separate defines
- Character wrapping is manual, not automatic
- Different languages may need different line breaks due to text length

### Missing Translations
- Some English strings appear in non-English files (incomplete translation)
- Korean comments remain in code (`// Korean text here`)

### No Plural Handling
- English strings don't handle singular/plural properly
- Format strings assume fixed grammar structure

### Encoding Issues
- Source files may have mixed encodings
- No Unicode/UTF-8 support (era limitation)

---

## Modernization Notes

### Recommended Approach

1. **Runtime Localization**
   - Load strings from JSON/YAML files at runtime
   - Single executable supporting all languages
   - User can switch language in settings

2. **UTF-8 Encoding**
   - Convert all strings to UTF-8
   - Consistent encoding across all languages
   - Proper Unicode support

3. **String Key System**
   ```cpp
   // Instead of:
   DrawText(x, y, UPDATE_SCREEN_ON_CONNECTING1);

   // Use:
   DrawText(x, y, i18n::get("screen.connecting.line1"));
   ```

4. **Format String Safety**
   - Use `std::format` or `fmt::format` instead of `sprintf`
   - Type-safe formatting

5. **Plural Handling**
   - ICU message format or similar
   - Proper pluralization rules per language

6. **Automatic Text Wrapping**
   - Calculate line breaks based on font metrics
   - Handle variable-width fonts properly

### Suggested File Structure

```
localization/
├── strings/
│   ├── en.json
│   ├── ko.json
│   ├── ja.json
│   ├── zh_cn.json
│   └── zh_tw.json
└── Localization.cpp/h
```

### Example JSON Format

```json
{
  "screen": {
    "connecting": {
      "line1": "Press ESC key during long time of no",
      "line2": "connection and return to the main menu.",
      "waiting": "Connecting to server. Please wait..."
    }
  },
  "npc": {
    "slime": "Slime",
    "skeleton": "Skeleton"
  },
  "notify": {
    "party_join": "{name} joined the party.",
    "party_leave": "{name} withdrew from the party."
  }
}
```

---

## String Count Summary

| Category | Approximate Count |
|----------|-------------------|
| Connection/Loading | ~50 |
| Character Creation | ~100 |
| Account Management | ~80 |
| Server Names | ~16 |
| NPC/Monster Names | ~80 |
| Map Names | ~64 |
| Dialog Text | ~500+ |
| Notifications | ~100+ |
| Item Attributes | ~35 |
| Input/Hotkey | ~50 |
| Special Abilities | ~30 |
| Object Status | ~40 |
| Quest System | ~25 |
| Chat Commands | ~10 |
| Miscellaneous | ~200+ |
| **Total per language** | **~1,800+** |
