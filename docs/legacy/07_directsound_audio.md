# Legacy Audio System Documentation

## Overview

The Helbreath client audio system is built on **Microsoft DirectSound 7** (circa 2000-2002), providing sound effect playback and background music functionality. The system is tightly integrated into the monolithic `CGame` class but structured around a few dedicated classes for sound management.

### Architecture Summary

```
┌─────────────────────────────────────────────────────────────────┐
│                         CGame Class                              │
│  ┌──────────────┐  ┌──────────────────────────────────────────┐ │
│  │   YWSound    │  │         Sound Buffer Arrays               │ │
│  │  (DSound     │  │  ┌────────────┐ ┌────────────┐ ┌───────┐ │ │
│  │   Init)      │──│  │ m_pCSound  │ │ m_pMSound  │ │m_pBGM │ │ │
│  │              │  │  │  [110]     │ │   [110]    │ │       │ │ │
│  │ m_lpDS ──────│──│  │ Character  │ │  Monster   │ │ Music │ │ │
│  │ m_DSCaps     │  │  │  Sounds    │ │  Sounds    │ │       │ │ │
│  └──────────────┘  │  └────────────┘ └────────────┘ └───────┘ │ │
│                    │  ┌────────────┐                          │ │
│                    │  │ m_pESound  │                          │ │
│                    │  │   [110]    │                          │ │
│                    │  │  Effect    │                          │ │
│                    │  │  Sounds    │                          │ │
│                    │  └────────────┘                          │ │
│                    └──────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

---

## Source Files

| File | Purpose | Lines |
|------|---------|-------|
| `YWSound.h` | DirectSound initialization class header | 23 |
| `YWSound.cpp` | DirectSound initialization implementation | 75 |
| `SoundBuffer.h` | Individual sound buffer management header | 43 |
| `SoundBuffer.cpp` | Sound buffer implementation with WAV loading | 302 |
| `SoundID.h` | Sound effect ID constants | 21 |
| `DXC_dsound.h` | Empty placeholder class (unused) | 20 |
| `DXC_dsound.cpp` | Empty placeholder implementation | 19 |

---

## Class: YWSound

### Purpose
Initializes the DirectSound subsystem and creates the primary sound buffer with optimal audio format settings.

### Header Definition (YWSound.h)

```cpp
#include "dsound.h"

class YWSound
{
public:
    YWSound();
    virtual ~YWSound();
    bool Create(HWND hWnd);

    LPDIRECTSOUND m_lpDS;    // DirectSound interface pointer
    DSCAPS m_DSCaps;         // DirectSound capabilities structure
};
```

### Member Variables

| Variable | Type | Description |
|----------|------|-------------|
| `m_lpDS` | `LPDIRECTSOUND` | Pointer to the DirectSound COM interface. Used to create sound buffers and control audio device. |
| `m_DSCaps` | `DSCAPS` | Structure containing device capabilities (hardware mixing, 3D support, etc.). Passed to sound buffers for hardware acceleration decisions. |

### Constructor/Destructor

```cpp
YWSound::YWSound()
{
    m_lpDS = NULL;
}

YWSound::~YWSound()
{
    if(m_lpDS) m_lpDS->Release();
}
```

**Notes:**
- Constructor initializes DirectSound pointer to NULL
- Destructor properly releases the DirectSound COM interface
- Uses COM reference counting via `Release()`

### Create Method

```cpp
bool YWSound::Create(HWND hWnd)
{
    HRESULT rval;
    LPDIRECTSOUNDBUFFER lpDsb;
    DSBUFFERDESC dsbdesc;
    WAVEFORMATEX wfm;

    // Step 1: Create DirectSound object
    rval = DirectSoundCreate(NULL, &m_lpDS, NULL);
    if(rval != DS_OK)
    {
        OutputDebugString("DirectSoundCreate error...\n");
        return FALSE;
    }

    // Step 2: Set priority cooperative level (allows format changes)
    rval = m_lpDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY);
    if(rval != DS_OK)
    {
        OutputDebugString("DirectSoundCreate error...\n");
        return FALSE;
    }

    // Step 3: Create primary buffer descriptor
    memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
    dsbdesc.dwSize = sizeof(DSBUFFERDESC);
    dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
    dsbdesc.dwBufferBytes = 0;        // Must be 0 for primary buffer
    dsbdesc.lpwfxFormat = NULL;       // Must be NULL for primary buffer

    // Step 4: Set up audio format (CD quality: 44.1kHz, 16-bit, stereo)
    memset(&wfm, 0, sizeof(WAVEFORMATEX));
    wfm.wFormatTag = WAVE_FORMAT_PCM;
    wfm.nChannels = 2;                        // Stereo
    wfm.nSamplesPerSec = 44100;               // 44.1 kHz
    wfm.wBitsPerSample = 16;                  // 16-bit
    wfm.nBlockAlign = wfm.wBitsPerSample / 8 * wfm.nChannels;  // 4 bytes
    wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign; // 176,400 bytes/sec

    // Step 5: Create primary buffer and set format
    rval = m_lpDS->CreateSoundBuffer(&dsbdesc, &lpDsb, NULL);
    if (rval != DS_OK) return FALSE;

    lpDsb->SetFormat(&wfm);

    // Step 6: Downgrade to normal cooperative level
    rval = m_lpDS->SetCooperativeLevel(hWnd, DSSCL_NORMAL);
    if(rval != DS_OK)
    {
        OutputDebugString("DirectSoundCreate error...\n");
        return FALSE;
    }

    return TRUE;
}
```

### Audio Format Configuration

| Parameter | Value | Description |
|-----------|-------|-------------|
| Format Tag | `WAVE_FORMAT_PCM` | Uncompressed PCM audio |
| Channels | 2 | Stereo output |
| Sample Rate | 44,100 Hz | CD quality sample rate |
| Bits Per Sample | 16 | 16-bit audio depth |
| Block Align | 4 bytes | (16 bits / 8) × 2 channels |
| Bytes Per Second | 176,400 | 44,100 × 4 |

### Cooperative Level Strategy

The code uses a two-phase cooperative level approach:

1. **DSSCL_PRIORITY** - Set initially to allow format changes on primary buffer
2. **DSSCL_NORMAL** - Downgraded after setup for standard playback

This pattern was common in DirectSound 7 to set optimal audio format while maintaining system compatibility.

---

## Class: CSoundBuffer

### Purpose
Manages individual sound effects with support for:
- WAV file loading from disk
- Double buffering for overlapping playback
- Volume and pan control
- Looping playback
- Automatic buffer recovery when lost

### Header Definition (SoundBuffer.h)

```cpp
#include "dsound.h"

