# Message Handlers

## Overview

The Helbreath client uses a message-based protocol to communicate with two server types: the **Login Server** (authentication, character management) and the **Game Server** (gameplay, world state). All server communication flows through message handlers that parse incoming packets and dispatch them to appropriate processing logic within the monolithic `CGame` class.

This document covers all packet types, handler functions, payload structures, and protocol quirks critical for server compatibility during modernization.

## Source Files

| File | Purpose |
|------|---------|
| `Game.cpp` | All message handler implementations (lines 2936-46077+) |
| `Game.h` | Handler function declarations |
| `NetMessages.h` | Protocol constants, message IDs, and type definitions |
| `XSocket.h/.cpp` | Socket wrapper, packet queuing, receive/send logic |

## Packet Structure Format

### Header Layout (6 bytes minimum)

```
Offset 0-3: Message ID (DWORD)  - DEF_INDEX4_MSGID = 0
Offset 4-5: Message Type (WORD) - DEF_INDEX2_MSGTYPE = 4
Offset 6+:  Payload data (variable length)
```

### Key Constants

```cpp
#define DEF_INDEX4_MSGID    0    // Message ID at offset 0 (4 bytes)
#define DEF_INDEX2_MSGTYPE  4    // Message Type at offset 4 (2 bytes)
```

### Packet Transmission Flow

**Receive Flow (XSocket):**
1. Read 1 byte confirmation code + WORD packet type (header)
2. Determine packet size from header or known protocol definition
3. Read remaining bytes into buffer (body)
4. Validate confirmation code
5. Queue complete packet for game loop processing

**Send Flow:**
1. Build packet: MSGID + Type + Payload
2. If socket blocked, queue packet (max 300 entries via `DEF_XSOCKBLOCKLIMIT`)
3. Direct send if socket available
4. Process unsent queue on next socket event

## Two-Server Architecture

### Login Server

- **Purpose**: Authentication, character selection, account management
- **Socket Member**: `m_pLSock`
- **Entry Point**: `LogRecvMsgHandler()` → `LogResponseHandler()`

### Game Server

- **Purpose**: In-game events, movement, combat, items, guilds, etc.
- **Socket Member**: `m_pGSock`
- **Entry Point**: `GameRecvMsgHandler()`

---

## Game Server Message Handlers

### Primary Dispatcher (GameRecvMsgHandler)

**Location**: Game.cpp:2936

| Message ID | Handler Function | Purpose |
|------------|------------------|---------|
| `MSGID_RESPONSE_INITPLAYER` | `InitPlayerResponseHandler()` | Player initialization after game server connect |
| `MSGID_RESPONSE_INITDATA` | `InitDataResponseHandler()` | Initial game data (items, magic, skills, NPCs, quests) |
| `MSGID_RESPONSE_MOTION` | `MotionResponseHandler()` | Response to motion/movement commands |
| `MSGID_EVENT_MOTION` | `MotionEventHandler()` | Motion events from other entities |
| `MSGID_EVENT_COMMON` | `CommonEventHandler()` | Common game events (items, magic effects) |
| `MSGID_EVENT_LOG` | `LogEventHandler()` | Login-related events |
| `MSGID_COMMAND_CHATMSG` | `ChatMsgHandler()` | Chat messages from players |
| `MSGID_NOTIFY` | `NotifyMsgHandler()` | Server notifications (100+ types) |
| `MSGID_PLAYERITEMLISTCONTENTS` | `InitItemList()` | Player inventory list |
| `MSGID_PLAYERCHARACTERCONTENTS` | `InitPlayerCharacteristics()` | Player stats/characteristics |
| `MSGID_RESPONSE_CREATENEWGUILD` | `CreateNewGuildResponseHandler()` | Guild creation response |
| `MSGID_RESPONSE_DISBANDGUILD` | `DisbandGuildResponseHandler()` | Guild disband response |
| `MSGID_RESPONSE_CIVILRIGHT` | `CivilRightAdmissionHandler()` | Civil rights response |
| `MSGID_RESPONSE_RETRIEVEITEM` | `RetrieveItemHandler()` | Item retrieval (bank/storage) |
| `MSGID_RESPONSE_PANNING` | `ResponsePanningHandler()` | Map panning response |
| `MSGID_RESPONSE_FIGHTZONE_RESERVE` | `ReserveFightzoneResponseHandler()` | Fight zone reservation |
| `MSGID_DYNAMICOBJECT` | `DynamicObjectHandler()` | Dynamic object updates |
| `MSGID_RESPONSE_NOTICEMENT` | `NoticementHandler()` | In-game announcements |
| `MSGID_RESPONSE_TELEPORT_LIST` | `ResponseTeleportList()` | Teleport location list |
| `MSGID_RESPONSE_CHARGED_TELEPORT` | `ResponseChargedTeleport()` | Teleport confirmation |

