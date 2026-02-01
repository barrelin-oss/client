# Crafting System

## Overview

The Helbreath crafting system allows players to combine ingredients to create new items. It uses a recipe-based approach where each craftable item requires up to 6 specific ingredients in defined quantities. Success is determined by the player's Manufacturing skill level compared to the recipe's difficulty.

## Source Files

| File | Purpose |
|------|---------|
| `BuildItem.h` | CBuildItem class definition for recipe data |
| `BuildItem.cpp` | CBuildItem constructor/destructor implementation |
| `Game.cpp` | Recipe loading, availability checking, UI rendering, crafting execution |
| `Game.h` | BuildItem list arrays and dialog info storage |
| `NetMessages.h` | Network message IDs for crafting requests |

## Key Data Structures

### CBuildItem Class

The core data structure representing a single crafting recipe:

```cpp
class CBuildItem
{
public:
    CBuildItem();
    virtual ~CBuildItem();

    // Recipe state
    BOOL m_bBuildEnabled;           // TRUE when all ingredients available

    // Output item
    char m_cName[21];               // Name of crafted item (20 chars max)

    // Skill requirements
    int m_iSkillLimit;              // Minimum skill level to see/craft recipe
    int m_iMaxSkill;                // Success rate ceiling (0-100%)

    // Sprite display
    int m_iSprH;                    // Sprite handle/index
    int m_iSprFrame;                // Sprite frame number

    // 6 ingredient slots - names
    char m_cElementName1[21];
    char m_cElementName2[21];
    char m_cElementName3[21];
    char m_cElementName4[21];
    char m_cElementName5[21];
    char m_cElementName6[21];

    // 6 ingredient slots - quantities
    DWORD m_iElementCount[7];       // [1]-[6] for ingredients, [0] unused

    // Ingredient availability tracking
    BOOL m_bElementFlag[7];         // TRUE if ingredient found in inventory
};
```

### Member Variables Explained

| Member | Type | Description |
|--------|------|-------------|
| `m_bBuildEnabled` | BOOL | Set TRUE when all 6 ingredients are present in inventory |
| `m_cName` | char[21] | The output item name this recipe creates |
| `m_iSkillLimit` | int | Minimum Manufacturing skill (0-100) to access this recipe |
| `m_iMaxSkill` | int | Maximum success percentage; caps success rate calculation |
| `m_iSprH` | int | Sprite handle index for recipe preview display |
| `m_iSprFrame` | int | Sprite frame number for recipe preview |
| `m_cElementName1-6` | char[21] | Names of required ingredients (empty string = optional) |
| `m_iElementCount[1-6]` | DWORD | Quantity required of each ingredient (0 = optional) |
| `m_bElementFlag[1-6]` | BOOL | Runtime flag: TRUE if ingredient available in inventory |

### CGame Recipe Storage

```cpp
class CGame {
    // All loaded recipes from config file (max 100)
    class CBuildItem * m_pBuildItemList[DEF_MAXBUILDITEMS];

    // Filtered list of recipes player can currently craft
    class CBuildItem * m_pDispBuildItemList[DEF_MAXBUILDITEMS];
};
```

### Dialog State Storage

Dialog box index 26 (Skill/Crafting dialog) stores crafting state:

```cpp
m_stDialogBoxInfo[26].sView       // Scroll position in recipe list
m_stDialogBoxInfo[26].cStr[0]     // Selected recipe index
m_stDialogBoxInfo[26].sV1         // Ingredient slot 1: inventory item index
m_stDialogBoxInfo[26].sV2         // Ingredient slot 2: inventory item index
m_stDialogBoxInfo[26].sV3         // Ingredient slot 3: inventory item index
m_stDialogBoxInfo[26].sV4         // Ingredient slot 4: inventory item index
m_stDialogBoxInfo[26].sV5         // Ingredient slot 5: inventory item index
m_stDialogBoxInfo[26].sV6         // Ingredient slot 6: inventory item index
m_stDialogBoxInfo[26].cStr[4]     // Craft eligibility flag (1 = can craft)
m_stDialogBoxInfo[26].cMode       // Dialog display mode (3, 4, or 5)
```

## Constants & Limits

```cpp
#define DEF_MAXBUILDITEMS       100     // Maximum recipe definitions
#define DEF_MAXITEMS            50      // Player inventory slots
```