#define DEF_MAXSOUNDBUFFERS  2  // Maximum duplicate buffers per sound

class CSoundBuffer
{
public:
    void _ReleaseSoundBuffer();
    void bStop(BOOL bIsNoRewind = FALSE);
    void SetVolume(LONG Volume);
    LPDIRECTSOUNDBUFFER GetIdleBuffer();
    BOOL Play(BOOL bLoop = FALSE, long lPan = 0, int iVol = 0);
    BOOL _LoadWavContents(char cBufferIndex, FILE* pFile, DWORD dwSize, DWORD dwPos);
    BOOL bCreateBuffer_LoadWavFileContents(char cBufferIndex);
    BOOL _bCreateSoundBuffer(char cBufferIndex, DWORD dwBufSize, DWORD dwFreq,
                             DWORD dwBitsPerSample, DWORD dwBlkAlign, BOOL bStereo);

    CSoundBuffer(LPDIRECTSOUND lpDS, DSCAPS DSCaps, char* pWavFileName,
                 BOOL bIsSingleLoad = FALSE);
    virtual ~CSoundBuffer();

    LPDIRECTSOUND       m_lpDS;                      // DirectSound interface
    DSCAPS              m_DSCaps;                    // Device capabilities
    char                m_cWavFileName[32];          // Sound file path
    LPDIRECTSOUNDBUFFER m_lpDSB[DEF_MAXSOUNDBUFFERS]; // Sound buffer array
    char                m_cCurrentBufferIndex;       // Active buffer index
    BOOL                m_bIsSingleLoad;             // Single instance mode
    BOOL                m_bIsLooping;                // Currently looping
    DWORD               m_dwTime;                    // Last play timestamp
};
```

### Member Variables

| Variable | Type | Size | Description |
|----------|------|------|-------------|
| `m_lpDS` | `LPDIRECTSOUND` | 4 bytes | Pointer to parent DirectSound interface |
| `m_DSCaps` | `DSCAPS` | 96 bytes | Copy of device capabilities |
| `m_cWavFileName` | `char[32]` | 32 bytes | Relative path to WAV file |
| `m_lpDSB` | `LPDIRECTSOUNDBUFFER[2]` | 8 bytes | Array of 2 buffer pointers for double-buffering |
| `m_cCurrentBufferIndex` | `char` | 1 byte | Index of currently active buffer (0 or 1) |
| `m_bIsSingleLoad` | `BOOL` | 4 bytes | If TRUE, only one instance can play (stops previous) |
| `m_bIsLooping` | `BOOL` | 4 bytes | TRUE if sound is currently looping |
| `m_dwTime` | `DWORD` | 4 bytes | Timestamp of last playback (for cache management) |

### WAV File Header Structure

```cpp
struct Waveheader
{
    BYTE        RIFF[4];          // "RIFF" magic bytes
    DWORD       dwSize;           // Size of data to follow
    BYTE        WAVE[4];          // "WAVE" magic bytes
    BYTE        fmt_[4];          // "fmt " chunk marker
    DWORD       dw16;             // 16 (size of format chunk)
    WORD        wOne_0;           // 1 (PCM format)
    WORD        wChnls;           // Number of channels (1=mono, 2=stereo)
    DWORD       dwSRate;          // Sample rate (Hz)
    DWORD       BytesPerSec;      // Bytes per second
    WORD        wBlkAlign;        // Block alignment
    WORD        BitsPerSample;    // Bits per sample (8 or 16)
    BYTE        DATA[4];          // "DATA" chunk marker
    DWORD       dwDSize;          // Size of audio data in bytes
};
```

**Note:** This is a simplified WAV header that assumes no extra chunks. Standard WAV files may have additional metadata chunks that would break this parser.

### Constructor

```cpp
CSoundBuffer::CSoundBuffer(LPDIRECTSOUND lpDS, DSCAPS DSCaps,
                           char* pWavFileName, BOOL bIsSingleLoad)
{
    int i;

    m_lpDS = lpDS;
    m_DSCaps = DSCaps;

    ZeroMemory(m_cWavFileName, sizeof(m_cWavFileName));
    strcpy(m_cWavFileName, pWavFileName);

    // Note: Lazy loading - buffers not created until first play
    // Commented-out code shows original eager loading approach

    for (i = 0; i < DEF_MAXSOUNDBUFFERS; i++)
        m_lpDSB[i] = NULL;

    m_cCurrentBufferIndex = 0;
    m_bIsSingleLoad = bIsSingleLoad;
    m_dwTime = NULL;
    m_bIsLooping = FALSE;
}
```

**Key Behavior:**
- Stores file path but does NOT load audio data immediately
- Uses lazy loading - WAV data loaded on first `Play()` call
- Original code had eager loading (commented out) but was disabled

### Destructor

```cpp
CSoundBuffer::~CSoundBuffer()
{
    for (int i = 0; i < DEF_MAXSOUNDBUFFERS; i++)
    {
        if (m_lpDSB[i] != NULL)
        {
            m_lpDSB[i]->Release();
            m_lpDSB[i] = NULL;
        }
    }
}
```

### Buffer Creation Method

```cpp
BOOL CSoundBuffer::_bCreateSoundBuffer(char cBufferIndex, DWORD dwBufSize,
    DWORD dwFreq, DWORD dwBitsPerSample, DWORD dwBlkAlign, BOOL bStereo)
{
    PCMWAVEFORMAT pcmwf;
    DSBUFFERDESC dsbdesc;

    // Skip if already allocated
    if (m_lpDSB[cBufferIndex] != NULL) return FALSE;

    // Set up PCM format
    memset(&pcmwf, 0, sizeof(PCMWAVEFORMAT));
    pcmwf.wf.wFormatTag         = WAVE_FORMAT_PCM;
    pcmwf.wf.nChannels          = bStereo ? 2 : 1;
    pcmwf.wf.nSamplesPerSec     = dwFreq;
    pcmwf.wf.nBlockAlign        = (WORD)dwBlkAlign;
    pcmwf.wf.nAvgBytesPerSec    = pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign;
    pcmwf.wBitsPerSample        = (WORD)dwBitsPerSample;

    // Set up buffer descriptor
    memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
    dsbdesc.dwSize              = sizeof(DSBUFFERDESC);
    dsbdesc.dwFlags             = DSBCAPS_STATIC |
                                  DSBCAPS_GETCURRENTPOSITION2 |
                                  DSBCAPS_CTRLPAN |
                                  DSBCAPS_CTRLFREQUENCY |
                                  DSBCAPS_CTRLVOLUME;
    dsbdesc.dwBufferBytes       = dwBufSize;
    dsbdesc.lpwfxFormat         = (LPWAVEFORMATEX)&pcmwf;

    if (m_lpDS->CreateSoundBuffer(&dsbdesc, &m_lpDSB[cBufferIndex], NULL) != DS_OK)
        return FALSE;

    return TRUE;
}
```

### Buffer Capability Flags

| Flag | Hex Value | Purpose |
|------|-----------|---------|
| `DSBCAPS_STATIC` | 0x00000002 | Uses hardware memory when available |
| `DSBCAPS_GETCURRENTPOSITION2` | 0x00010000 | More accurate play position |
| `DSBCAPS_CTRLPAN` | 0x00000040 | Enable stereo panning control |
| `DSBCAPS_CTRLFREQUENCY` | 0x00000020 | Enable playback rate control |
| `DSBCAPS_CTRLVOLUME` | 0x00000080 | Enable volume control |

### WAV Loading Method

```cpp
BOOL CSoundBuffer::bCreateBuffer_LoadWavFileContents(char cBufferIndex)
{
    FILE* pFile;
    Waveheader Wavhdr;
    DWORD dwSize;
    BOOL bStereo;

    // Skip if already loaded
    if (m_lpDSB[cBufferIndex] != NULL) return FALSE;

    pFile = fopen(m_cWavFileName, "rb");
    if (pFile == NULL) return FALSE;

    if (fread(&Wavhdr, sizeof(Wavhdr), 1, pFile) != 1)
    {
        fclose(pFile);
        return FALSE;
    }

    dwSize = Wavhdr.dwDSize;
    bStereo = Wavhdr.wChnls > 1 ? TRUE : FALSE;

    if (_bCreateSoundBuffer(cBufferIndex, dwSize, Wavhdr.dwSRate,
                            Wavhdr.BitsPerSample, Wavhdr.wBlkAlign, bStereo) == FALSE)
    {
        fclose(pFile);
        return FALSE;
    }

    if (!_LoadWavContents(cBufferIndex, pFile, dwSize, sizeof(Wavhdr)))
    {
        fclose(pFile);
        return FALSE;
    }

    fclose(pFile);
    return TRUE;
}
```

### Buffer Data Loading

```cpp
BOOL CSoundBuffer::_LoadWavContents(char cBufferIndex, FILE* pFile,
                                     DWORD dwSize, DWORD dwPos)
{
    LPVOID pData1, pData2;
    DWORD dwData1Size, dwData2Size;
    HRESULT rval;

    if (m_lpDSB[cBufferIndex] == NULL) return FALSE;
    if (dwPos == 0xffffffff) return FALSE;
    if (fseek(pFile, dwPos, SEEK_SET) != 0) return FALSE;

    // Lock entire buffer for writing
    rval = m_lpDSB[cBufferIndex]->Lock(0, dwSize, &pData1, &dwData1Size,
                                        &pData2, &dwData2Size, DSBLOCK_ENTIREBUFFER);
    if (rval != DS_OK) return FALSE;

    // Read WAV data into buffer
    if (dwData1Size > 0)
        if (fread(pData1, dwData1Size, 1, pFile) != 1)
            return FALSE;

    if (dwData2Size > 0)
        if (fread(pData2, dwData2Size, 1, pFile) != 1)
            return FALSE;

    rval = m_lpDSB[cBufferIndex]->Unlock(pData1, dwData1Size, pData2, dwData2Size);
    if (rval != DS_OK) return FALSE;

    // Reset frequency to original WAV frequency
    m_lpDSB[cBufferIndex]->SetFrequency(DSBFREQUENCY_ORIGINAL);

    return TRUE;
}
```

**Lock/Unlock Pattern:**
- DirectSound uses a lock/unlock pattern similar to graphics surfaces
- `DSBLOCK_ENTIREBUFFER` locks the complete buffer in one call
- Two data pointers returned because circular buffers may wrap

### Play Method

```cpp
BOOL CSoundBuffer::Play(BOOL bLoop, long lPan, int iVol)
{
    HRESULT rval;
    LPDIRECTSOUNDBUFFER Buffer = NULL;

    if(m_lpDS == NULL) return FALSE;

    // Get an available buffer (handles double-buffering)
    Buffer = GetIdleBuffer();
    if(Buffer == NULL) return FALSE;

    // Set volume
    SetVolume(iVol);

    // Clamp pan to valid range
    if (lPan < DSBPAN_LEFT) lPan = DSBPAN_LEFT;      // -10000
    else if (lPan > DSBPAN_RIGHT) lPan = DSBPAN_RIGHT; // +10000
    Buffer->SetPan(lPan);

    m_bIsLooping = bLoop;

    if (bLoop == FALSE)
         rval = Buffer->Play(0, 0, 0);                    // One-shot
    else
         rval = Buffer->Play(0, 0, DSBPLAY_LOOPING);     // Loop forever

    if(rval != DS_OK) return FALSE;

    return TRUE;
}
```

### GetIdleBuffer - Double Buffer Management

```cpp
LPDIRECTSOUNDBUFFER CSoundBuffer::GetIdleBuffer(void)
{
    DWORD Status;
    HRESULT rval;
    LPDIRECTSOUNDBUFFER Buffer;

    Buffer = NULL;

    if (m_lpDSB[m_cCurrentBufferIndex] != NULL) {
        rval = m_lpDSB[m_cCurrentBufferIndex]->GetStatus(&Status);
        if (rval < 0) Status = 0;

        if (Status & DSBSTATUS_BUFFERLOST) {
            // Buffer was lost (alt-tab, etc.) - recreate it
            m_lpDSB[m_cCurrentBufferIndex]->Release();
            m_lpDSB[m_cCurrentBufferIndex] = NULL;
            bCreateBuffer_LoadWavFileContents(m_cCurrentBufferIndex);
        }
        else if ((Status & DSBSTATUS_PLAYING) == DSBSTATUS_PLAYING) {
            // Current buffer is playing

            if (m_bIsSingleLoad == TRUE) {
                // Single-instance mode: stop current and reuse
                m_lpDSB[m_cCurrentBufferIndex]->Stop();
                m_lpDSB[m_cCurrentBufferIndex]->SetCurrentPosition(0);
                Buffer = m_lpDSB[m_cCurrentBufferIndex];
                m_dwTime = timeGetTime();
                return Buffer;
            }

            // Double-buffer mode: switch to alternate buffer
            m_cCurrentBufferIndex++;
            if (m_cCurrentBufferIndex >= DEF_MAXSOUNDBUFFERS)
                m_cCurrentBufferIndex = 0;

            if (m_lpDSB[m_cCurrentBufferIndex] != NULL) {
                rval = m_lpDSB[m_cCurrentBufferIndex]->GetStatus(&Status);
                if (rval < 0) Status = 0;

                if (Status & DSBSTATUS_BUFFERLOST) {
                    // Alternate buffer lost - recreate
                    m_lpDSB[m_cCurrentBufferIndex]->Release();
                    m_lpDSB[m_cCurrentBufferIndex] = NULL;
                    bCreateBuffer_LoadWavFileContents(m_cCurrentBufferIndex);
                }
                else if ((Status & DSBSTATUS_PLAYING) == DSBSTATUS_PLAYING) {
                    // Both buffers playing - stop alternate
                    m_lpDSB[m_cCurrentBufferIndex]->Stop();
                    m_lpDSB[m_cCurrentBufferIndex]->SetCurrentPosition(0);
                }
            }
            else {
                // Alternate buffer not created - create it now
                bCreateBuffer_LoadWavFileContents(m_cCurrentBufferIndex);
            }
        }

        Buffer = m_lpDSB[m_cCurrentBufferIndex];
    }
    else {
        // No buffer exists - create on demand
        bCreateBuffer_LoadWavFileContents(m_cCurrentBufferIndex);
        Buffer = m_lpDSB[m_cCurrentBufferIndex];
    }

    m_dwTime = timeGetTime();
    return Buffer;
}
```

**Double-Buffering Logic:**
1. Check if current buffer is playing
2. If single-load mode: stop and reuse same buffer
3. If double-buffer mode: switch to alternate buffer
4. Handle `DSBSTATUS_BUFFERLOST` by recreating buffer
5. Lazy-load buffers on first access

### Volume Control

```cpp
void CSoundBuffer::SetVolume(LONG Volume)
{
    int i;

    for (i = 0; i < DEF_MAXSOUNDBUFFERS; i++)
        if (m_lpDSB[i] != NULL)
            m_lpDSB[i]->SetVolume(Volume);
}
```

**Volume Range:**
- DirectSound volume is in hundredths of decibels (dB)
- Range: `-10000` (silent) to `0` (full volume)
- This is a logarithmic scale matching human hearing

### Stop Method

```cpp
void CSoundBuffer::bStop(BOOL bIsNoRewind)
{
    for (int i = 0; i < DEF_MAXSOUNDBUFFERS; i++)
    {
        if (m_lpDSB[i] != NULL)
        {
            if(m_lpDSB[i]->Stop() != DS_OK) return;
            if (bIsNoRewind == FALSE)
                m_lpDSB[i]->SetCurrentPosition(0);
        }
    }
}
```

### Release Buffer Method

```cpp
void CSoundBuffer::_ReleaseSoundBuffer()
{
    int i;

    for (i = 0; i < DEF_MAXSOUNDBUFFERS; i++)
        if (m_lpDSB[i] != NULL) {
            m_lpDSB[i]->Release();
            m_lpDSB[i] = NULL;
        }
}
```

**Note:** This releases buffers but keeps CSoundBuffer object alive for potential re-creation later. Used for memory management when sounds haven't been played recently.

---

## Sound Effect ID Constants (SoundID.h)

```cpp
#define DEF_SOUND_SHORTSWORDATTACK    0   // Short sword swing sound
#define DEF_SOUND_LONGSWORDATTACK     1   // Long sword swing sound
#define DEF_SOUND_BOWAIMING           2   // Bow draw/aiming sound
#define DEF_SOUND_BOWSHOOT            3   // Arrow release sound
#define DEF_SOUND_AXEATTACK           4   // Axe swing sound
#define DEF_SOUND_MENDAMAGE           5   // Male character hurt grunt
#define DEF_SOUND_WOMENDAMAGE         6   // Female character hurt grunt
#define DEF_SOUND_WALKLAND            7   // Footstep on land
#define DEF_SOUND_WALKGLASS           8   // Footstep on glass/crystal
#define DEF_SOUND_RUNLAND             9   // Running footstep on land
#define DEF_SOUND_RUNGLASS           10   // Running footstep on glass
#define DEF_SOUND_MENDYING           11   // Male death sound
#define DEF_SOUND_WOMENDYING         12   // Female death sound
#define DEF_SOUND_BAREHANDHIT        13   // Unarmed attack impact
#define DEF_SOUND_SWORDHIT           14   // Sword impact sound
#define DEF_SOUND_MACEHIT            15   // Mace/blunt impact
#define DEF_SOUND_ARROWHIT           16   // Arrow impact sound
```

**Note:** These constants are defined but may not be directly used in the codebase. The actual PlaySound calls use numeric indices directly.

---

## Integration with CGame Class

### Sound Member Variables in Game.h

```cpp
// Game.h - Line 632-636, 703-704, 871
class YWSound m_DSound;                              // DirectSound manager
class CSoundBuffer* m_pCSound[DEF_MAXSOUNDEFFECTS];  // Character sounds (C1-C24.wav)
class CSoundBuffer* m_pMSound[DEF_MAXSOUNDEFFECTS];  // Monster sounds (M1-M98.wav)
class CSoundBuffer* m_pESound[DEF_MAXSOUNDEFFECTS];  // Effect sounds (E1-E47.wav)
class CSoundBuffer* m_pBGM;                          // Background music (looping)