---

## Common Event Handler (MSGID_EVENT_COMMON)

**Location**: Game.cpp:4833

### Packet Structure

```
Bytes 0-3:   MSGID_EVENT_COMMON
Bytes 4-5:   Event Type (WORD) - wEventType
Bytes 6-7:   X coordinate (short)
Bytes 8-9:   Y coordinate (short)
Bytes 10-11: Value 1 (short) - sV1
Bytes 12-13: Value 2 (short) - sV2
Bytes 14-15: Value 3 (short) - sV3
Bytes 16-17: Value 4 (short) - sV4
```

### Event Types

| Constant | Value | Action |
|----------|-------|--------|
| `DEF_COMMONTYPE_ITEMDROP` | 0x0A01 | Place dropped item on map at (X,Y), create effect if item type 6 |
| `DEF_COMMONTYPE_SETITEM` | 0x0A0C | Place/set item on map without drop effect |
| `DEF_COMMONTYPE_MAGIC` | 0x0A0D | Create magic effect at (X,Y); sV3=effect ID, sV1/sV2=target, sV4=param |
| `DEF_COMMONTYPE_CLEARGUILDNAME` | 0x0A25 | Clear cached guild names (initialize all 100 slots) |

---

## Notify Message Handler (MSGID_NOTIFY)

**Location**: Game.cpp:29946

The largest handler, processing 100+ notification types from the server.

### Packet Structure

```
Bytes 0-3: MSGID_NOTIFY (0x0FA314D0)
Bytes 4-5: Notify Type (WORD) - wEventType
Bytes 6+:  Variable payload based on notify type
```

### Login/Account Notifications

| Type | Constant | Action |
|------|----------|--------|
| 0x0B43 | `DEF_NOTIFY_ADMINUSERLEVELLOW` | Admin level too low message |
| 0x0B38 | `DEF_NOTIFY_TRAVELERLIMITEDLEVEL` | Traveler level limit warning |

### Item Notifications

| Type | Constant | Payload | Action |
|------|----------|---------|--------|
| 0x0B01 | `DEF_NOTIFY_ITEMOBTAINED` | - | Item obtained |
| 0x0B06 | `DEF_NOTIFY_ITEMPURCHASED` | - | Item purchased |
| 0x0B19 | `DEF_NOTIFY_ITEMTOBANK` | - | Item moved to bank |
| 0x0B26 | `DEF_NOTIFY_CANNOTITEMTOBANK` | - | Cannot move to bank |
| 0x0B62 | `DEF_NOTIFY_CANNOTGIVEITEM` | - | Cannot give item |
| 0x0B2C | `DEF_NOTIFY_CANNOTSELLITEM` | - | Cannot sell item |
| 0x0B31 | `DEF_NOTIFY_ITEMSOLD` | - | Item sold |
| 0x0B2E | `DEF_NOTIFY_CANNOTREPAIRITEM` | - | Cannot repair |
| 0x0B30 | `DEF_NOTIFY_ITEMREPAIRED` | - | Item repaired |
| 0x0B25 | `DEF_NOTIFY_SETITEMCOUNT` | short: slot, short: count | Item count changed |
| 0x0BA3 | `DEF_NOTIFY_ITEMATTRIBUTECHANGE` | short: index, DWORD: attr, DWORD: spec1, DWORD: spec2 | Item attributes upgraded |
| 0x0BA8 | `DEF_NOTIFY_ITEMUPGRADEFAIL` | short: failure_type | Item upgrade failed |
| 0x0BA5 | `DEF_NOTIFY_GIZONEITEMCHANGE` | short: slot, char: type, WORD: lifespan, short: sprite, short: frame, char: color, char: effect, DWORD: attr, char[20]: name | Gizonitem properties |
| 0x0BA4 | `DEF_NOTIFY_GIZONITEMUPGRADELEFT` | short: upgrades_left, DWORD: detail | Upgrade count updated |
| 0x0B17 | `DEF_NOTIFY_ITEMLIFESPANEND` | - | Item lifespan expired |
| 0x0B1D | `DEF_NOTIFY_GIVEITEMFIN_COUNTCHANGED` | - | Item count after give |
| 0x0B1F | `DEF_NOTIFY_DROPITEMFIN_COUNTCHANGED` | - | Item count after drop |
| 0x0B20 | `DEF_NOTIFY_ITEMDEPLETED_ERASEITEM` | - | Item depleted/removed |
| 0x0B65 | `DEF_NOTIFY_ITEMCOLORCHANGE` | - | Item color changed |
| 0x0B5B | `DEF_NOTIFY_ITEMPOSLIST` | - | Item position list |
| 0x0B5C | `DEF_NOTIFY_ITEMRELEASED` | - | Item released |

