# Configuration & Persistence System

## Overview

The legacy Helbreath client uses a mixed configuration system combining text-based config files, Windows Registry, and XOR-encrypted content files. Settings are scattered across multiple locations with different formats and persistence mechanisms.

**Key Characteristics:**
- Text-based config files with custom parsing
- Windows Registry for user preferences
- XOR-encrypted content filter files
- Compile-time language selection
- No unified configuration format

---

## Source Files

| File | Purpose |
|------|---------|
| `Game.cpp` | Config loading functions (bReadLoginConfigFile, bReadItemNameConfigFile, ReadSettings, WriteSettings) |
| `Game.h` | Config-related member variables |
| `GlobalDef.h` | Compile-time configuration constants |
| `ItemName.h` / `ItemName.cpp` | Item name database class |
| `Misc.cpp` | XOR file encryption/decryption |
| `GameMonitor.cpp` | Bad word list loading |
| `Curse.cpp` | Curse word filtering (language-specific) |

---

## Configuration Files

### 1. login.cfg - Server Connection

**Location:** Application root directory
**Format:** Text key-value pairs
**Function:** `CGame::bReadLoginConfigFile()` (Game.cpp:15526-15616)

#### Parameters

| Key | Type | Max Length | Description |
|-----|------|------------|-------------|
| `log-server-address` | String | 15 chars | Login server IP address |
| `log-server-port` | Integer | - | Login server port number |

#### File Format
```
log-server-address=192.168.1.100
log-server-port=2848
```

#### Parsing Details
- Separators: `= ,\t\n` (equals, space, comma, tab, newline)
- Uses `strtok()` for tokenization
- Two-pass parsing: reads key, then value

#### Member Variables
```cpp
char m_cLogServerAddr[16];    // Server IP (Game.h:859)
int  m_iLogServerPort;        // Server port (Game.h:780)
```

#### Compile-Time Overrides
The login.cfg file is bypassed for certain builds:

| Condition | Server Address | Port |
|-----------|----------------|------|
| `DEF_TESTSERVER` | 203.234.215.200 | 2848 |
| `DEF_JAPAN_FOR_TERRA` | 211.43.213.43 | 2849 |
| `DEF_LANGUAGE > 3` | Hardcoded per language | Varies |
| `DEF_LANGUAGE <= 3` | Read from login.cfg | Read from login.cfg |

#### Reading Process
```cpp
1. CreateFile() to get file size
2. fopen() in text read mode ("rt")
3. Allocate buffer (dwFileSize + 2 bytes)
4. fread() entire file
5. strtok() parsing loop
6. Validate IP <= 15 chars, port > 0
7. Cleanup buffer and close file
```

---

### 2. ItemName.cfg - Item Display Names

**Location:** `contents\ItemName.cfg`
**Format:** Text with equals-delimited fields
**Function:** `CGame::bReadItemNameConfigFile()` (Game.cpp:24707-24765)

#### Data Structure
```cpp
class CItemName {
public:
    char m_cOriginName[21];   // Internal/code item name
    char m_cName[34];         // Localized display name
};
```

#### Storage
```cpp
CItemName* m_pItemNameList[DEF_MAXITEMNAMES];  // 1000 items max
```

#### File Format
```
Item=InternalName=Display Name
Item=GoldCoin=Gold Coin
Item=SwordOfIce=Sword of Ice
```

#### Parsing State Machine
```
State 0: Looking for "Item" keyword
State 1, Mode 1: Read origin name -> advance to Mode 2
State 1, Mode 2: Read display name -> create entry, reset to State 0
```

#### Constants
```cpp
#define DEF_MAXITEMNAMES 1000   // Maximum item entries
```

---

### 3. MAGICCFG.TXT - Magic Configuration

**Location:** `contents\MAGICCFG.TXT`
**Function:** `CGame::bInitMagicCfgList()` (Game.h:443)

Loaded during initialization to configure the 100+ magic spells. See `13_magic_system.md` for detailed format.

---

### 4. SKILLCFG.TXT - Skill Configuration

**Location:** `contents\SKILLCFG.TXT`
**Function:** `CGame::bInitSkillCfgList()` (Game.h:428)

Loaded during initialization to configure the 60 skill types. See `14_skill_system.md` for detailed format.

---

## Encrypted Content Files

### XOR Encryption Scheme

**Function:** `CMisc::_iConvertFileXor()` (Misc.cpp:478-542)

#### Encryption Parameters
| Component | XOR Key | Description |
|-----------|---------|-------------|
| Header | `0x14` (20) | First 10 bytes, contains file size |
| Content | `0x23` (35) | Remaining bytes, actual content |