BOOL m_bSoundFlag;       // TRUE if DirectSound initialized successfully
BOOL m_bSoundStat;       // Sound effects enabled/disabled
BOOL m_bMusicStat;       // Background music enabled/disabled

char m_cSoundVolume;     // Sound effect volume (0-100)
char m_cMusicVolume;     // Music volume (0-100)
```

### Constants

```cpp
#define DEF_MAXSOUNDEFFECTS  110  // Maximum sounds per category
```

### Initialization in CGame Constructor

```cpp
// Game.cpp - Line 492-494
m_pBGM = NULL;
for (i = 0; i < DEF_MAXSOUNDEFFECTS; i++) {
    m_pCSound[i] = NULL;
    // m_pMSound and m_pESound also initialized to NULL
}

// Line 791
m_bSoundFlag = FALSE;

// Line 956-957
m_cSoundVolume = 100;  // Default to maximum
m_cMusicVolume = 100;
```

### DirectSound Initialization

```cpp
// Game.cpp - Line 866-867
m_bSoundFlag = m_DSound.Create(m_hWnd);
m_bMusicStat = m_bSoundStat = m_bSoundFlag;
```

### Sound Loading During Game Load

```cpp
// Game.cpp - Line 4229-4244 (during DEF_GAMEMODE_ONLOADING)
if (m_bSoundFlag) {
    // Load 24 character sounds (C1.wav - C24.wav)
    for (i = 1; i <= 24; i++) {
        wsprintf(G_cTxt, "sounds\\C%d.wav", i);
        m_pCSound[i] = new class CSoundBuffer(m_DSound.m_lpDS, m_DSound.m_DSCaps, G_cTxt);
    }

    // Load 98 monster sounds (M1.wav - M98.wav)
    for (i = 1; i <= 98; i++) {
        wsprintf(G_cTxt, "sounds\\M%d.wav", i);
        m_pMSound[i] = new class CSoundBuffer(m_DSound.m_lpDS, m_DSound.m_DSCaps, G_cTxt);
    }

    // Load 47 effect sounds (E1.wav - E47.wav)
    for (i = 1; i <= 47; i++) {
        wsprintf(G_cTxt, "sounds\\E%d.wav", i);
        m_pESound[i] = new class CSoundBuffer(m_DSound.m_lpDS, m_DSound.m_DSCaps, G_cTxt);
    }
}
```

### Cleanup

```cpp
// Game.cpp - Line 1037-1043
for (i = 0; i < DEF_MAXSOUNDEFFECTS; i++) {
    if (m_pCSound[i] != NULL) delete m_pCSound[i];
    // Similar cleanup for m_pMSound and m_pESound
}
if (m_pBGM != NULL) delete m_pBGM;
```

---

## Sound Categories

### Category 'C' - Character Sounds (24 sounds)

Located in `sounds\C*.wav`

| Index | Confirmed Usage | Context |
|-------|-----------------|---------|
| C4 | Arrow shot | Bow attack animation |
| C12 | Female character | Death/defeat scenario |
| C13 | Female character | Death/defeat scenario |
| C17 | Character sound | Specific event |
| C19 | Character sound | Combat related |
| C20 | Character sound | Combat related |
| C21 | Male character | Victory/level-up fanfare |
| C22 | Male character | Victory/level-up fanfare |

### Category 'M' - Monster/Music Sounds (98 sounds)

Located in `sounds\M*.wav`

**Note:** Despite being named "MSound" and loaded as monster sounds, no direct `PlaySound('M', ...)` calls were found in the codebase. These may be used indirectly or through different mechanisms.

### Category 'E' - Effect Sounds (47 sounds)

Located in `sounds\E*.wav`

| Index | Sound Effect | Context |
|-------|-------------|---------|
| E1 | Generic impact | Various combat effects |
| E2 | Energy/Lightning | Energy Bolt, Lightning Arrow spell |
| E3 | Magic missile | Magic Missile explosion |
| E4 | Fire/Explosion | FireBall spell, explosions |
| E5 | Heavy impact | Large weapon impacts |
| E12 | Coin/Gold | Gold pickup effect |
| E14 | UI Click | Button clicks, dialog interactions |
| E23 | Victory fanfare | Crusade victory |
| E24 | Defeat sound | Crusade defeat |
| E25 | Neutral event | Crusade end, announcements |
| E38 | Rain/Weather | Ambient rain loop (special handling) |
| E40 | Combat effect | Specific attack type |
| E42 | Combat effect | Specific attack type |
| E44 | Special action | Item-related action |
| E45 | Combat sound | Weapon impact |
| E46 | Combat sound | Weapon impact |
| E47 | Combat sound | Heavy impact |

---

## PlaySound Function

### Method Signature

```cpp
void CGame::PlaySound(char cType, int iNum, int iDist, long lPan = 0);
```

### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `cType` | `char` | Sound category: 'C' (Character), 'M' (Monster), 'E' (Effect) |
| `iNum` | `int` | Sound index within category (1-based) |
| `iDist` | `int` | Distance from listener (0-10), affects volume attenuation |
| `lPan` | `long` | Stereo panning (-10000 left to +10000 right), default 0 (center) |

### Implementation

```cpp
void CGame::PlaySound(char cType, int iNum, int iDist, long lPan)
{
    int iVol;

    // Early exit if sound system not available or disabled
    if (m_bSoundFlag == FALSE) return;
    if (m_bSoundStat == FALSE) return;

    // Clamp distance to maximum of 10
    if (iDist > 10) iDist = 10;

    // Calculate volume based on user setting and distance
    // m_cSoundVolume is 0-100, subtract 100 and multiply by 20
    // gives range of -2000 to 0 for base volume
    iVol = (m_cSoundVolume - 100) * 20;

    // Distance attenuation: -200 dB per unit distance
    iVol += -200 * iDist;

    // Clamp volume to valid DirectSound range
    if (iVol > 0) iVol = 0;
    if (iVol < -10000) iVol = -10000;

    // Only play if loud enough to hear (> -2000 dB)
    if (iVol > -2000) {
        switch (cType) {
        case 'C':  // Character sounds
            if (m_pCSound[iNum] == NULL) return;
            m_pCSound[iNum]->Play(FALSE, lPan, iVol);
            break;

        case 'M':  // Monster sounds
            if (m_pMSound[iNum] == NULL) return;
            m_pMSound[iNum]->Play(FALSE, lPan, iVol);
            break;

        case 'E':  // Effect sounds
            if (m_pESound[iNum] == NULL) return;
            m_pESound[iNum]->Play(FALSE, lPan, iVol);
            break;
        }
    }
}
```

### Volume Calculation

The volume calculation uses DirectSound's decibel-based system:

```
Base Volume = (UserVolume - 100) * 20
            = (0 to 100 - 100) * 20
            = -2000 to 0