### Status Notifications

| Type | Constant | Action |
|------|----------|--------|
| 0x0B07 | `DEF_NOTIFY_HP` | HP changed |
| 0x0B14 | `DEF_NOTIFY_MP` | MP changed |
| 0x0B15 | `DEF_NOTIFY_SP` | SP/stamina changed |
| 0x0B32 | `DEF_NOTIFY_CHARISMA` | Charisma changed |
| 0x0B16 | `DEF_NOTIFY_LEVELUP` | Character leveled up |
| 0x0B39 | `DEF_NOTIFY_HUNGER` | Hunger level changed |

### Magic/Skill Notifications

| Type | Constant | Action |
|------|----------|--------|
| 0x0B10 | `DEF_NOTIFY_MAGICSTUDYSUCCESS` | Magic study succeeded |
| 0x0B11 | `DEF_NOTIFY_MAGICSTUDYFAIL` | Magic study failed |
| 0x0B12 | `DEF_NOTIFY_SKILLTRAINSUCCESS` | Skill training succeeded |
| 0x0B13 | `DEF_NOTIFY_SKILLTRAINFAIL` | Skill training failed |
| 0x0B23 | `DEF_NOTIFY_SKILL` | Skill used/triggered |
| 0x0B2A | `DEF_NOTIFY_SKILLUSINGEND` | Skill usage ended |
| 0x0B59 | `DEF_NOTIFY_DOWNSKILLINDEXSET` | Quick slot skill set |
| 0x0B56 | `DEF_NOTIFY_PORTIONSUCCESS` | Potion creation succeeded |
| 0x0B55 | `DEF_NOTIFY_PORTIONFAIL` | Potion creation failed |
| 0x0B54 | `DEF_NOTIFY_LOWPORTIONSKILL` | Potion skill too low |
| 0x0B53 | `DEF_NOTIFY_NOMATCHINGPORTION` | No matching potion recipe |
| 0x0B27 | `DEF_NOTIFY_MAGICEFFECTON` | Magic effect applied |
| 0x0B28 | `DEF_NOTIFY_MAGICEFFECTOFF` | Magic effect removed |

### Combat Notifications

| Type | Constant | Payload | Action |
|------|----------|---------|--------|
| 0x0B09 | `DEF_NOTIFY_KILLED` | - | Character killed |
| 0x0B0A | `DEF_NOTIFY_EXP` | - | Experience gained |
| 0x0B1C | `DEF_NOTIFY_ENEMYKILLREWARD` | - | Enemy kill reward |
| 0x0B5A | `DEF_NOTIFY_ENEMYKILLS` | - | Total enemy kills |
| 0x0B74 | `DEF_NOTIFY_DAMAGEMOVE` | short: amount | Knockback movement |

### Commerce Notifications

| Type | Constant | Action |
|------|----------|--------|
| 0x0B08 | `DEF_NOTIFY_NOTENOUGHGOLD` | Insufficient gold |
| 0x0B2D | `DEF_NOTIFY_SELLITEMPRICE` | Sell price displayed |
| 0x0B2F | `DEF_NOTIFY_REPAIRITEMPRICE` | Repair price displayed |
| 0x0B4F | `DEF_NOTIFY_REWARDGOLD` | Gold reward received |

### Party Notifications

| Type | Constant | Payload | Action |
|------|----------|---------|--------|
| 0x0BA2 | `DEF_NOTIFY_PARTY` | short: mode, short: status, short: count, ... | Party operations |

**Party Sub-modes:**
- Mode 1: Create/accept responses
- Mode 2: Disband/leave
- Mode 4: Member join (includes 11-byte name)
- Mode 5: Member list (sV3 = count, followed by 11-byte names)
- Mode 6: Member leave
- Mode 7: Disbanded by leader
- Mode 8: Leader disconnected

### Guild Notifications

| Type | Constant | Action |
|------|----------|--------|
| 0x0B0B | `DEF_NOTIFY_GUILDDISBANDED` | Guild disbanded |
| 0x0B0E | `DEF_NOTIFY_NEWGUILDSMAN` | New member joined |
| 0x0B0F | `DEF_NOTIFY_DISMISSGUILDSMAN` | Member dismissed |
| 0x0B78 | `DEF_NOTIFY_SUCCESSBANGUILDMAN` | Member ban succeeded |
| 0x0B79 | `DEF_NOTIFY_CANNOTBANGUILDMAN` | Cannot ban member |
| 0x0BA6 | `DEF_NOTIFY_REQGUILDNAMEANSWER` | short: rank, short: index, char[20]: name |

### Exchange/Trade Notifications