#### File Structure
```
[Header: 10 bytes XOR'd with 0x14] [Content: N bytes XOR'd with 0x23]
```

#### Decryption Process
```cpp
1. Read 10-byte header
2. XOR header with key 0x14
3. Convert decrypted header to integer (original file size)
4. Read remaining content
5. XOR each content byte with key 0x23
6. Validate decrypted size matches header
7. Write to temporary file
```

---

### 5. badword.txt - Profanity Filter

**Location:** `contents\badword.txt` (encrypted)
**Temp File:** `contents\badword.tmp`
**Function:** `CGameMonitor::iReadBadWordFileList()` (GameMonitor.cpp)

#### Loading Process (Game.cpp:995-1001)
```cpp
_iConvertFileXor("contents\\badword.txt", "contents\\badword.tmp", 35);
m_pGameMonitor->iReadBadWordFileList("contents\\badword.tmp");
DeleteFile("contents\\badword.tmp");
```

#### Data Structure
```cpp
class CGameMonitor {
    CMsg* m_pWordList[DEF_MAXBADWORD];  // 500 words maximum
};
```

#### File Format (Decrypted)
```
word1/word2,word3
word4	word5
```
Separators: `/,\t\n` (slash, comma, tab, newline)

#### Checking Logic (GameMonitor.cpp:66-83)
- Substring matching using `memcmp()`
- Checks all 500 words against input
- Case-sensitive comparison
- Returns on first match

---

### 6. Curse.txt - Language-Specific Filter

**Location:** `contents\Curse.txt` (encrypted)
**Condition:** Only loaded when `DEF_LANGUAGE > 2`
**Function:** `CCurse::LoadCurse()` (Curse.cpp)

#### Loading Process (Game.cpp:1004-1010)
```cpp
if (DEF_LANGUAGE > 2) {
    _iConvertFileXor("contents\\Curse.txt", "contents\\Curse.tmp", 35);
    m_curse.LoadCurse("contents\\Curse.tmp");
    DeleteFile("contents\\Curse.tmp");
}
```

#### Data Structure
```cpp
class CCurse {
    static char curse_string[MAX_CURSE_STRING];  // 7000 bytes
};
```

#### File Format (Decrypted)
- Comment characters: `$;\n`
- Tab-separated fields
- Supports Japanese multi-byte characters (Shift-JIS)

#### Advanced Filtering (Curse.cpp:97-156)
- Handles Hiragana, Katakana, Kanji
- Filters whitespace and punctuation
- Pattern-based matching for multiple word forms

---

## Windows Registry Settings

**Location:** `HKEY_CURRENT_USER\Software\Siementech\Helbreath\Settings`

### Functions
- **Read:** `CGame::ReadSettings()` (Game.cpp:320-381)
- **Write:** `CGame::WriteSettings()` (Game.cpp:383-438)

### Persisted Values

| Registry Key | Type | Valid Range | Member Variable | Description |
|--------------|------|-------------|-----------------|-------------|
| `Magic` | DWORD | 1-100 | `m_sMagicShortCut` | Last used magic spell |
| `ShortCut0` | DWORD | 1-200 | `m_sShortCut[0]` | Hotbar slot 1 |
| `ShortCut1` | DWORD | 1-200 | `m_sShortCut[1]` | Hotbar slot 2 |
| `ShortCut2` | DWORD | 1-200 | `m_sShortCut[2]` | Hotbar slot 3 |
| `ShortCut3` | DWORD | 1-200 | `m_sShortCut[3]` | Hotbar slot 4 |
| `ShortCut4` | DWORD | 1-200 | `m_sShortCut[4]` | Hotbar slot 5 |

### Storage Format
- Values stored as REG_DWORD (4 bytes)
- **Write:** `stored_value = internal_value + 1` (0-indexed to 1-indexed)
- **Read:** `internal_value = stored_value - 1` (1-indexed to 0-indexed)
- Invalid/missing values default to `-1`

### Member Variables (Game.h)
```cpp
short m_sMagicShortCut;     // Last used magic (line 761)
short m_sRecentShortCut;    // Recently used shortcut
short m_sShortCut[5];       // 5 configurable hotbar slots (line 773-774)
```

### When Called
- **ReadSettings():** In `CGame` constructor (Game.cpp:440-444)
- **WriteSettings():** On game exit or logout

---

## Runtime-Only Settings

These settings are NOT persisted between sessions:

### Audio Settings
```cpp
BOOL m_bSoundStat;          // Sound effects enabled
BOOL m_bMusicStat;          // Background music enabled
char m_cSoundVolume;        // SFX volume (0-100), default 100
char m_cMusicVolume;        // Music volume (0-100), default 100
```