| Constant | Value | Description |
|----------|-------|-------------|
| Max Recipes | 100 | Maximum buildable item definitions |
| Ingredient Slots | 6 | Fixed number of ingredient slots per recipe |
| Name Length | 20 | Maximum characters for item/ingredient names |
| Skill Range | 0-100 | Skill requirement and mastery scale |
| Success Cap | 0-100% | Maximum success rate percentage |

## Core Functions

### Recipe Loading

#### `_bDecodeBuildItemContents()`

**Location:** Game.cpp:23978

Loads all crafting recipes from the configuration file.

```cpp
BOOL CGame::_bDecodeBuildItemContents()
```

**Process:**
1. Clears existing `m_pBuildItemList` entries (deletes and nulls)
2. Opens `contents\BItemcfg.txt`
3. Reads entire file into memory buffer
4. Calls `__bDecodeBuildItemContents(pBuffer)` for parsing
5. Cleans up buffer and returns success/failure

**Returns:** TRUE on success, FALSE on file read or parse error

#### `__bDecodeBuildItemContents(char* pData)`

**Location:** Game.cpp:24271

Parses the recipe configuration buffer using a state machine.

**State Variables:**
- `cReadModeA`: Outer state (0 = idle, 1 = reading BuildItem)
- `cReadModeB`: Inner state (1-17, one for each field)

**Parsing Sequence (cReadModeB values):**

| State | Field |
|-------|-------|
| 1 | Recipe name |
| 2 | Skill requirement (m_iSkillLimit) |
| 3 | Ingredient 1 name |
| 4 | Ingredient 1 count |
| 5 | Ingredient 2 name |
| 6 | Ingredient 2 count |
| 7 | Ingredient 3 name |
| 8 | Ingredient 3 count |
| 9 | Ingredient 4 name |
| 10 | Ingredient 4 count |
| 11 | Ingredient 5 name |
| 12 | Ingredient 5 count |
| 13 | Ingredient 6 name |
| 14 | Ingredient 6 count |
| 15 | Sprite handle (m_iSprH) |
| 16 | Sprite frame (m_iSprFrame) |
| 17 | Max skill percentage (m_iMaxSkill) |

### Availability Checking

#### `_bCheckBuildItemStatus()`

**Location:** Game.cpp:24026

Filters recipes based on player skill and ingredient availability.

```cpp
void CGame::_bCheckBuildItemStatus()
```

**Process:**
1. Clears `m_pDispBuildItemList` (sets all to NULL)
2. Creates local `iItemCount[DEF_MAXITEMS]` to track ingredient usage
3. For each recipe in `m_pBuildItemList`:
   - Skip if player skill < `m_iSkillLimit`
   - Check each of 6 ingredients against inventory
   - Set `m_bElementFlag[1-6]` for found ingredients
   - Set `m_bBuildEnabled = TRUE` only if ALL ingredients match
4. Copy matching recipes to `m_pDispBuildItemList`

**Skill Reference:** Manufacturing skill stored in `m_cSkillMastery[13]`

**Note:** Uses labeled goto statements (CBIS_STEP1 through CBIS_STEP7) for ingredient checking flow control.

#### `_bCheckCurrentBuildItemStatus()`

**Location:** Game.cpp:24410

Validates if the currently selected recipe can be crafted with assigned items.

```cpp
BOOL CGame::_bCheckCurrentBuildItemStatus()
```

**Input:** Reads from dialog info:
- `m_stDialogBoxInfo[26].cStr[0]` - Selected recipe index
- `m_stDialogBoxInfo[26].sV1-sV6` - Inventory indices for 6 ingredient slots

**Returns:** TRUE if:
- All 6 ingredients match recipe requirements
- All required slots are filled (no -1 values for needed ingredients)
- Quantities match exactly

**Used:** Called when player clicks "Manufacture" button before sending network request.

## Configuration File Format

**File:** `contents\BItemcfg.txt`

**Format:**
```
BuildItem = <name>, <skill_limit>, <ing1>, <cnt1>, <ing2>, <cnt2>, <ing3>, <cnt3>, <ing4>, <cnt4>, <ing5>, <cnt5>, <ing6>, <cnt6>, <spr_h>, <spr_frame>, <max_skill>
```

**Delimiters:** `= , \t\n` (equals, comma, space, tab, newline)

**Example:**
```
BuildItem = Healing Potion, 20, Herb, 3, Spring Water, 2, Crystal Dust, 1, Empty Bottle, 1, , 0, , 0, 6, 45, 80
```