| Type | Constant | Action |
|------|----------|--------|
| 0x0B5E | `DEF_NOTIFY_OPENEXCHANGEWINDOW` | Open trade window |
| 0x0B5F | `DEF_NOTIFY_SETEXCHANGEITEM` | Item added to trade |
| 0x0B60 | `DEF_NOTIFY_CANCELEXCHANGEITEM` | Trade cancelled |
| 0x0B61 | `DEF_NOTIFY_EXCHANGEITEMCOMPLETE` | Trade completed |

### World/Map Notifications

| Type | Constant | Action |
|------|----------|--------|
| 0x0B2B | `DEF_NOTIFY_SHOWMAP` | Show map dialog |
| 0x0B21 | `DEF_NOTIFY_NEWDYNAMICOBJECT` | Dynamic object created |
| 0x0B22 | `DEF_NOTIFY_DELDYNAMICOBJECT` | Dynamic object deleted |
| 0x0B41 | `DEF_NOTIFY_TIMECHANGE` | Game time changed |
| 0x0B4D | `DEF_NOTIFY_WHETHERCHANGE` | Weather changed |

### Crusade Notifications

| Type | Constant | Payload | Action |
|------|----------|---------|--------|
| 0x0B94 | `DEF_NOTIFY_CRUSADE` | int: active, int: duty, int: contrib, int: result | Crusade mode updates |
| 0x0BA0 | `DEF_NOTIFY_TCLOC` | short[2]: tele_pos, char[10]: tele_map, short[2]: const_pos, char[10]: const_map | Teleport/construction locations |
| 0x0B9F | `DEF_NOTIFY_CONSTRUCTIONPOINT` | short: points, short: contrib, short: flag | Construction point updates |
| 0x0B9E | `DEF_NOTIFY_NOMORECRUSADESTRUCTURE` | - | Cannot build more structures |
| 0x0B95 | `DEF_NOTIFY_LOCKEDMAP` | short: duration, char[10]: map | Map locked |
| 0x0B96 | `DEF_NOTIFY_DUTYSELECTED` | - | Crusade duty selected |
| 0x0B97 | `DEF_NOTIFY_MAPSTATUSNEXT` | - | Next map status |
| 0x0B98 | `DEF_NOTIFY_MAPSTATUSLAST` | - | Last map status |

### Grand Magic Notifications

| Type | Constant | Payload | Action |
|------|----------|---------|--------|
| 0x0B9D | `DEF_NOTIFY_GRANDMAGICRESULT` | WORD[3]: power/targets/duration, char[10]: caster, WORD: param, WORD[4]: target_HPs | Grand magic result |
| 0x0B9B | `DEF_NOTIFY_METEORSTRIKECOMING` | WORD: duration | Meteor strike warning |
| 0x0B9C | `DEF_NOTIFY_METEORSTRIKEHIT` | - | Meteor impact (36 effect particles) |

### Special Ability Notifications

| Type | Constant | Payload | Action |
|------|----------|---------|--------|
| 0x0B92 | `DEF_NOTIFY_SPECIALABILITYENABLED` | - | Ability can be used |
| 0x0B93 | `DEF_NOTIFY_SPECIALABILITYSTATUS` | short: type (1=enabled, 2=ready, 3=cooldown, 4=disabled), short: ability, short: time | Ability status |
| 0x0B91 | `DEF_NOTIFY_ENERGYSPHEREGOALIN` | short: id, short: incoming, short: defending, char[20]: player | Energy sphere goal |
| 0x0B90 | `DEF_NOTIFY_ENERGYSPHERECREATED` | - | Energy sphere created |

### NPC/Chat Notifications

| Type | Constant | Action |
|------|----------|--------|
| 0x0B57 | `DEF_NOTIFY_NPCTALK` | NPC talk received |
| 0x0B0C | `DEF_NOTIFY_EVENTMSGSTRING` | Event message string |
| 0x0B46 | `DEF_NOTIFY_NOTICEMSG` | Notice message |
| 0x0B42 | `DEF_NOTIFY_PLAYERSHUTUP` | Player muted |

### Miscellaneous Notifications