**Volume Conversion:** `DirectSound_volume = (volume - 100) * 20`

### Display Settings
- Resolution locked to 800x600
- Color depth based on DirectDraw capabilities
- No persistent display preferences

### Dialog State
```cpp
struct {
    int   sV1...sV14;           // Integer values
    DWORD dwV1, dwV2, dwT1;     // Double-word values
    BOOL  bFlag;                // Boolean flag
    short sX, sY;               // Position
    short sSizeX, sSizeY;       // Size
    short sView;                // Current view/page
    char  cStr[32], cStr2[32];  // String buffers
    char  cStr3[32], cStr4[32]; // More string buffers
    char  cMode;                // Operation mode
    BOOL  bIsScrollSelected;    // Scroll state
} m_stDialogBoxInfo[41];        // All 41 dialog types
```

---

## Compile-Time Configuration

### Language Selection (GlobalDef.h)

```cpp
#define DEF_LANGUAGE 3   // Default: Korean

// Language codes:
// 1 = Taiwan (Traditional Chinese)
// 2 = China (Simplified Chinese)
// 3 = Korea
// 4 = English
// 5 = Japan
```

### Language-Specific Features

| Feature Define | TW | CN | KR | EN | JP |
|----------------|----|----|----|----|----|
| `DEF_USING_WIN_IME` | Yes | Yes | No | No | Yes |
| `DEF_ENGLISHITEM` | Yes | Yes | No | Yes | Yes |
| `DEF_SELECTSERVER` | No | Yes | Yes | Yes | Yes |
| `DEF_MAKE_ACCOUNT` | No | Yes | No | No | No |
| `DEF_FEEDBACKCARD` | No | Yes | No | No | No |
| `DEF_HTMLCOMMOM` | No | Yes | No | No | No |

### Version Numbers (GlobalDef.h)

| Language | DEF_UPPERVERSION | DEF_LOWERVERSION |
|----------|------------------|------------------|
| Taiwan | 2 | 19 |
| China | 2 | 19 |
| Korea | 2 | 20 |
| English | 2 | 20 |
| Japan | 2 | 20 |

---

## Command Line Parameters

**Parsing Location:** Game.cpp:815-851

### Global Variables
```cpp
char G_cCmdLine[256];        // Full command line
char G_cCmdLineTokenA[120];  // Token 1
char G_cCmdLineTokenB[120];  // Token 2
char G_cCmdLineTokenC[120];  // Token 3
char G_cCmdLineTokenD[120];  // Token 4
char G_cCmdLineTokenE[120];  // Token 5
```

### Separators
`&= ,\t\n` (ampersand, equals, space, comma, tab, newline)

### Special Handling
```cpp
// If token A starts with "/egparam", replace with "dataq"
if (strncmp(G_cCmdLineTokenA, "/egparam", 8) == 0) {
    strcpy(G_cCmdLineTokenA, "dataq");
}
```

---

## Initialization Sequence

**Order of configuration loading in `CGame::bInit()` (Game.cpp:815-901):**

```
1. CGame Constructor
   └── ReadSettings() - Load registry shortcuts

2. bInit()
   ├── _iCheckAndInitialize() - File integrity checks
   ├── _iDecodeBuildItemContents() - Decode crafting data
   ├── bReadLoginConfigFile() - Load login.cfg
   ├── bReadItemNameConfigFile() - Load ItemName.cfg
   ├── bInitMagicCfgList() - Load MAGICCFG.TXT
   └── bInitSkillCfgList() - Load SKILLCFG.TXT

3. Post-Init (Game.cpp:990-1014)
   ├── Create CGameMonitor
   ├── _iConvertFileXor() + iReadBadWordFileList() - badword.txt
   ├── _iConvertFileXor() + LoadCurse() - Curse.txt (if DEF_LANGUAGE > 2)
   └── _LoadGameMsgTextContents() - Load game messages
```

---

## File I/O Patterns

### Standard Text File Reading
```cpp
// 1. Get file size using Win32 API
HANDLE hFile = CreateFile(filename, GENERIC_READ, NULL, NULL,
                          OPEN_EXISTING, NULL, NULL);
DWORD dwFileSize = GetFileSize(hFile, NULL);
CloseHandle(hFile);

// 2. Read with C stdio
FILE* pFile = fopen(filename, "rt");
if (pFile == NULL) return FALSE;

char* pContents = new char[dwFileSize + 2];
ZeroMemory(pContents, dwFileSize + 2);
fread(pContents, dwFileSize, 1, pFile);
fclose(pFile);

// 3. Parse with strtok
char* token = strtok(pContents, "= ,\t\n");
while (token != NULL) {
    // Process token
    token = strtok(NULL, "= ,\t\n");
}

// 4. Cleanup
delete[] pContents;
```