Distance Attenuation = -200 * Distance
                     = -200 * (0 to 10)
                     = 0 to -2000

Final Volume = Base + Attenuation
             = Range of -4000 to 0

Clamped to: -10000 to 0 (DirectSound valid range)
Cutoff at: -2000 (sounds quieter than this are not played)
```

### Panning Calculation

Stereo panning is calculated based on screen position:

```cpp
// Calculate pan based on X position relative to screen center
lPan = -(((m_sViewPointX / 32) + 10) - sX) * 1000;
// or
lPan = -(320 - (sX - m_sViewPointX)) * 1000;
```

- Negative values = sound from left speaker
- Positive values = sound from right speaker
- Multiplier of 1000 scales tile position to pan range

---

## Background Music (BGM) System

### StartBGM Function

```cpp
void CGame::StartBGM()
{
    if (m_bSoundFlag == FALSE)
    {
        if (m_pBGM != NULL) {
            m_pBGM->bStop();
            delete m_pBGM;
            m_pBGM = NULL;
        }
        return;
    }

    char cWavFileName[32];
    ZeroMemory(cWavFileName, sizeof(cWavFileName));

    // Christmas event special music
    #ifdef DEF_XMAS
    if (m_cWhetherEffectType >= 4 && m_cWhetherEffectType <= 6)
        strcpy(cWavFileName, "music\\Carol.wav");
    else
    #endif
    {
        // Location-based music selection
        if (memcmp(m_cCurLocation, "aresden", 7) == 0)
            strcpy(cWavFileName, "music\\aresden.wav");
        else if (memcmp(m_cCurLocation, "elvine", 6) == 0)
            strcpy(cWavFileName, "music\\elvine.wav");
        else if (memcmp(m_cCurLocation, "dglv", 4) == 0)
            strcpy(cWavFileName, "music\\dungeon.wav");
        else if (memcmp(m_cCurLocation, "middled1", 8) == 0)
            strcpy(cWavFileName, "music\\dungeon.wav");
        else if (memcmp(m_cCurLocation, "middleland", 10) == 0)
            strcpy(cWavFileName, "music\\middleland.wav");
        else
            strcpy(cWavFileName, "music\\MainTm.wav");
    }

    // Avoid reloading same track
    if (m_pBGM != NULL) {
        if (strcmp(m_pBGM->m_cWavFileName, cWavFileName) == 0)
            return;
        m_pBGM->bStop();
        delete m_pBGM;
        m_pBGM = NULL;
    }

    // Calculate volume
    int iVolume = (m_cMusicVolume - 100) * 20;
    if (iVolume > 0) iVolume = 0;
    if (iVolume < -10000) iVolume = -10000;

    // Create and play (TRUE = single-instance mode for looping)
    m_pBGM = new class CSoundBuffer(m_DSound.m_lpDS, m_DSound.m_DSCaps,
                                     cWavFileName, TRUE);
    m_pBGM->Play(TRUE, 0, iVolume);  // TRUE = loop
}
```

### Music Files by Location

| Location Prefix | Music File | Description |
|-----------------|------------|-------------|
| `aresden` | `music\aresden.wav` | Aresden city theme |
| `elvine` | `music\elvine.wav` | Elvine city theme |
| `dglv` | `music\dungeon.wav` | Dungeon level theme |
| `middled1` | `music\dungeon.wav` | Middle dungeon theme |
| `middleland` | `music\middleland.wav` | Middleland area theme |
| (default) | `music\MainTm.wav` | Main/default theme |
| (Christmas) | `music\Carol.wav` | Christmas event music |

### BGM Control in System Menu

```cpp
// Game.cpp - Line 41797
if (m_bMusicStat)
    PutString(sX + 180, sY + 85, DRAW_DIALOGBOX_SYSMENU_ON, RGB(255,255,255));