| Type | Constant | Action |
|------|----------|--------|
| 0x0B29 | `DEF_NOTIFY_TOTALUSERS` | Total users online |
| 0x0B33 | `DEF_NOTIFY_PLAYERONGAME` | Player online |
| 0x0B34 | `DEF_NOTIFY_PLAYERNOTONGAME` | Player offline |
| 0x0B35 | `DEF_NOTIFY_WHISPERMODEON` | Whisper mode on |
| 0x0B36 | `DEF_NOTIFY_WHISPERMODEOFF` | Whisper mode off |
| 0x0B37 | `DEF_NOTIFY_PLAYERPROFILE` | Player profile info |
| 0x0B40 | `DEF_NOTIFY_TOBERECALLED` | Player recalled |
| 0x0B44 | `DEF_NOTIFY_CANNOTRATING` | Cannot rate player |
| 0x0B45 | `DEF_NOTIFY_RATINGPLAYER` | Player rated |
| 0x0B49 | `DEF_NOTIFY_DEBUGMSG` | Debug message |
| 0x0B4A | `DEF_NOTIFY_FISHSUCCESS` | Fishing succeeded |
| 0x0B4B | `DEF_NOTIFY_FISHFAIL` | Fishing failed |
| 0x0B4C | `DEF_NOTIFY_FISHCANCELED` | Fishing cancelled |
| 0x0B47 | `DEF_NOTIFY_EVENTFISHMODE` | Event fishing mode |
| 0x0B48 | `DEF_NOTIFY_FISHCHANCE` | Fish chance displayed |
| 0x0B4E | `DEF_NOTIFY_SERVERSHUTDOWN` | Server shutdown |
| 0x0B50 | `DEF_NOTIFY_IPACCOUNTINFO` | IP/account info |
| 0x0B51 | `DEF_NOTIFY_SAFEATTACKMODE` | Safe attack mode |
| 0x0B52 | `DEF_NOTIFY_SUPERATTACKLEFT` | Super attack remaining |
| 0x0B1A | `DEF_NOTIFY_PKPENALTY` | PK penalty applied |
| 0x0B1B | `DEF_NOTIFY_PKCAPTURED` | Player captured for PK |
| 0x0B66 | `DEF_NOTIFY_QUESTCONTENTS` | Quest info |
| 0x0B67 | `DEF_NOTIFY_QUESTABORTED` | Quest aborted |
| 0x0B68 | `DEF_NOTIFY_QUESTCOMPLETED` | Quest completed |
| 0x0B69 | `DEF_NOTIFY_QUESTREWARD` | Quest reward |
| 0x0B70 | `DEF_NOTIFY_BUILDITEMSUCCESS` | Build succeeded |
| 0x0B71 | `DEF_NOTIFY_BUILDITEMFAIL` | Build failed |
| 0x0B72 | `DEF_NOTIFY_OBSERVERMODE` | Observer mode |
| 0x0B73 | `DEF_NOTIFY_GLOBALATTACKMODE` | Global attack mode |
| 0x0B75 | `DEF_NOTIFY_FORCEDISCONN` | Force disconnect |
| 0x0B76 | `DEF_NOTIFY_FIGHTZONERESERVE` | Fight zone reserved |
| 0x0B77 | `DEF_NOTIFY_NOGUILDMASTERLEVEL` | Guild master level too low |
| 0x0B80 | `DEF_NOTIFY_RESPONSE_CREATENEWPARTY` | Party creation response |
| 0x0B81 | `DEF_NOTIFY_QUERY_JOINPARTY` | Party join query |
| 0x0B99 | `DEF_NOTIFY_HELP` | Help notification |
| 0x0B9A | `DEF_NOTIFY_HELPFAILED` | Help failed |
| 0x0BAA | `DEF_NOTIFY_MONSTEREVENT_POSITION` | char: id, short: x, short: y |
| 0x0BB0 | `DEF_NOTIFY_NOMOREAGRICULTURE` | Agriculture unavailable |
| 0x0BB1 | `DEF_NOTIFY_AGRICULTURESKILLLIMIT` | Agri skill too low |
| 0x0BB2 | `DEF_NOTIFY_AGRICULTURENOAREA` | Not in agri area |

---

## Login Server Handlers (LogResponseHandler)

**Location**: Game.cpp:14773-15450

### Packet Structure

```
Bytes 0-3: Message ID (DWORD)
Bytes 4-5: Response Type (WORD)
Bytes 6+:  Variable payload
```

### Login Response Types