**Breakdown:**
| Field | Value | Description |
|-------|-------|-------------|
| Item Name | "Healing Potion" | Output item created |
| Min Skill | 20 | Requires Manufacturing level 20+ |
| Ingredient 1 | "Herb" x3 | 3 Herbs required |
| Ingredient 2 | "Spring Water" x2 | 2 Spring Waters required |
| Ingredient 3 | "Crystal Dust" x1 | 1 Crystal Dust required |
| Ingredient 4 | "Empty Bottle" x1 | 1 Empty Bottle required |
| Ingredient 5 | (empty) | No 5th ingredient |
| Ingredient 6 | (empty) | No 6th ingredient |
| Sprite | Handle 6, Frame 45 | Display sprite |
| Max Skill | 80% | Success rate capped at 80% |

## UI Dialog System

### Dialog Box 26 (Skill/Crafting Dialog)

The crafting UI operates in multiple modes:

#### Mode 3: Recipe List

Displays scrollable list of available recipes:
- Up to 13 visible recipes at once
- Shows recipe name and max skill percentage
- Greyed out if `m_bBuildEnabled` is FALSE (missing ingredients)
- Highlights on mouse hover
- Clicking selects recipe and switches to Mode 4

#### Mode 4: Recipe Details (Craftable)

Shows selected recipe with full details:
- Recipe item sprite preview
- Ingredient list (greyed out if missing, normal if present)
- 6 ingredient drop slots for player to drag items
- "Back" button returns to Mode 3
- "Manufacture" button initiates crafting (only enabled if all ingredients placed)

#### Mode 5: Recipe Details (Insufficient Skill)

Shows error message when skill too low:
- Displays skill requirement message
- Buttons remain disabled
- Instructs player to increase Manufacturing skill

### UI Strings

From localization files:

```cpp
DRAW_DIALOGBOX_SKILLDLG7    = "Skill Level: %d/%d"
DRAW_DIALOGBOX_SKILLDLG8    = "Needed Materials:"
DRAW_DIALOGBOX_SKILLDLG15   = "Place the items and click"
DRAW_DIALOGBOX_SKILLDLG16   = "the Manufacture button to create"
DRAW_DIALOGBOX_SKILLDLG17   = "the item."
DRAW_DIALOGBOX_SKILLDLG18   = "You need to increase your crafting"
DRAW_DIALOGBOX_SKILLDLG19   = "skill level to manufacture"
DRAW_DIALOGBOX_SKILLDLG20   = "this item."
DRAW_DIALOGBOX_SKILLDLG29   = "Manufacturing the items...."
```

## Network Protocol

### Crafting Request

**Outbound Message:**
```cpp
bSendCommand(MSGID_COMMAND_COMMON, DEF_COMMONTYPE_REQ_CREATEPORTION,
             NULL, NULL, NULL, NULL, NULL);
```

**Message IDs:**
```cpp
#define MSGID_COMMAND_COMMON              0x0FA314DC
#define DEF_COMMONTYPE_REQ_CREATEPORTION  0x0A19
```

**Payload:** Includes recipe index and 6 inventory indices for ingredients

### Server Response

Server processes crafting and sends inventory update notification:
1. Validates ingredients exist in player inventory
2. Calculates success based on player skill vs recipe difficulty
3. On success: Removes ingredients, creates output item
4. On failure: Returns ingredients to inventory
5. Sends inventory update to client

### Related Message IDs

```cpp
#define MSGID_BUILDITEMCONFIGURATIONCONTENTS    0x0FA40002
    // Server sends crafting recipe configuration to client during login
```

## Integration Points

### Skill System

- **Skill Index:** 13 (Manufacturing/Crafting)
- **Skill Storage:** `m_cSkillMastery[13]` (0-100 scale)
- **Impact:**
  - Minimum level check: Recipe visible only if `m_cSkillMastery[13] >= m_iSkillLimit`
  - Success rate: Higher skill increases success chance (server-side calculation)

### Inventory System

- Ingredient checking scans `m_pItemList[DEF_MAXITEMS]`
- Each recipe ingredient must find matching item by name
- Item quantities verified against `m_iElementCount[1-6]`
- Successful craft consumes items from inventory

### UI System

- Uses Dialog Box 26 (shared with Skills dialog)
- Modes 3-5 specifically for crafting
- Item drag-drop from inventory to ingredient slots
- Visual feedback for ingredient availability

## State Management

### Recipe List Lifecycle