// Volume slider rendering - Line 41815-41819
DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX + 130 + m_cSoundVolume, sY + 129, 8);
DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX + 130 + m_cMusicVolume, sY + 145, 8);

// Volume slider interaction - Line 41921-41937
m_cSoundVolume = msX - (sX + 127);
if (m_cSoundVolume > 100) m_cSoundVolume = 100;
if (m_cSoundVolume < 0) m_cSoundVolume = 0;

m_cMusicVolume = msX - (sX + 127);
if (m_cMusicVolume > 100) m_cMusicVolume = 100;
if (m_cMusicVolume < 0) m_cMusicVolume = 0;

// Apply volume change immediately
if (m_pBGM != NULL) {
    iVol = (m_cMusicVolume - 100) * 20;
    m_pBGM->bStop(TRUE);  // TRUE = don't rewind
    m_pBGM->Play(FALSE, 0, iVol);
}
```

### Music Toggle

```cpp
// Toggle music on/off - Game.cpp Line 28696-28726, 33323-33341
if (m_bMusicStat == TRUE) {
    m_bMusicStat = FALSE;
    if (m_pBGM != NULL) {
        m_pBGM->bStop();
        delete m_pBGM;
        m_pBGM = NULL;
    }
}
else {
    m_bMusicStat = TRUE;
    StartBGM();
}
```

---

## Weather Sound System

### Rain Ambient Sound (E38)

The rain sound (Effect #38) has special handling as a looping ambient sound:

```cpp
// Start rain sound when weather effect begins
// Game.cpp - Line 23265
if ((m_bSoundStat == TRUE) && (m_bSoundFlag) && (cType >= 1) && (cType <= 3))
    m_pESound[38]->Play(TRUE);  // TRUE = loop