| Type | Constant | Payload | Action |
|------|----------|---------|--------|
| 0x0F14 | `DEF_LOGRESMSGTYPE_CONFIRM` | Complex (see below) | Successful login |
| 0x0F15 | `DEF_LOGRESMSGTYPE_REJECT` | int[3]: block date | Login rejected |
| 0x0F16 | `DEF_LOGRESMSGTYPE_PASSWORDMISMATCH` | - | Wrong password |
| 0x0F17 | `DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT` | - | Account doesn't exist |
| 0x0F18 | `DEF_LOGRESMSGTYPE_NEWACCOUNTCREATED` | - | Account created |
| 0x0F19 | `DEF_LOGRESMSGTYPE_NEWACCOUNTFAILED` | - | Account creation failed |
| 0x0F1A | `DEF_LOGRESMSGTYPE_ALREADYEXISTINGACCOUNT` | - | Account exists |
| 0x0F1B | `DEF_LOGRESMSGTYPE_NOTEXISTINGCHARACTER` | - | Character doesn't exist |
| 0x0F1C | `DEF_LOGRESMSGTYPE_NEWCHARACTERCREATED` | char[10]: name, char: total, [list] | Character created |
| 0x0F1D | `DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED` | - | Character creation failed |
| 0x0F1F | `DEF_LOGRESMSGTYPE_CHARACTERDELETED` | char: status, char: total, [list] | Character deleted |
| 0x0F30 | `DEF_LOGRESMSGTYPE_NOTENOUGHPOINT` | - | Not enough points |
| 0x0F31 | `DEF_LOGRESMSGTYPE_ACCOUNTLOCKED` | - | Account locked |
| 0x0F32 | `DEF_LOGRESMSGTYPE_SERVICENOTAVAILABLE` | - | Service unavailable |
| 0x0A00 | `DEF_LOGRESMSGTYPE_PASSWORDCHANGESUCCESS` | - | Password changed |
| 0x0A01 | `DEF_LOGRESMSGTYPE_PASSWORDCHANGEFAIL` | - | Password change failed |
| 0x0A02 | `DEF_LOGRESMSGTYPE_NOTEXISTINGWORLDSERVER` | - | World server not found |
| 0x0A03 | `DEF_LOGRESMSGTYPE_INPUTKEYCODE` | - | Input key code request |
| 0x0A04 | `DEF_LOGRESMSGTYPE_REALACCOUNT` | - | Real account |
| 0x0A05 | `DEF_LOGRESMSGTYPE_FORCECHANGEPASSWORD` | - | Force password change |
| 0x0A06 | `DEF_LOGRESMSGTYPE_INVALIDKOREANSSN` | - | Invalid Korean SSN |
| 0x0A07 | `DEF_LOGRESMSGTYPE_LESSTHENFIFTEEN` | - | Age less than 15 |

### DEF_LOGRESMSGTYPE_CONFIRM Payload

```
WORD:       server_version_upper
WORD:       server_version_lower
char:       account_status
WORD[3]:    dates (year/month/day)
char:       total_characters

For each character (if name not empty):
  char[10]: character_name
  char:     unknown_byte
  WORD[4]:  appearance (Appr1-4)
  WORD:     sex
  WORD:     skin_color
  WORD:     level
  DWORD:    experience
  WORD[6]:  stats (Str/Vit/Dex/Int/Mag/Chr)
  DWORD:    color
  WORD[5]:  creation_date (year/month/day/hour/minute)
  char[10]: map_name

int:        time_left_account_sec
int:        time_left_IP_sec
```

### Gateway Response (v2.05+)

| Message ID | Payload | Action |
|------------|---------|--------|
| `MSGID_GETMINIMUMLOADGATEWAY` | char[15]: ip, int: port | Redirect to minimum-load gateway |

---

## Common Type Commands (DEF_COMMONTYPE_*)

Sub-message types for `MSGID_COMMAND_COMMON`:

### Client to Server Requests

| Type | Constant | Purpose |
|------|----------|---------|
| 0x0A03 | `DEF_COMMONTYPE_REQ_LISTCONTENTS` | Request shop/storage contents |
| 0x0A04 | `DEF_COMMONTYPE_REQ_PURCHASEITEM` | Purchase item |
| 0x0A0E | `DEF_COMMONTYPE_REQ_STUDYMAGIC` | Learn magic |
| 0x0A0F | `DEF_COMMONTYPE_REQ_TRAINSKILL` | Train skill |
| 0x0A10 | `DEF_COMMONTYPE_REQ_GETREWARDMONEY` | Get quest reward |
| 0x0A11 | `DEF_COMMONTYPE_REQ_USEITEM` | Use item |
| 0x0A12 | `DEF_COMMONTYPE_REQ_USESKILL` | Use skill |
| 0x0A13 | `DEF_COMMONTYPE_REQ_SELLITEM` | Sell item |
| 0x0A14 | `DEF_COMMONTYPE_REQ_REPAIRITEM` | Repair item |
| 0x0A15 | `DEF_COMMONTYPE_REQ_SELLITEMCONFIRM` | Confirm sale |
| 0x0A16 | `DEF_COMMONTYPE_REQ_REPAIRITEMCONFIRM` | Confirm repair |
| 0x0A17 | `DEF_COMMONTYPE_REQ_GETFISHTHISTIME` | Request fishing |
| 0x0A1B | `DEF_COMMONTYPE_REQ_SETDOWNSKILLINDEX` | Set quick skill slot |
| 0x0A1C | `DEF_COMMONTYPE_REQ_GETOCCUPYFLAG` | Get occupy flag |
| 0x0A1D | `DEF_COMMONTYPE_REQ_GETHEROMANTLE` | Get hero mantle |
| 0x0A24 | `DEF_COMMONTYPE_GETMAGICABILITY` | Get magic ability info |
| 0x0A40 | `DEF_COMMONTYPE_REQUEST_ACTIVATESPECABLTY` | Activate special ability |
| 0x0A50 | `DEF_COMMONTYPE_REQUEST_CANCELQUEST` | Cancel quest |
| 0x0A51 | `DEF_COMMONTYPE_REQUEST_SELECTCRUSADEDUTY` | Select crusade duty |
| 0x0A52 | `DEF_COMMONTYPE_REQUEST_MAPSTATUS` | Request map status |
| 0x0A53 | `DEF_COMMONTYPE_REQUEST_HELP` | Request help |
| 0x0A60 | `DEF_COMMONTYPE_REQUEST_HUNTMODE` | Request hunt mode |
| 0x0A30 | `DEF_COMMONTYPE_REQUEST_ACCEPTJOINPARTY` | Accept party invite |
| 0x0A31 | `DEF_COMMONTYPE_REQUEST_JOINPARTY` | Join party request |