```
Game Initialization
    |
    v
_bDecodeBuildItemContents()
    |-> Reads contents\BItemcfg.txt
    |-> Populates m_pBuildItemList[0..99]
    |
    v
_bCheckBuildItemStatus()  (called when inventory changes or dialog opens)
    |-> Filters by player skill level
    |-> Checks ingredient availability
    |-> Populates m_pDispBuildItemList
    |
    v
Dialog Rendering (Mode 3)
    |-> Displays m_pDispBuildItemList
    |-> Player selects recipe
    |
    v
Dialog Rendering (Mode 4)
    |-> Player assigns items to 6 slots
    |-> _bCheckCurrentBuildItemStatus() validates
    |
    v
Craft Request
    |-> Sends MSGID_COMMAND_COMMON
    |-> Server processes
    |-> Inventory updated
```

### Ingredient Slot Assignment

Items are assigned to ingredient slots via drag-drop:
1. Player drags item from inventory
2. Drops on ingredient slot (slots 1-6)
3. `sV1-sV6` updated with inventory index
4. `_bCheckCurrentBuildItemStatus()` validates assignment
5. If all valid, "Manufacture" button enables

## Success/Failure Mechanics

**Important:** Success calculation is entirely server-side.

**Server-Side Logic (conceptual):**
```cpp
int success_chance = min(player_skill, recipe_max_skill_cap);
int random_roll = random(0, 100);

if (random_roll <= success_chance) {
    remove_ingredients_from_inventory();
    add_output_item_to_inventory();
    notify_success();
} else {
    // Ingredients may or may not be consumed on failure
    // (server implementation dependent)
    notify_failure();
}
```

**Client Behavior:**
1. Sends request and displays "Manufacturing the items...." message
2. Waits for server response
3. Receives inventory update notification
4. Updates UI based on new inventory state

## Known Issues / Technical Debt

### Memory Management
- Raw pointers for `m_pBuildItemList` and `m_pDispBuildItemList`
- Manual delete in destructor; no smart pointers
- Potential memory leaks if loading fails mid-parse

### Code Quality
- Uses goto statements for ingredient checking flow (CBIS_STEP1-7)
- Fixed-size char arrays instead of strings
- Magic numbers throughout (dialog indices, skill indices)
- No separation between data and UI logic

### Design Limitations
- Fixed 6 ingredient slots (cannot add more without code changes)
- Hardcoded 100 recipe limit
- Skill index 13 hardcoded throughout
- No recipe categories or filtering

### Coupling Issues
- Recipe checking tightly coupled to inventory system
- Dialog state stored in generic `m_stDialogBoxInfo` array
- UI rendering mixed with business logic in Game.cpp

## Modernization Notes

### Data Structure Improvements

```cpp
// Replace raw pointers with smart pointers
std::vector<std::unique_ptr<BuildItem>> m_buildItems;

// Replace char arrays with strings
struct BuildItem {
    std::string name;
    std::array<Ingredient, 6> ingredients;
    // ...
};

// Use std::optional for optional ingredients
struct Ingredient {
    std::optional<std::string> name;
    uint32_t count = 0;
};
```

### Type Safety

```cpp
// Strong typing for skill indices
enum class SkillType : uint8_t {
    Manufacturing = 13,
    // ...
};

// Enum for dialog modes
enum class CraftingDialogMode {
    RecipeList = 3,
    RecipeDetails = 4,
    InsufficientSkill = 5
};
```

### Separation of Concerns

```cpp
// Separate crafting logic from UI
class CraftingSystem {
public:
    std::span<const BuildItem> getAvailableRecipes(const PlayerStats& stats,
                                                    const Inventory& inv);
    bool canCraft(const BuildItem& recipe, const Inventory& inv);
    void requestCraft(const BuildItem& recipe,
                      std::span<const InventorySlot> ingredients);
};

// Separate UI rendering
class CraftingDialog : public Dialog {
    CraftingSystem& m_craftingSystem;
    // ...
};
```

### Configuration Loading

```cpp
// Use modern file parsing
std::expected<std::vector<BuildItem>, Error>
loadRecipes(const std::filesystem::path& configPath);

// Consider JSON or YAML for recipe definitions
{
    "recipes": [
        {
            "name": "Healing Potion",
            "skillRequired": 20,
            "maxSuccess": 80,
            "ingredients": [
                {"name": "Herb", "count": 3},
                {"name": "Spring Water", "count": 2}
            ]
        }
    ]
}
```

### Error Handling

```cpp
// Use std::expected for fallible operations
std::expected<void, CraftingError> loadRecipes(std::string_view path);

enum class CraftingError {
    FileNotFound,
    ParseError,
    InvalidRecipe,
    MissingIngredients,
    InsufficientSkill
};
```