// Stop rain sound when weather ends
// Game.cpp - Line 23283
if ((m_bSoundStat == TRUE) && (m_bSoundFlag))
    m_pESound[38]->bStop();
```

### Weather Sound Stop Points

Rain sound is stopped during various transitions:

```cpp
// Game mode changes
if (m_bSoundFlag) m_pESound[38]->bStop();  // Multiple locations
```

Locations where rain is stopped:
- Line 4316: During loading transitions
- Line 25782: Connection lost
- Line 28710: System menu toggle
- Line 31534: Game mode change
- Line 33310: System menu toggle
- Line 34758: Map change
- Line 35338: Map change
- Line 35925: Map change
- Line 43536: Map change

---

## Sound Cache Management

### Automatic Buffer Release

Sounds that haven't played recently are released to free memory:

```cpp
// Game.cpp - Line 15649-15659 (in ReleaseUnusedSprites)
for (i = 0; i < DEF_MAXSOUNDEFFECTS; i++) {
    if (m_pCSound[i] != NULL) {
        // Release if not played in 30 seconds AND not looping
        if (((G_dwGlobalTime - m_pCSound[i]->m_dwTime) > 30000) &&
            (m_pCSound[i]->m_bIsLooping == FALSE))
            m_pCSound[i]->_ReleaseSoundBuffer();
    }
    // Same for m_pMSound and m_pESound
}
```

**Key Points:**
- 30-second timeout for unused sounds
- Looping sounds are never released automatically
- CSoundBuffer object persists, only DirectSound buffers released
- Buffers recreated on next Play() call (lazy loading)

---

## Common Sound Usage Patterns

### UI Click Sound

The most common sound is E14 (UI click), used extensively for button interactions:

```cpp
PlaySound('E', 14, 5);  // Distance 5 = moderate volume
```

Used in:
- Dialog button clicks
- Menu interactions
- Item actions
- Spell casting confirmations
- All dialog box interactions

### Combat Sounds

```cpp
// Arrow shot
PlaySound('C', 4, sDist);