### Server to Client Notifications

| Type | Constant | Purpose |
|------|----------|---------|
| 0x0A02 | `DEF_COMMONTYPE_EQUIPITEM` | Item equipped |
| 0x0A05 | `DEF_COMMONTYPE_GIVEITEMTOCHAR` | Give item to character |
| 0x0A0A | `DEF_COMMONTYPE_RELEASEITEM` | Release/unequip item |
| 0x0A0B | `DEF_COMMONTYPE_TOGGLECOMBATMODE` | Toggle combat mode |
| 0x0A0D | `DEF_COMMONTYPE_MAGIC` | Magic cast event |
| 0x0A19 | `DEF_COMMONTYPE_REQ_CREATEPORTION` | Create potion |
| 0x0A1A | `DEF_COMMONTYPE_TALKTONPC` | Talk to NPC |
| 0x0A1E | `DEF_COMMONTYPE_EXCHANGEITEMTOCHAR` | Open trade |
| 0x0A1F | `DEF_COMMONTYPE_SETEXCHANGEITEM` | Add trade item |
| 0x0A20 | `DEF_COMMONTYPE_CONFIRMEXCHANGEITEM` | Confirm trade |
| 0x0A21 | `DEF_COMMONTYPE_CANCELEXCHANGEITEM` | Cancel trade |
| 0x0A22 | `DEF_COMMONTYPE_QUESTACCEPTED` | Quest accepted |
| 0x0A23 | `DEF_COMMONTYPE_BUILDITEM` | Build item |
| 0x0A25 | `DEF_COMMONTYPE_CLEARGUILDNAME` | Clear guild cache |
| 0x0A26 | `DEF_COMMONTYPE_BANGUILD` | Ban from guild |
| 0x0A54 | `DEF_COMMONTYPE_SETGUILDTELEPORTLOC` | Set guild teleport |
| 0x0A55 | `DEF_COMMONTYPE_GUILDTELEPORT` | Guild teleport |
| 0x0A56 | `DEF_COMMONTYPE_SUMMONWARUNIT` | Summon war unit |
| 0x0A57 | `DEF_COMMONTYPE_SETGUILDCONSTRUCTLOC` | Set construction loc |
| 0x0A58 | `DEF_COMMONTYPE_UPGRADEITEM` | Upgrade item |
| 0x0A59 | `DEF_COMMONTYPE_REQGUILDNAME` | Request guild name |

---

## Initialization Messages

### InitDataResponseHandler (MSGID_RESPONSE_INITDATA)

Loads all static configuration data on game start:

| Sub-message ID | Constant | Data Type |
|----------------|----------|-----------|
| 0x0FA314D9 | `MSGID_ITEMCONFIGURATIONCONTENTS` | Item definitions |
| 0x0FA314DA | `MSGID_NPCCONFIGURATIONCONTENTS` | NPC data |
| 0x0FA314DB | `MSGID_MAGICCONFIGURATIONCONTENTS` | Magic spells (100+) |
| 0x0FA314DC | `MSGID_SKILLCONFIGURATIONCONTENTS` | Skills (60) |
| 0x0FA314DE | `MSGID_PORTIONCONFIGURATIONCONTENTS` | Potions |
| 0x0FA40001 | `MSGID_QUESTCONFIGURATIONCONTENTS` | Quests |
| 0x0FA40002 | `MSGID_BUILDITEMCONFIGURATIONCONTENTS` | Build items (100) |
| 0x0FA40003 | `MSGID_DUPITEMIDFILECONTENTS` | Item ID duplicates |
| 0x0FA40004 | `MSGID_NOTICEMENTFILECONTENTS` | Announcements |