### Registry Access Pattern
```cpp
// Read
HKEY key;
DWORD dwDisp;
RegCreateKeyEx(HKEY_CURRENT_USER,
               "Software\\Siementech\\Helbreath\\Settings",
               0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
               NULL, &key, &dwDisp);

DWORD value, size = sizeof(DWORD);
RegQueryValueEx(key, "Magic", 0, NULL, (LPBYTE)&value, &size);
RegCloseKey(key);

// Write
RegSetValueEx(key, "Magic", 0, REG_DWORD, (LPBYTE)&value, sizeof(DWORD));
```

---

## Error Handling

### Critical Initialization Errors

| Error Message | Cause | Recovery |
|---------------|-------|----------|
| `"File checksum error! Get Update again please!"` | File integrity check failed | Re-download client |
| `"login.cfg file contains wrong infomation."` | Missing/invalid login.cfg | Recreate file |
| `"ItemName.cfg file contains wrong infomation."` | Missing/invalid ItemName.cfg | Restore file |
| `"BADWORD.TXT file contains wrong infomation."` | Decryption failure | Restore file |
| `"CURSE.TXT file contains wrong infomation."` | Decryption failure | Restore file |
| `"This program requires DirectX7.0a!"` | DirectX init failed | Install DirectX 7+ |

### Error Return Codes
- Most config functions return `BOOL` (TRUE/FALSE)
- FALSE indicates failure, no detailed error info
- MessageBox used for user-facing errors

---

## Constants & Limits

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_MAXITEMNAMES` | 1000 | Maximum item name entries |
| `DEF_MAXBADWORD` | 500 | Maximum bad words |
| `MAX_CURSE_STRING` | 7000 | Curse string buffer size |
| Server IP max | 15 | Characters for IP address |
| Character name max | 12 | Characters per name |

---

## Summary Table

| Component | Location | Format | Persistence | Purpose |
|-----------|----------|--------|-------------|---------|
| Server Address | login.cfg | Text KV | File | Login server connection |
| Item Names | ItemName.cfg | Text delimited | File | Localized item display |
| Magic Config | MAGICCFG.TXT | Text | File | Spell definitions |
| Skill Config | SKILLCFG.TXT | Text | File | Skill definitions |
| Bad Words | badword.txt | XOR encrypted | File | Chat filter |
| Curse Words | Curse.txt | XOR encrypted | File | Regional filter |
| Shortcuts | Registry | DWORD | Registry | User hotbar |
| Audio Volume | Memory | - | None | Session only |
| Dialog State | Memory | Struct | None | Session only |

---

## Known Issues / Technical Debt

1. **No Input Validation:** Name lengths not validated, potential buffer overflows
2. **Weak Encryption:** XOR with fixed keys (0x14, 0x23) trivially reversible
3. **Registry Dependency:** Windows-specific, not portable
4. **Mixed File Formats:** Inconsistent parsing across config files
5. **No Error Recovery:** Config errors are fatal, no fallback values
6. **Hardcoded Paths:** `contents\` prefix hardcoded throughout
7. **Global State:** Uses global command line variables
8. **Memory Leaks:** Some error paths may not clean up allocations

---

## Modernization Notes

### Unified Configuration
- Replace all config files with single JSON/YAML format
- Use schema validation for type safety
- Support hot-reload for development

### Platform Independence
- Replace Windows Registry with cross-platform config file
- Use `std::filesystem` for path handling
- Store in user data directory (`~/.helbreath/` or `%APPDATA%`)

### Modern File I/O
```cpp
// Replace legacy pattern with:
auto loadConfig(const std::filesystem::path& path)
    -> std::expected<Config, ConfigError>
{
    std::ifstream file(path);
    if (!file) return std::unexpected(ConfigError::FileNotFound);

    auto json = nlohmann::json::parse(file, nullptr, false);
    if (json.is_discarded()) return std::unexpected(ConfigError::ParseError);

    return Config::from_json(json);
}
```

### Security Improvements
- Use proper encryption (AES) if data protection needed
- Sign configuration files for integrity verification
- Validate all inputs before use

### Suggested Config Structure
```yaml
# config.yaml
server:
  login_address: "192.168.1.100"
  login_port: 2848

audio:
  master_volume: 100
  sfx_volume: 100
  music_volume: 100
  sfx_enabled: true
  music_enabled: true

shortcuts:
  magic: 0
  slots: [-1, -1, -1, -1, -1]

display:
  fullscreen: false
  vsync: true
```