// Spell explosions
PlaySound('E', 4, sDist, lPan);  // FireBall
PlaySound('E', 2, sDist, lPan);  // Energy Bolt
PlaySound('E', 3, sDist, lPan);  // Magic Missile

// Impact sounds
PlaySound('E', 1, sDist, lPan);  // Generic impact
PlaySound('E', 5, sDist, lPan);  // Heavy impact
```

### Victory/Defeat Sounds

```cpp
// Victory (Crusade win)
PlaySound('E', 23, 0, 0);
PlaySound('C', 21, 0, 0);
PlaySound('C', 22, 0, 0);

// Defeat (Crusade loss)
PlaySound('E', 24, 0, 0);
PlaySound('C', 12, 0, 0);
PlaySound('C', 13, 0, 0);

// Neutral announcement
PlaySound('E', 25, 0, 0);
```

---

## File Structure Summary

### Sound Directories

```
client/
├── sounds/
│   ├── C1.wav - C24.wav    (24 character sounds)
│   ├── M1.wav - M98.wav    (98 monster sounds)
│   └── E1.wav - E47.wav    (47 effect sounds)
└── music/
    ├── aresden.wav         (Aresden city theme)
    ├── elvine.wav          (Elvine city theme)
    ├── dungeon.wav         (Dungeon theme)
    ├── middleland.wav      (Middleland theme)
    ├── MainTm.wav          (Main menu/default theme)
    └── Carol.wav           (Christmas event music)