---

## Other Specialized Handlers

| Handler | Location | Purpose |
|---------|----------|---------|
| `InitPlayerResponseHandler()` | 3085 | Initial player data on game server connect |
| `InitDataResponseHandler()` | 45800 | Load static game data |
| `CreateNewGuildResponseHandler()` | 5429 | Guild creation result |
| `DisbandGuildResponseHandler()` | 5594 | Guild disband result |
| `LogEventHandler()` | 14610 | Login server events |
| `ChatMsgHandler()` | 15678 | Chat messages (sender, position, type, text) |
| `CivilRightAdmissionHandler()` | 22576 | Civil rights response |
| `DynamicObjectHandler()` | 22854 | Dynamic objects (NPCs, monsters, effects) |
| `NoticementHandler()` | 24584 | In-game announcements |
| `ResponsePanningHandler()` | 24670 | Map panning response |
| `ReserveFightzoneResponseHandler()` | 31467 | Fight zone result |
| `RetrieveItemHandler()` | 31887 | Item retrieval |
| `ResponseTeleportList()` | 43057 | Teleport destinations |
| `ResponseChargedTeleport()` | 43091 | Teleport confirmation |
| `MotionResponseHandler()` | 35498 | Motion command response |
| `MotionEventHandler()` | 46077 | Entity motion events |

---

## Protocol Quirks & Compatibility Notes

### String Lengths
- **Character Name**: 10 bytes max (null-terminated, often padded)
- **Map Name**: 10 bytes max
- **Guild Name**: 20 bytes (underscores → spaces for display)

### Numeric Types
- **Gold/Money**: DWORD (32-bit), can exceed 2 billion
- **Experience**: DWORD (32-bit)
- **Item Lifespan**: WORD, tracked in game tick cycles
- **Attributes**: DWORD bitmask for item/player attributes

### Appearance Data
- **Appr1-4**: 4 WORDs for clothing/armor slots

### Date/Time Format
- **Date**: WORD year + WORD month + WORD day
- **Time**: WORD hour + WORD minute

### Entity Limits
- **Party Members**: 8 max (6 for some quest groupings)
- **Guild Members**: 32 max

### Endianness
- **Little-endian** (Intel format) for all multi-byte integers

### Confirmation Code
- 1-byte code prepended to packets for validation

---

## Game Mode State Machine Integration

Message handlers transition the game between modes:

```
MAINMENU → SELECTSERVER → LOGIN → SELECTCHARACTER → ENTERGAME → GAME
                                                          ↓
                                          MSGID_RESPONSE_INITPLAYER triggers
```

Key states:
- `DEF_GAMEMODE_ONWAITINGRESPONSE` - Waiting for server response
- `DEF_GAMEMODE_ONWAITINGINITDATA` - Waiting for game data after player init
- `DEF_GAMEMODE_ONLOGRESMSG` - Displaying login result
- `DEF_GAMEMODE_ONCONNECTIONLOST` - Connection dropped

---

## Known Issues / Technical Debt

1. **Monolithic Handler**: All handlers embedded in 48,500-line CGame class
2. **No Error Recovery**: Many handlers assume valid data without bounds checking
3. **Hardcoded Sizes**: String lengths (10, 20 bytes) hardcoded throughout
4. **Global State**: Handlers modify global game state directly
5. **No Packet Validation**: Minimal validation of packet contents
6. **Blocking Operations**: Some handlers perform blocking operations
7. **Memory Leaks**: Manual memory management with potential leaks

---

## Modernization Notes

### Recommended Approach

1. **Message Registry**: Create type-safe message registry with handler registration
2. **Packet Classes**: Strongly-typed packet classes with serialization
3. **Handler Interface**: Abstract `IMessageHandler` with typed dispatch
4. **Validation Layer**: Validate all packets before processing
5. **Async Processing**: Use coroutines for async packet handling
6. **State Machine**: Decouple handlers from game state machine

### Example Modern Handler

```cpp
namespace hb::net {
    template<typename T>
    concept PacketHandler = requires(T handler, const Packet& pkt) {
        { handler.handle(pkt) } -> std::same_as<void>;
    };

    class MessageDispatcher {
    public:
        template<PacketHandler H>
        void registerHandler(MessageId id, H&& handler);

        void dispatch(const Packet& packet);
    };

    struct NotifyPacket {
        NotifyType type;
        std::span<const std::byte> payload;

        static std::expected<NotifyPacket, Error> parse(std::span<const std::byte> data);
    };
}
```

### Protocol Preservation

- **CRITICAL**: Exact byte-level compatibility required for existing servers
- Document all packet formats with byte offsets
- Create protocol specification separate from implementation
- Add packet capture/replay testing for verification