```

### Total Sound Assets

| Category | Count | File Pattern |
|----------|-------|--------------|
| Character Sounds | 24 | C1.wav - C24.wav |
| Monster Sounds | 98 | M1.wav - M98.wav |
| Effect Sounds | 47 | E1.wav - E47.wav |
| Music Tracks | 6 | Various .wav files |
| **Total** | **175** | |

---

## Technical Limitations

### Known Issues

1. **Simplified WAV Parser**: The `Waveheader` structure assumes a specific WAV format. Files with extra chunks, different formats, or metadata may fail to load.

2. **No Streaming**: All audio is loaded entirely into memory. Large music files consume significant RAM.

3. **2-Buffer Limit**: Only 2 simultaneous instances of the same sound can play. Rapid-fire sounds may cut off.

4. **No 3D Positional Audio**: Despite DirectSound 7 supporting 3D audio, the implementation only uses 2D pan.

5. **Fixed Sample Rate**: Primary buffer is set to 44.1kHz. Lower-quality sounds are upsampled.

6. **No Error Recovery**: Many error paths return FALSE without cleanup or user notification.

7. **Memory Leak Potential**: Buffer lost recovery creates new buffers without releasing failed ones properly.

### DirectSound 7 vs Modern APIs

| Feature | DirectSound 7 | Modern Alternative |
|---------|---------------|-------------------|
| API Age | ~2000 | XAudio2 (2008+), WASAPI |
| 3D Audio | Limited | Full spatial audio |
| Latency | Higher | Low latency support |
| Format Support | WAV/PCM | Multiple codecs |
| Cross-Platform | Windows only | SDL2, OpenAL |

---

## Modernization Recommendations

1. **Replace DirectSound**: Use XAudio2 (Windows) or SFML/SDL_mixer (cross-platform)

2. **Implement Streaming**: Music should stream from disk, not load entirely

3. **Use Resource Manager**: Centralized sound loading with reference counting

4. **Add Sound Banks**: Group related sounds for efficient loading

5. **Implement Sound Prioritization**: Drop low-priority sounds when channels are full

6. **Add Proper Error Handling**: Log errors, provide fallback behavior

7. **Support Modern Formats**: OGG Vorbis for music, compressed formats for SFX

8. **Implement 3D Audio**: Use listener/source model for spatial audio

---

## Appendix A: DirectSound Constants Reference

### Buffer Status Flags (DSBSTATUS_*)

| Constant | Value | Description |
|----------|-------|-------------|
| `DSBSTATUS_PLAYING` | 0x00000001 | Buffer is currently playing |
| `DSBSTATUS_BUFFERLOST` | 0x00000002 | Buffer memory was lost |
| `DSBSTATUS_LOOPING` | 0x00000004 | Buffer is looping |

### Cooperative Level (DSSCL_*)

| Constant | Value | Description |
|----------|-------|-------------|
| `DSSCL_NORMAL` | 0x00000001 | Standard cooperative level |
| `DSSCL_PRIORITY` | 0x00000002 | Priority level (allows format changes) |
| `DSSCL_EXCLUSIVE` | 0x00000003 | Exclusive access (deprecated) |
| `DSSCL_WRITEPRIMARY` | 0x00000004 | Write to primary buffer |

### Pan Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `DSBPAN_LEFT` | -10000 | Full left pan |
| `DSBPAN_CENTER` | 0 | Center (balanced) |
| `DSBPAN_RIGHT` | +10000 | Full right pan |

### Volume Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `DSBVOLUME_MIN` | -10000 | Minimum volume (silent) |
| `DSBVOLUME_MAX` | 0 | Maximum volume (full) |

---

## Appendix B: Complete Sound Effect Index

### Character Sounds (C-Series) - Known Mappings

| Index | Known Usage |
|-------|-------------|
| C4 | Bow/Arrow shot |
| C12 | Female death/defeat |
| C13 | Female death/defeat (secondary) |
| C17 | Character action |
| C19 | Combat sound |
| C20 | Combat sound |
| C21 | Male victory/level-up |
| C22 | Male victory/level-up (secondary) |

### Effect Sounds (E-Series) - Known Mappings

| Index | Sound Effect |
|-------|-------------|
| E1 | Generic impact/hit |
| E2 | Energy Bolt / Lightning Arrow |
| E3 | Magic Missile explosion |
| E4 | FireBall explosion |
| E5 | Heavy impact |
| E12 | Gold/coin pickup |
| E14 | UI button click |
| E23 | Victory fanfare |
| E24 | Defeat sound |
| E25 | Neutral announcement |
| E38 | Rain ambient (looping) |
| E40 | Combat effect |
| E42 | Combat effect |
| E44 | Item action |
| E45 | Weapon impact |
| E46 | Weapon impact |
| E47 | Heavy weapon impact |

---

## Appendix C: Code Cross-References

### Files That Use Audio System

| File | Usage |
|------|-------|
| `Game.cpp` | Main audio integration, PlaySound calls |
| `Game.h` | Audio member declarations |
| `YWSound.cpp/h` | DirectSound initialization |
| `SoundBuffer.cpp/h` | Individual sound management |
| `SoundID.h` | Sound constant definitions |

### Key Functions

| Function | Location | Purpose |
|----------|----------|---------|
| `YWSound::Create` | YWSound.cpp:26 | Initialize DirectSound |
| `CSoundBuffer::Play` | SoundBuffer.cpp:177 | Play a sound |
| `CSoundBuffer::GetIdleBuffer` | SoundBuffer.cpp:204 | Double-buffer management |
| `CGame::PlaySound` | Game.cpp:22783 | High-level sound API |
| `CGame::StartBGM` | Game.cpp:35458 | Start background music |
| `ReleaseUnusedSprites` | Game.cpp:15649 | Sound cache cleanup |
