# Legacy Documentation: XSocket Networking System

## Overview

The `XSocket` class is the foundational networking layer for the Helbreath client, providing a C++ wrapper around the WinSock2 API. It implements asynchronous TCP/IP socket communication using Windows message-based event notification (`WSAAsyncSelect`). The class handles connection management, message framing, encryption, and buffering of unsent data during network congestion.

**Source Files:**
- `XSocket.h` - Class declaration and constants
- `XSocket.cpp` - Implementation

**Dependencies:**
- WinSock2 API (`winsock2.h`)
- Windows API (`windows.h`, `windowsx.h`, `winbase.h`)
- Standard C libraries (`stdlib.h`, `stdio.h`, `memory.h`, `malloc.h`)

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                         CGame Class                              │
│  ┌─────────────┐                           ┌─────────────┐      │
│  │  m_pLSock   │ ─── Login Server Socket   │  m_pGSock   │      │
│  │  (XSocket)  │                           │  (XSocket)  │      │
│  └──────┬──────┘                           └──────┬──────┘      │
│         │                                         │              │
│         ▼                                         ▼              │
│  WM_USER_LOGSOCKETEVENT                 WM_USER_GAMESOCKETEVENT  │
│         │                                         │              │
│         ▼                                         ▼              │
│  OnLogSocketEvent()                      OnGameSocketEvent()     │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      XSocket Class                               │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Message Framing Protocol                                 │   │
│  │  ┌─────────┬────────────────┬─────────────────────────┐  │   │
│  │  │ Key (1) │ Size (2 bytes) │ Payload (variable)      │  │   │
│  │  └─────────┴────────────────┴─────────────────────────┘  │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              │                                   │
│  ┌───────────────────────────┴───────────────────────────────┐  │
│  │  Unsent Data Queue (Circular Buffer)                       │  │
│  │  ┌────┬────┬────┬────┬────┬────┬─────────────────────┐    │  │
│  │  │ 0  │ 1  │ 2  │... │298 │299 │ (DEF_XSOCKBLOCKLIMIT)│   │  │
│  │  └────┴────┴────┴────┴────┴────┴─────────────────────┘    │  │
│  │    ▲                                ▲                      │  │
│  │    │ m_sHead                        │ m_sTail              │  │
│  └───────────────────────────────────────────────────────────┘  │
│                              │                                   │
│                              ▼                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  WinSock2 TCP Socket (SOCK_STREAM)                        │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

---

## Constants and Definitions

### Socket Type Constants

```cpp
#define DEF_XSOCK_LISTENSOCK         1    // Server listening socket
#define DEF_XSOCK_NORMALSOCK         2    // Normal connected socket
#define DEF_XSOCK_SHUTDOWNEDSOCK     3    // Socket has been shut down
```

**Purpose:** These constants identify the operational mode of an XSocket instance.

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_XSOCK_LISTENSOCK` | 1 | Socket is in listening mode (server-side, accepts connections) |
| `DEF_XSOCK_NORMALSOCK` | 2 | Socket is connected and operational |
| `DEF_XSOCK_SHUTDOWNEDSOCK` | 3 | Socket has been closed or connection lost |

### Read Status Constants

```cpp
#define DEF_XSOCKSTATUS_READINGHEADER    11    // Currently reading packet header
#define DEF_XSOCKSTATUS_READINGBODY      12    // Currently reading packet body
```

**Purpose:** Track the current state of the packet reading state machine.

| Constant | Value | Description |
|----------|-------|-------------|
| `DEF_XSOCKSTATUS_READINGHEADER` | 11 | Socket is reading the 3-byte packet header |
| `DEF_XSOCKSTATUS_READINGBODY` | 12 | Socket is reading the packet payload |

### Socket Event Return Codes

```cpp
#define DEF_XSOCKEVENT_SOCKETMISMATCH           -121  // Socket mismatch in event
#define DEF_XSOCKEVENT_CONNECTIONESTABLISH      -122  // Connection established
#define DEF_XSOCKEVENT_RETRYINGCONNECTION       -123  // Retrying connection
#define DEF_XSOCKEVENT_ONREAD                   -124  // Currently reading message
#define DEF_XSOCKEVENT_READCOMPLETE             -125  // Message read complete
#define DEF_XSOCKEVENT_UNKNOWN                  -126  // Unknown event
#define DEF_XSOCKEVENT_SOCKETCLOSED             -127  // Socket closed by remote
#define DEF_XSOCKEVENT_BLOCK                    -128  // Operation would block
#define DEF_XSOCKEVENT_SOCKETERROR              -129  // Socket error occurred
#define DEF_XSOCKEVENT_CRITICALERROR            -130  // Critical error, must terminate
#define DEF_XSOCKEVENT_NOTINITIALIZED           -131  // Class not initialized
#define DEF_XSOCKEVENT_MSGSIZETOOLARGE          -132  // Message too large for buffer
#define DEF_XSOCKEVENT_CONFIRMCODENOTMATCH      -133  // Confirmation code mismatch
#define DEF_XSOCKEVENT_QUENEFULL                -134  // Send queue is full
#define DEF_XSOCKEVENT_UNSENTDATASENDBLOCK      -135  // Blocked while sending queued data
#define DEF_XSOCKEVENT_UNSENTDATASENDCOMPLETE   -136  // All queued data sent
```

**Detailed Event Descriptions:**

| Event Code | Value | Description | CGame Response |
|------------|-------|-------------|----------------|
| `SOCKETMISMATCH` | -121 | The socket handle in the event doesn't match this instance | Ignored |
| `CONNECTIONESTABLISH` | -122 | TCP connection successfully established | Call `ConnectionEstablishHandler()` |
| `RETRYINGCONNECTION` | -123 | Connection failed, automatically retrying | Wait for next event |
| `ONREAD` | -124 | Partial read in progress, more data expected | Wait for `READCOMPLETE` |
| `READCOMPLETE` | -125 | Complete message received, ready for processing | Call `GameRecvMsgHandler()` or `LogRecvMsgHandler()` |
| `UNKNOWN` | -126 | Unrecognized WinSock event | Ignored |
| `SOCKETCLOSED` | -127 | Remote end closed the connection | Transition to `DEF_GAMEMODE_ONCONNECTIONLOST` |
| `BLOCK` | -128 | Send operation would block (non-blocking mode) | Data queued for later |
| `SOCKETERROR` | -129 | Socket error occurred (check `m_WSAErr`) | Transition to `DEF_GAMEMODE_ONCONNECTIONLOST` |
| `CRITICALERROR` | -130 | Fatal error, application should terminate | Close all sockets, clean up |
| `NOTINITIALIZED` | -131 | Operation attempted on uninitialized socket | Programming error |
| `MSGSIZETOOLARGE` | -132 | Message exceeds buffer capacity | Message rejected |
| `CONFIRMCODENOTMATCH` | -133 | Protocol validation failed | Connection should be terminated |
| `QUENEFULL` | -134 | Unsent data queue is full | Connection should be terminated |
| `UNSENTDATASENDBLOCK` | -135 | Blocked while flushing queue | Wait for `FD_WRITE` event |
| `UNSENTDATASENDCOMPLETE` | -136 | All queued data successfully sent | Resume normal operation |

### Buffer and Queue Limits

```cpp
#define DEF_XSOCKBLOCKLIMIT    300    // Maximum unsent data queue entries
```

**Note:** The actual block limit is configurable per-instance via the constructor parameter `iBlockLimit`. The default value of 300 is defined in `XSocket.h`, but the client uses `DEF_SOCKETBLOCKLIMIT` (also 300) defined in `Game.h`.

---

## Class Definition

### Member Variables

```cpp
class XSocket
{
public:
    // Connection State
    int    m_WSAErr;              // Last WinSock error code
    BOOL   m_bIsAvailable;        // TRUE when connection is established
    BOOL   m_bIsWriteEnabled;     // TRUE when socket is ready for writing

    // Socket Configuration
    char   m_cType;               // Socket type (LISTEN/NORMAL/SHUTDOWN)
    SOCKET m_Sock;                // WinSock socket handle
    char   m_pAddr[30];           // Server address string
    int    m_iPortNum;            // Server port number
    unsigned int m_uiMsg;         // Windows message ID for async events
    HWND   m_hWnd;                // Window handle for message delivery
    int    m_iBlockLimit;         // Maximum queue entries (configurable)

    // Read State Machine
    char   m_cStatus;             // Current read status (HEADER/BODY)
    DWORD  m_dwReadSize;          // Bytes remaining to read
    DWORD  m_dwTotalReadSize;     // Bytes read so far for current message

    // Buffers
    char * m_pRcvBuffer;          // Receive buffer (dynamically allocated)
    char * m_pSndBuffer;          // Send buffer (dynamically allocated)
    DWORD  m_dwBufferSize;        // Size of each buffer

    // Unsent Data Queue (Circular Buffer)
    char * m_pUnsentDataList[DEF_XSOCKBLOCKLIMIT];  // Array of data pointers
    int    m_iUnsentDataSize[DEF_XSOCKBLOCKLIMIT];  // Size of each entry
    short  m_sHead;               // Queue head index (next to send)
    short  m_sTail;               // Queue tail index (next to add)
};
```

### Memory Layout Visualization

```
XSocket Instance
├── Connection State (12 bytes)
│   ├── m_WSAErr (4 bytes)
│   ├── m_bIsAvailable (4 bytes)
│   └── m_bIsWriteEnabled (4 bytes)
│
├── Socket Configuration (~50 bytes)
│   ├── m_cType (1 byte + padding)
│   ├── m_Sock (4/8 bytes)
│   ├── m_pAddr[30] (30 bytes)
│   ├── m_iPortNum (4 bytes)
│   ├── m_uiMsg (4 bytes)
│   ├── m_hWnd (4/8 bytes)
│   └── m_iBlockLimit (4 bytes)
│
├── Read State (12 bytes)
│   ├── m_cStatus (1 byte + padding)
│   ├── m_dwReadSize (4 bytes)
│   └── m_dwTotalReadSize (4 bytes)
│
├── Buffers (~20 bytes)
│   ├── m_pRcvBuffer (4/8 bytes) → [dwBufferSize+8 bytes]
│   ├── m_pSndBuffer (4/8 bytes) → [dwBufferSize+8 bytes]
│   └── m_dwBufferSize (4 bytes)
│
└── Unsent Queue (~3604 bytes)
    ├── m_pUnsentDataList[300] (1200/2400 bytes)
    ├── m_iUnsentDataSize[300] (1200 bytes)
    ├── m_sHead (2 bytes)
    └── m_sTail (2 bytes)
```

---

## Custom Memory Management

The XSocket class overrides the default `new` and `delete` operators to use the Windows heap API:

```cpp
void * operator new (size_t size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
};

void operator delete(void * mem)
{
    HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, mem);
};
```

**Rationale:**
- `HEAP_ZERO_MEMORY` ensures all memory is initialized to zero
- `HEAP_NO_SERIALIZE` on delete allows faster deallocation (no thread synchronization)
- Uses process heap rather than CRT heap for potentially better memory management

**Implications:**
- All XSocket instances are zero-initialized automatically
- Cannot mix with standard `new`/`delete` for derived classes
- Memory is not exception-safe (no RAII)

---

## Packet Protocol Format

### Message Frame Structure

The XSocket class implements a simple length-prefixed message framing protocol:

```
┌─────────────┬──────────────────┬─────────────────────────────────┐
│   Key (1)   │   Size (2 bytes) │   Payload (Size - 3 bytes)      │
├─────────────┼──────────────────┼─────────────────────────────────┤
│   Byte 0    │   Bytes 1-2      │   Bytes 3 to (Size-1)           │
└─────────────┴──────────────────┴─────────────────────────────────┘
```

| Field | Offset | Size | Description |
|-------|--------|------|-------------|
| Key | 0 | 1 byte | Encryption key (0 = no encryption, 1-255 = XOR key) |
| Size | 1 | 2 bytes (WORD) | Total message size including header (little-endian) |
| Payload | 3 | Size - 3 bytes | Encrypted or plaintext message data |

### Header Details

- **Minimum message size:** 3 bytes (header only, no payload)
- **Maximum message size:** Limited by `m_dwBufferSize` (typically 30,000 bytes)
- **Byte order:** Little-endian (Intel x86 native)

### Encryption Algorithm

When `cKey != 0`, the payload is encrypted using a position-dependent XOR algorithm:

**Encryption (sending):**
```cpp
for (i = 0; i < dwSize; i++) {
    m_pSndBuffer[3+i] += (i ^ cKey);
    m_pSndBuffer[3+i] = (char)(m_pSndBuffer[3+i] ^ (cKey ^ (dwSize - i)));
}
```

**Decryption (receiving):**
```cpp
for (i = 0; i < dwSize; i++) {
    m_pRcvBuffer[3+i] = (char)(m_pRcvBuffer[3+i] ^ (cKey ^ (dwSize - i)));
    m_pRcvBuffer[3+i] -= (i ^ cKey);
}
```

**Algorithm Analysis:**
1. Add `(i ^ cKey)` to each byte (where `i` is position)
2. XOR with `(cKey ^ (dwSize - i))`

This creates a simple stream cipher where each byte's transformation depends on:
- Its position in the message (`i`)
- The encryption key (`cKey`)
- The total message size (`dwSize`)

**Security Note:** This is a very weak encryption scheme, easily broken if the plaintext structure is known. It provides obfuscation rather than security.

---

## Constructor and Destructor

### Constructor

```cpp
XSocket::XSocket(HWND hWnd, int iBlockLimit)
{
    register int i;

    m_cType       = NULL;           // No type assigned yet
    m_pRcvBuffer  = NULL;           // Buffers not allocated
    m_pSndBuffer  = NULL;
    m_Sock        = INVALID_SOCKET; // No socket created
    m_dwBufferSize = 0;

    m_cStatus   = DEF_XSOCKSTATUS_READINGHEADER;  // Start reading header
    m_dwReadSize = 3;               // Header is 3 bytes
    m_dwTotalReadSize = 0;

    // Initialize unsent data queue
    for (i = 0; i < DEF_XSOCKBLOCKLIMIT; i++) {
        m_iUnsentDataSize[i] = 0;
        m_pUnsentDataList[i] = NULL;
    }

    m_sHead = 0;
    m_sTail = 0;

    m_WSAErr = NULL;

    m_hWnd = hWnd;                  // Store window handle for events
    m_bIsAvailable = FALSE;         // Not yet connected
    m_bIsWriteEnabled = FALSE;      // Not yet ready to write

    m_iBlockLimit = iBlockLimit;    // Store queue limit
}
```

**Parameters:**
- `hWnd`: Window handle that will receive socket events
- `iBlockLimit`: Maximum number of unsent data entries (typically 300)

**Usage in CGame:**
```cpp
m_pLSock = new class XSocket(m_hWnd, DEF_SOCKETBLOCKLIMIT);
m_pGSock = new class XSocket(m_hWnd, DEF_SOCKETBLOCKLIMIT);
```

### Destructor

```cpp
XSocket::~XSocket()
{
    register int i;

    // Free receive and send buffers
    if (m_pRcvBuffer != NULL) delete[] m_pRcvBuffer;
    if (m_pSndBuffer != NULL) delete[] m_pSndBuffer;

    // Free any unsent data in queue
    for (i = 0; i < DEF_XSOCKBLOCKLIMIT; i++)
        if (m_pUnsentDataList[i] != NULL) delete[] m_pUnsentDataList[i];

    // Close the socket gracefully
    _CloseConn();
}
```

**Cleanup Order:**
1. Free dynamically allocated buffers
2. Free any pending unsent data
3. Gracefully close the TCP connection

---

## Initialization Methods

### bInitBufferSize

```cpp
BOOL XSocket::bInitBufferSize(DWORD dwBufferSize)
{
    // Free existing buffers if any
    if (m_pRcvBuffer != NULL) delete[] m_pRcvBuffer;
    if (m_pSndBuffer != NULL) delete[] m_pSndBuffer;

    // Allocate new buffers with 8 extra bytes for safety
    m_pRcvBuffer = new char[dwBufferSize+8];
    if (m_pRcvBuffer == NULL) return FALSE;

    m_pSndBuffer = new char[dwBufferSize+8];
    if (m_pSndBuffer == NULL) return FALSE;

    m_dwBufferSize = dwBufferSize;

    return TRUE;
}
```

**Purpose:** Allocate send and receive buffers.

**Parameters:**
- `dwBufferSize`: Size of each buffer in bytes

**Returns:** `TRUE` on success, `FALSE` if memory allocation fails.

**Usage:**
```cpp
m_pLSock->bInitBufferSize(30000);  // 30KB buffers
```

**Note:** The extra 8 bytes provide a safety margin for header operations.

---

## Connection Methods

### bConnect (Asynchronous)

```cpp
BOOL XSocket::bConnect(char * pAddr, int iPort, unsigned int uiMsg)
{
    SOCKADDR_IN  saTemp;
    u_long       arg;
    int          iRet;
    DWORD        dwOpt;

    // Cannot call on listening socket
    if (m_cType == DEF_XSOCK_LISTENSOCK) return FALSE;

    // Close existing socket if any
    if (m_Sock != INVALID_SOCKET) closesocket(m_Sock);

    // Create TCP socket
    m_Sock = socket(AF_INET, SOCK_STREAM, 0);
    if (m_Sock == INVALID_SOCKET)
        return FALSE;

    // Set up address structure
    memset(&saTemp, 0, sizeof(saTemp));
    saTemp.sin_family = AF_INET;
    saTemp.sin_addr.s_addr = inet_addr(pAddr);
    saTemp.sin_port = htons(iPort);

    // Register for async events BEFORE connecting
    WSAAsyncSelect(m_Sock, m_hWnd, uiMsg,
                   FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE);

    // Initiate connection (non-blocking)
    iRet = connect(m_Sock, (struct sockaddr *) &saTemp, sizeof(saTemp));
    if (iRet == SOCKET_ERROR) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            m_WSAErr = WSAGetLastError();
            return FALSE;
        }
        // WSAEWOULDBLOCK is expected for async connect
    }

    // Set socket buffer sizes (40KB each)
    dwOpt = 8192 * 5;  // 40,960 bytes
    setsockopt(m_Sock, SOL_SOCKET, SO_RCVBUF, (const char FAR *)&dwOpt, sizeof(dwOpt));
    setsockopt(m_Sock, SOL_SOCKET, SO_SNDBUF, (const char FAR *)&dwOpt, sizeof(dwOpt));

    // Store connection info for potential reconnection
    strcpy(m_pAddr, pAddr);
    m_iPortNum = iPort;
    m_uiMsg = uiMsg;
    m_cType = DEF_XSOCK_NORMALSOCK;

    return TRUE;
}
```

**Purpose:** Initiate an asynchronous TCP connection.

**Parameters:**
- `pAddr`: IP address string (e.g., "192.168.1.1")
- `iPort`: Port number
- `uiMsg`: Windows message ID for event notifications

**Returns:** `TRUE` if connection initiated, `FALSE` on error.

**Event Notification:**
The function registers for four event types:
- `FD_CONNECT`: Connection completed (success or failure)
- `FD_READ`: Data available to read
- `FD_WRITE`: Socket ready for writing
- `FD_CLOSE`: Connection closed by remote

**Socket Options:**
- `SO_RCVBUF`: 40,960 bytes receive buffer
- `SO_SNDBUF`: 40,960 bytes send buffer

### bBlockConnect (Synchronous)

```cpp
BOOL XSocket::bBlockConnect(char * pAddr, int iPort, unsigned int uiMsg)
{
    SOCKADDR_IN   saTemp;
    int           iRet;
    DWORD         dwOpt;
    struct hostent * hp;

    if (m_cType == DEF_XSOCK_LISTENSOCK) return FALSE;
    if (m_Sock != INVALID_SOCKET) closesocket(m_Sock);

    m_Sock = socket(AF_INET, SOCK_STREAM, 0);
    if (m_Sock == INVALID_SOCKET)
        return FALSE;

    // Use gethostbyname for DNS resolution
    memset(&saTemp, 0, sizeof(saTemp));
    hp = gethostbyname(pAddr);
    if (hp) {
        memcpy(&(saTemp.sin_addr), hp->h_addr, hp->h_length);
        saTemp.sin_family = hp->h_addrtype;
        saTemp.sin_port = htons(iPort);
    }
    else {
        return FALSE;
    }

    // Blocking connect (waits for completion)
    iRet = connect(m_Sock, (struct sockaddr *) &saTemp, sizeof(saTemp));
    if (iRet == SOCKET_ERROR) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            m_WSAErr = WSAGetLastError();
            return FALSE;
        }
    }

    // Set socket buffer sizes
    dwOpt = 8192 * 5;
    setsockopt(m_Sock, SOL_SOCKET, SO_RCVBUF, (const char FAR *)&dwOpt, sizeof(dwOpt));
    setsockopt(m_Sock, SOL_SOCKET, SO_SNDBUF, (const char FAR *)&dwOpt, sizeof(dwOpt));

    strcpy(m_pAddr, pAddr);
    m_iPortNum = iPort;
    m_uiMsg = uiMsg;
    m_cType = DEF_XSOCK_NORMALSOCK;

    return TRUE;
}
```

**Differences from bConnect:**
1. Uses `gethostbyname()` for DNS resolution (supports hostnames, not just IP addresses)
2. Does not call `WSAAsyncSelect()` - remains in blocking mode
3. The `connect()` call blocks until completion

**Note:** This method appears to be unused in the client code. The client exclusively uses the asynchronous `bConnect()` method.

### bListen (Server Mode)

```cpp
BOOL XSocket::bListen(char * pAddr, int iPort, unsigned int uiMsg)
{
    SOCKADDR_IN saTemp;

    if (m_cType != NULL) return FALSE;  // Already initialized
    if (m_Sock != INVALID_SOCKET) closesocket(m_Sock);

    // Create TCP socket
    m_Sock = socket(AF_INET, SOCK_STREAM, 0);
    if (m_Sock == INVALID_SOCKET)
        return FALSE;

    // Bind to address
    memset(&saTemp, 0, sizeof(saTemp));
    saTemp.sin_family = AF_INET;
    saTemp.sin_addr.s_addr = inet_addr(pAddr);
    saTemp.sin_port = htons(iPort);
    if (bind(m_Sock, (PSOCKADDR)&saTemp, sizeof(saTemp)) == SOCKET_ERROR) {
        closesocket(m_Sock);
        return FALSE;
    }

    // Start listening (backlog of 5)
    if (listen(m_Sock, 5) == SOCKET_ERROR) {
        closesocket(m_Sock);
        return FALSE;
    }

    // Register for accept events
    WSAAsyncSelect(m_Sock, m_hWnd, uiMsg, FD_ACCEPT);

    m_uiMsg = uiMsg;
    m_cType = DEF_XSOCK_LISTENSOCK;

    return TRUE;
}
```

**Purpose:** Create a server socket that listens for incoming connections.

**Note:** This method is designed for server-side use and is not used by the game client.

### bAccept (Server Mode)

```cpp
BOOL XSocket::bAccept(class XSocket * pXSock, unsigned int uiMsg)
{
    SOCKET       AcceptedSock;
    sockaddr     Addr;
    register int iLength;
    DWORD        dwOpt;

    if (m_cType != DEF_XSOCK_LISTENSOCK) return FALSE;
    if (pXSock == NULL) return FALSE;

    iLength = sizeof(Addr);
    AcceptedSock = accept(m_Sock, (struct sockaddr FAR *)&Addr, (int FAR *)&iLength);
    if (AcceptedSock == INVALID_SOCKET)
        return FALSE;

    // Configure the accepted socket
    pXSock->m_Sock = AcceptedSock;
    WSAAsyncSelect(pXSock->m_Sock, m_hWnd, uiMsg, FD_READ | FD_WRITE | FD_CLOSE);

    pXSock->m_uiMsg = uiMsg;
    pXSock->m_cType = DEF_XSOCK_NORMALSOCK;

    // Set socket buffer sizes
    dwOpt = 8192 * 5;
    setsockopt(pXSock->m_Sock, SOL_SOCKET, SO_RCVBUF, (const char FAR *)&dwOpt, sizeof(dwOpt));
    setsockopt(pXSock->m_Sock, SOL_SOCKET, SO_SNDBUF, (const char FAR *)&dwOpt, sizeof(dwOpt));

    return TRUE;
}
```

**Purpose:** Accept an incoming connection on a listening socket.

**Note:** Server-side only, not used by the game client.

---

## Event Handling

### iOnSocketEvent

```cpp
int XSocket::iOnSocketEvent(WPARAM wParam, LPARAM lParam)
{
    int WSAEvent;

    // Validate socket type
    if (m_cType != DEF_XSOCK_NORMALSOCK) return DEF_XSOCKEVENT_SOCKETMISMATCH;
    if (m_cType == NULL) return DEF_XSOCKEVENT_NOTINITIALIZED;

    // Validate socket handle
    if ((SOCKET)wParam != m_Sock) return DEF_XSOCKEVENT_SOCKETMISMATCH;

    // Extract event type
    WSAEvent = WSAGETSELECTEVENT(lParam);

    switch (WSAEvent) {
    case FD_CONNECT:
        if (WSAGETSELECTERROR(lParam) != 0) {
            // Connection failed, retry
            if (bConnect(m_pAddr, m_iPortNum, m_uiMsg) == FALSE)
                return DEF_XSOCKEVENT_SOCKETERROR;
            return DEF_XSOCKEVENT_RETRYINGCONNECTION;
        }
        else {
            m_bIsAvailable = TRUE;
            return DEF_XSOCKEVENT_CONNECTIONESTABLISH;
        }
        break;

    case FD_READ:
        if (WSAGETSELECTERROR(lParam) != 0) {
            m_WSAErr = WSAGETSELECTERROR(lParam);
            return DEF_XSOCKEVENT_SOCKETERROR;
        }
        else return _iOnRead();
        break;

    case FD_WRITE:
        m_bIsWriteEnabled = TRUE;
        return _iSendUnsentData();
        break;

    case FD_CLOSE:
        m_cType = DEF_XSOCK_SHUTDOWNEDSOCK;
        return DEF_XSOCKEVENT_SOCKETCLOSED;
        break;
    }

    return DEF_XSOCKEVENT_UNKNOWN;
}
```

**Purpose:** Main entry point for processing Windows socket events.

**Parameters:**
- `wParam`: Socket handle that generated the event
- `lParam`: Event details (use `WSAGETSELECTEVENT` and `WSAGETSELECTERROR` macros)

**Event Flow:**

```
Windows Message (WM_USER_GAMESOCKETEVENT or WM_USER_LOGSOCKETEVENT)
                              │
                              ▼
              CGame::OnGameSocketEvent() / OnLogSocketEvent()
                              │
                              ▼
                  XSocket::iOnSocketEvent()
                              │
          ┌───────────────────┼───────────────────┐
          ▼                   ▼                   ▼
     FD_CONNECT          FD_READ             FD_WRITE
          │                   │                   │
          ▼                   ▼                   ▼
  Set m_bIsAvailable    _iOnRead()      _iSendUnsentData()
          │                   │                   │
          ▼                   ▼                   ▼
  CONNECTIONESTABLISH   READCOMPLETE    UNSENTDATASENDCOMPLETE
```

---

## Reading Messages

### _iOnRead (Internal)

```cpp
int XSocket::_iOnRead()
{
    int iRet, WSAErr;
    WORD * wp;

    if (m_cStatus == DEF_XSOCKSTATUS_READINGHEADER) {
        // Reading 3-byte header
        iRet = recv(m_Sock, (char *)(m_pRcvBuffer + m_dwTotalReadSize), m_dwReadSize, 0);

        if (iRet == SOCKET_ERROR) {
            WSAErr = WSAGetLastError();
            if (WSAErr != WSAEWOULDBLOCK) {
                m_WSAErr = WSAErr;
                return DEF_XSOCKEVENT_SOCKETERROR;
            }
            else return DEF_XSOCKEVENT_BLOCK;
        }
        else if (iRet == 0) {
            m_cType = DEF_XSOCK_SHUTDOWNEDSOCK;
            return DEF_XSOCKEVENT_SOCKETCLOSED;
        }

        m_dwReadSize -= iRet;
        m_dwTotalReadSize += iRet;

        if (m_dwReadSize == 0) {
            // Header complete, parse size
            m_cStatus = DEF_XSOCKSTATUS_READINGBODY;
            wp = (WORD *)(m_pRcvBuffer + 1);
            m_dwReadSize = (int)(*wp - 3);  // Subtract header size

            if (m_dwReadSize == 0) {
                // No body, message complete
                m_cStatus = DEF_XSOCKSTATUS_READINGHEADER;
                m_dwReadSize = 3;
                m_dwTotalReadSize = 0;
                return DEF_XSOCKEVENT_READCOMPLETE;
            }
            else if (m_dwReadSize > m_dwBufferSize) {
                // Message too large
                m_cStatus = DEF_XSOCKSTATUS_READINGHEADER;
                m_dwReadSize = 3;
                m_dwTotalReadSize = 0;
                return DEF_XSOCKEVENT_MSGSIZETOOLARGE;
            }
        }
        return DEF_XSOCKEVENT_ONREAD;
    }
    else if (m_cStatus == DEF_XSOCKSTATUS_READINGBODY) {
        // Reading message body
        iRet = recv(m_Sock, (char *)(m_pRcvBuffer + m_dwTotalReadSize), m_dwReadSize, 0);

        if (iRet == SOCKET_ERROR) {
            WSAErr = WSAGetLastError();
            if (WSAErr != WSAEWOULDBLOCK) {
                m_WSAErr = WSAErr;
                return DEF_XSOCKEVENT_SOCKETERROR;
            }
            else return DEF_XSOCKEVENT_BLOCK;
        }
        else if (iRet == 0) {
            m_cType = DEF_XSOCK_SHUTDOWNEDSOCK;
            return DEF_XSOCKEVENT_SOCKETCLOSED;
        }

        m_dwReadSize -= iRet;
        m_dwTotalReadSize += iRet;

        if (m_dwReadSize == 0) {
            // Body complete
            m_cStatus = DEF_XSOCKSTATUS_READINGHEADER;
            m_dwReadSize = 3;
            m_dwTotalReadSize = 0;
        }
        else return DEF_XSOCKEVENT_ONREAD;
    }

    return DEF_XSOCKEVENT_READCOMPLETE;
}
```

**State Machine Diagram:**

```
                    ┌──────────────────────┐
                    │   READINGHEADER      │
                    │   (m_dwReadSize = 3) │
                    └──────────┬───────────┘
                               │
                    ┌──────────▼───────────┐
                    │   recv() header      │
                    │   bytes              │
                    └──────────┬───────────┘
                               │
              ┌────────────────┼────────────────┐
              ▼                ▼                ▼
        [Partial]        [Complete]       [Error/Close]
              │                │                │
              ▼                ▼                ▼
        ONREAD           Parse size       SOCKETERROR/
              │                │          SOCKETCLOSED
              │                ▼
              │      ┌─────────────────────┐
              │      │   Size == 0?        │
              │      └─────────┬───────────┘
              │           Yes  │  No
              │                ▼
              │      ┌─────────────────────┐
              │      │   READINGBODY       │
              │      │   (Size - 3 bytes)  │
              │      └─────────┬───────────┘
              │                │
              │      ┌─────────▼───────────┐
              │      │   recv() body       │
              │      │   bytes             │
              │      └─────────┬───────────┘
              │                │
              │   ┌────────────┼────────────┐
              │   ▼            ▼            ▼
              │ [Partial]  [Complete]  [Error/Close]
              │   │            │
              │   ▼            ▼
              │ ONREAD    READCOMPLETE
              │   │            │
              └───┴────────────┴──────────────────┐
                                                  │
                               ┌──────────────────▼───────────────┐
                               │  Reset state for next message    │
                               │  m_cStatus = READINGHEADER       │
                               │  m_dwReadSize = 3                │
                               │  m_dwTotalReadSize = 0           │
                               └──────────────────────────────────┘
```

### pGetRcvDataPointer

```cpp
char * XSocket::pGetRcvDataPointer(DWORD * pMsgSize, char * pKey)
{
    WORD * wp;
    DWORD  dwSize;
    register int i;
    char cKey;

    // Extract key from header
    cKey = m_pRcvBuffer[0];
    if (pKey != NULL) *pKey = cKey;

    // Extract and return message size (excluding header)
    wp = (WORD *)(m_pRcvBuffer + 1);
    *pMsgSize = (*wp) - 3;
    dwSize = (*wp) - 3;

    // Decrypt if key is non-zero
    if (cKey != NULL) {
        for (i = 0; i < (int)(dwSize); i++) {
            m_pRcvBuffer[3+i] = (char)(m_pRcvBuffer[3+i] ^ (cKey ^ (dwSize - i)));
            m_pRcvBuffer[3+i] -= (i ^ cKey);
        }
    }

    // Return pointer to payload
    return (m_pRcvBuffer + 3);
}
```

**Purpose:** Retrieve a pointer to the received message data after decryption.

**Parameters:**
- `pMsgSize`: Output - receives the payload size in bytes
- `pKey`: Output (optional) - receives the encryption key

**Returns:** Pointer to the decrypted payload data.

**Usage in CGame:**
```cpp
pData = m_pGSock->pGetRcvDataPointer(&dwMsgSize);
GameRecvMsgHandler(dwMsgSize, pData);
```

---

## Sending Messages

### iSendMsg

```cpp
int XSocket::iSendMsg(char * cData, DWORD dwSize, char cKey)
{
    WORD * wp;
    int    i, iRet;

    // Validate message size
    if (dwSize > m_dwBufferSize) return DEF_XSOCKEVENT_MSGSIZETOOLARGE;

    // Validate socket state
    if (m_cType != DEF_XSOCK_NORMALSOCK) return DEF_XSOCKEVENT_SOCKETMISMATCH;
    if (m_cType == NULL) return DEF_XSOCKEVENT_NOTINITIALIZED;

    // Build header: Key byte
    m_pSndBuffer[0] = cKey;

    // Build header: Size (including header)
    wp = (WORD *)(m_pSndBuffer + 1);
    *wp = (WORD)(dwSize + 3);

    // Copy payload
    memcpy((char *)(m_pSndBuffer + 3), cData, dwSize);

    // Encrypt if key is non-zero
    if (cKey != NULL) {
        for (i = 0; i < (int)(dwSize); i++) {
            m_pSndBuffer[3+i] += (i ^ cKey);
            m_pSndBuffer[3+i] = (char)(m_pSndBuffer[3+i] ^ (cKey ^ (dwSize - i)));
        }
    }

    // Send (or queue if not ready)
    if (m_bIsWriteEnabled == FALSE) {
        iRet = _iRegisterUnsentData(m_pSndBuffer, dwSize + 3);
    }
    else {
        iRet = _iSend(m_pSndBuffer, dwSize + 3, TRUE);
    }

    if (iRet < 0) return iRet;
    else return (iRet - 3);  // Return payload bytes sent
}
```

**Purpose:** Send a message to the connected peer with optional encryption.

**Parameters:**
- `cData`: Pointer to message data (payload only, no header)
- `dwSize`: Size of the payload in bytes
- `cKey`: Encryption key (0 = no encryption, 1-255 = XOR key)

**Returns:**
- Positive value: Number of payload bytes sent
- Negative value: Error code (see event constants)

**Key Generation in CGame:**
```cpp
cKey = (char)(rand() % 255) + 1;  // Random key 1-255
```

**Usage Examples from CGame:**
```cpp
// Encrypted message
iRet = m_pGSock->iSendMsg(cMsg, 26, cKey);

// Unencrypted message (cKey = 0 or omitted)
iRet = m_pGSock->iSendMsg(cMsg, 11);
```

### iSendMsgBlockingMode

```cpp
int XSocket::iSendMsgBlockingMode(char *buf, int nbytes)
{
    int nleft = 0, nwriten = 0;

    nleft = nbytes;
    while (nleft > 0) {
        nwriten = send(m_Sock, buf, nleft, 0);
        if (nwriten < 0) {
            return nwriten;
        }
        if (nwriten == 0) break;
        nleft -= nwriten;
        buf += nwriten;
    }
    return (nbytes - nleft);
}
```

**Purpose:** Send data synchronously, blocking until all data is sent.

**Note:** This method bypasses the message framing protocol and sends raw data. It appears to be unused in the client.

---

## Unsent Data Queue

The XSocket class implements a circular buffer queue to handle network congestion (when sends would block).

### Queue Structure

```
m_pUnsentDataList[0]  ─┬─ NULL or data pointer
m_pUnsentDataList[1]  ─┤
m_pUnsentDataList[2]  ─┤
        ...           ─┤
m_pUnsentDataList[299]─┘

m_iUnsentDataSize[0]  ─┬─ Size of data at same index
m_iUnsentDataSize[1]  ─┤
        ...           ─┘

m_sHead → Next entry to send (dequeue)
m_sTail → Next slot to fill (enqueue)
```

**Queue States:**

```
Empty Queue:
┌────┬────┬────┬────┬────┐
│NULL│NULL│NULL│NULL│NULL│
└────┴────┴────┴────┴────┘
  ▲
  Head = Tail = 0

Partially Full:
┌────┬────┬────┬────┬────┐
│NULL│data│data│NULL│NULL│
└────┴────┴────┴────┴────┘
       ▲         ▲
       Head      Tail

Full Queue:
┌────┬────┬────┬────┬────┐
│data│data│data│data│data│
└────┴────┴────┴────┴────┘
  ▲                   ▲
  Head                Tail (wraps to Head)
```

### _iRegisterUnsentData

```cpp
int XSocket::_iRegisterUnsentData(char * cData, int iSize)
{
    // Check if queue is full
    if (m_pUnsentDataList[m_sTail] != NULL) return 0;

    // Allocate memory for data copy
    m_pUnsentDataList[m_sTail] = new char[iSize];
    if (m_pUnsentDataList[m_sTail] == NULL) return -1;

    // Copy data to queue
    memcpy(m_pUnsentDataList[m_sTail], cData, iSize);
    m_iUnsentDataSize[m_sTail] = iSize;

    // Advance tail pointer (circular)
    m_sTail++;
    if (m_sTail >= m_iBlockLimit) m_sTail = 0;

    return 1;
}
```

**Returns:**
- `1`: Data successfully queued
- `0`: Queue is full
- `-1`: Memory allocation failed

### _iSendUnsentData

```cpp
int XSocket::_iSendUnsentData()
{
    int iRet;
    char * pTemp;

    // Process queue until empty or blocked
    while (m_pUnsentDataList[m_sHead] != NULL) {

        iRet = _iSend_ForInternalUse(m_pUnsentDataList[m_sHead],
                                      m_iUnsentDataSize[m_sHead]);

        if (iRet == m_iUnsentDataSize[m_sHead]) {
            // Successfully sent all data
            delete[] m_pUnsentDataList[m_sHead];
            m_pUnsentDataList[m_sHead] = NULL;
            m_iUnsentDataSize[m_sHead] = 0;

            // Advance head pointer (circular)
            m_sHead++;
            if (m_sHead >= m_iBlockLimit) m_sHead = 0;
        }
        else {
            // Handle errors
            if (iRet < 0)
                return iRet;

            // Partial send - keep remaining data
            pTemp = new char[m_iUnsentDataSize[m_sHead] - iRet];
            memcpy(pTemp, m_pUnsentDataList[m_sHead] + iRet,
                   m_iUnsentDataSize[m_sHead] - iRet);

            delete[] m_pUnsentDataList[m_sHead];
            m_pUnsentDataList[m_sHead] = pTemp;
            m_iUnsentDataSize[m_sHead] -= iRet;

            return DEF_XSOCKEVENT_UNSENTDATASENDBLOCK;
        }
    }

    return DEF_XSOCKEVENT_UNSENTDATASENDCOMPLETE;
}
```

**Purpose:** Flush the unsent data queue when the socket becomes writable.

**Called When:** `FD_WRITE` event is received.

### _iSend (Internal)

```cpp
int XSocket::_iSend(char * cData, int iSize, BOOL bSaveFlag)
{
    int iOutLen, iRet, WSAErr;

    // If queue not empty, must queue to preserve order
    if (m_pUnsentDataList[m_sHead] != NULL) {
        if (bSaveFlag == TRUE) {
            iRet = _iRegisterUnsentData(cData, iSize);
            switch (iRet) {
            case -1: return DEF_XSOCKEVENT_CRITICALERROR;
            case 0:  return DEF_XSOCKEVENT_QUENEFULL;
            case 1:  break;
            }
            return DEF_XSOCKEVENT_BLOCK;
        }
        else return 0;
    }

    // Try to send all data
    iOutLen = 0;
    while (iOutLen < iSize) {
        iRet = send(m_Sock, (cData + iOutLen), iSize - iOutLen, 0);

        if (iRet == SOCKET_ERROR) {
            WSAErr = WSAGetLastError();
            if (WSAErr != WSAEWOULDBLOCK) {
                m_WSAErr = WSAErr;
                return DEF_XSOCKEVENT_SOCKETERROR;
            }
            else {
                // Would block - queue remaining data
                if (bSaveFlag == TRUE) {
                    iRet = _iRegisterUnsentData((cData + iOutLen), (iSize - iOutLen));
                    switch (iRet) {
                    case -1: return DEF_XSOCKEVENT_CRITICALERROR;
                    case 0:  return DEF_XSOCKEVENT_QUENEFULL;
                    case 1:  break;
                    }
                }
                return DEF_XSOCKEVENT_BLOCK;
            }
        }
        else iOutLen += iRet;
    }

    return iOutLen;
}
```

**Parameters:**
- `cData`: Data to send
- `iSize`: Size of data
- `bSaveFlag`: If TRUE, queue data on WOULDBLOCK; if FALSE, discard

### _iSend_ForInternalUse

```cpp
int XSocket::_iSend_ForInternalUse(char * cData, int iSize)
{
    int iOutLen, iRet, WSAErr;

    iOutLen = 0;
    while (iOutLen < iSize) {
        iRet = send(m_Sock, (cData + iOutLen), iSize - iOutLen, 0);

        if (iRet == SOCKET_ERROR) {
            WSAErr = WSAGetLastError();
            if (WSAErr != WSAEWOULDBLOCK) {
                m_WSAErr = WSAErr;
                return DEF_XSOCKEVENT_SOCKETERROR;
            }
            else {
                // Return partial send count (for queue management)
                return iOutLen;
            }
        }
        else iOutLen += iRet;
    }

    return iOutLen;
}
```

**Purpose:** Internal send for queue flushing. Returns bytes sent rather than queueing on WOULDBLOCK.

---

## Connection Closure

### _CloseConn

```cpp
void XSocket::_CloseConn()
{
    char cTmp[100];
    BOOL bFlag = TRUE;
    int  iRet;

    if (m_Sock == INVALID_SOCKET) return;

    // Graceful shutdown - stop sending
    shutdown(m_Sock, 0x01);  // SD_SEND

    // Drain any remaining incoming data
    while (bFlag == TRUE) {
        iRet = recv(m_Sock, cTmp, sizeof(cTmp), 0);
        if (iRet == SOCKET_ERROR) bFlag = FALSE;
        if (iRet == 0) bFlag = FALSE;
    }

    // Close the socket
    closesocket(m_Sock);

    m_cType = DEF_XSOCK_SHUTDOWNEDSOCK;
}
```

**Purpose:** Gracefully close a TCP connection.

**Graceful Close Sequence:**
1. `shutdown(SD_SEND)` - Signal we're done sending
2. Drain receive buffer - Read until EOF or error
3. `closesocket()` - Release socket resources

---

## Utility Methods

### iGetPeerAddress

```cpp
int XSocket::iGetPeerAddress(char * pAddrString)
{
    SOCKADDR_IN sockaddr;
    int iRet, iLen;

    iLen = sizeof(sockaddr);
    iRet = getpeername(m_Sock, (struct sockaddr *)&sockaddr, &iLen);
    strcpy(pAddrString, (const char *)inet_ntoa(sockaddr.sin_addr));

    return iRet;
}
```

**Purpose:** Get the IP address of the connected peer.

**Parameters:**
- `pAddrString`: Buffer to receive the IP address string (at least 16 bytes)

**Returns:** 0 on success, SOCKET_ERROR on failure.

---

## Integration with CGame

### Socket Member Variables

In `Game.h`:
```cpp
class CGame {
    // ...
    class XSocket * m_pGSock;   // Game server socket
    class XSocket * m_pLSock;   // Login server socket
    // ...
};
```

### Windows Message IDs

```cpp
#define WM_USER_GAMESOCKETEVENT    WM_USER + 2000  // Game socket events
#define WM_USER_LOGSOCKETEVENT     WM_USER + 2001  // Login socket events
```

### Connection Flow

```
1. Login Phase
   ┌─────────────────────────────────────────────────────────┐
   │ m_pLSock = new XSocket(m_hWnd, DEF_SOCKETBLOCKLIMIT)    │
   │ m_pLSock->bConnect(addr, port, WM_USER_LOGSOCKETEVENT)  │
   │ m_pLSock->bInitBufferSize(30000)                        │
   └─────────────────────────────────────────────────────────┘
                              │
                              ▼
   ┌─────────────────────────────────────────────────────────┐
   │ WM_USER_LOGSOCKETEVENT received                         │
   │ CGame::OnLogSocketEvent() called                        │
   │ → iOnSocketEvent() → CONNECTIONESTABLISH                │
   │ → ConnectionEstablishHandler(DEF_SERVERTYPE_LOG)        │
   └─────────────────────────────────────────────────────────┘
                              │
                              ▼
   ┌─────────────────────────────────────────────────────────┐
   │ Login messages exchanged                                │
   │ Character selection                                     │
   │ Receive game server address                             │
   └─────────────────────────────────────────────────────────┘
                              │
                              ▼
2. Game Server Connection
   ┌─────────────────────────────────────────────────────────┐
   │ delete m_pLSock (close login connection)                │
   │ m_pGSock = new XSocket(m_hWnd, DEF_SOCKETBLOCKLIMIT)    │
   │ m_pGSock->bConnect(addr, port, WM_USER_GAMESOCKETEVENT) │
   │ m_pGSock->bInitBufferSize(30000)                        │
   └─────────────────────────────────────────────────────────┘
                              │
                              ▼
   ┌─────────────────────────────────────────────────────────┐
   │ WM_USER_GAMESOCKETEVENT received                        │
   │ CGame::OnGameSocketEvent() called                       │
   │ → Game communication begins                             │
   └─────────────────────────────────────────────────────────┘
```

### Event Handler Implementation

```cpp
void CGame::OnGameSocketEvent(WPARAM wParam, LPARAM lParam)
{
    int iRet;
    char * pData;
    DWORD dwMsgSize;

    if (m_pGSock == NULL) return;

    iRet = m_pGSock->iOnSocketEvent(wParam, lParam);

    switch (iRet) {
    case DEF_XSOCKEVENT_CONNECTIONESTABLISH:
        ConnectionEstablishHandler(DEF_SERVERTYPE_GAME);
        break;

    case DEF_XSOCKEVENT_READCOMPLETE:
        pData = m_pGSock->pGetRcvDataPointer(&dwMsgSize);
        GameRecvMsgHandler(dwMsgSize, pData);
        m_dwTime = G_dwGlobalTime;  // Update last message time
        break;

    case DEF_XSOCKEVENT_SOCKETCLOSED:
        ChangeGameMode(DEF_GAMEMODE_ONCONNECTIONLOST);
        delete m_pGSock;
        m_pGSock = NULL;
        break;

    case DEF_XSOCKEVENT_SOCKETERROR:
        ChangeGameMode(DEF_GAMEMODE_ONCONNECTIONLOST);
        delete m_pGSock;
        m_pGSock = NULL;
        break;

    case DEF_XSOCKEVENT_CRITICALERROR:
        delete m_pGSock;
        m_pGSock = NULL;
        if (G_pCalcSocket != NULL) {
            delete G_pCalcSocket;
            G_pCalcSocket = NULL;
        }
        break;
    }
}
```

### Message Sending Pattern

```cpp
BOOL CGame::bSendCommand(DWORD dwMsgID, ...)
{
    char  cMsg[512], cKey, cTxt[128];
    WORD  * wp;
    DWORD * dwp;
    char  * cp;
    int   iRet;

    if ((m_pGSock == NULL) && (m_pLSock == NULL)) return FALSE;

    ZeroMemory(cMsg, sizeof(cMsg));
    cKey = (char)(rand() % 255) + 1;  // Random encryption key

    switch (dwMsgID) {
    case MSGID_REQUEST_RESTART:
        // Build message header
        dwp = (DWORD *)(cMsg + DEF_INDEX4_MSGID);  // Offset 0
        *dwp = dwMsgID;
        wp = (WORD *)(cMsg + DEF_INDEX2_MSGTYPE);  // Offset 4
        *wp = NULL;

        // Send via game socket
        iRet = m_pGSock->iSendMsg(cMsg, 6, cKey);
        break;

    case MSGID_REQUEST_LOGIN:
        // Build message with payload
        dwp = (DWORD *)(cMsg + DEF_INDEX4_MSGID);
        *dwp = dwMsgID;
        wp = (WORD *)(cMsg + DEF_INDEX2_MSGTYPE);
        *wp = NULL;

        cp = (char *)(cMsg + DEF_INDEX2_MSGTYPE + 2);
        memcpy(cp, m_cAccountName, 10);
        cp += 10;
        memcpy(cp, m_cAccountPassword, 10);
        cp += 10;
        // ... more fields ...

        // Send via login socket
        iRet = m_pLSock->iSendMsg(cMsg, 46, cKey);
        break;

    // ... many more message types ...
    }

    return TRUE;
}
```

---

## Global Socket Instance

In addition to the game/login sockets, there's a global "calculator" socket:

```cpp
// Wmain.cpp
class XSocket * G_pCalcSocket = NULL;

// Initialization
G_pCalcSocket = new class XSocket(G_hWnd, 100);
```

This appears to be used for auxiliary calculations or anti-cheat verification, with a smaller queue limit (100 vs 300).

---

## Error Handling

### WSA Error Codes

Common WinSock errors stored in `m_WSAErr`:

| Error | Value | Description |
|-------|-------|-------------|
| `WSAEWOULDBLOCK` | 10035 | Operation would block (expected in async mode) |
| `WSAENOTCONN` | 10057 | Socket not connected |
| `WSAECONNRESET` | 10054 | Connection reset by peer |
| `WSAECONNREFUSED` | 10061 | Connection refused |
| `WSAETIMEDOUT` | 10060 | Connection timed out |
| `WSAENETUNREACH` | 10051 | Network unreachable |
| `WSAEHOSTUNREACH` | 10065 | Host unreachable |

### Reconnection Logic

When `FD_CONNECT` reports an error, XSocket automatically retries:

```cpp
case FD_CONNECT:
    if (WSAGETSELECTERROR(lParam) != 0) {
        // Retry connection with stored parameters
        if (bConnect(m_pAddr, m_iPortNum, m_uiMsg) == FALSE)
            return DEF_XSOCKEVENT_SOCKETERROR;
        return DEF_XSOCKEVENT_RETRYINGCONNECTION;
    }
```

This creates an infinite retry loop until:
- Connection succeeds
- `bConnect()` itself fails (returns FALSE)

---

## Thread Safety

**XSocket is NOT thread-safe.**

All socket operations must occur on the same thread that owns the window handle (`m_hWnd`). This is enforced by WinSock's `WSAAsyncSelect` which delivers events to a specific window's message queue.

**Implications:**
- All XSocket instances in the client share the main thread
- No mutex/critical section protection
- The Windows message loop serializes all socket events
- Long-running operations in message handlers will block all socket I/O

---

## Performance Characteristics

### Buffer Sizes

| Buffer | Size | Purpose |
|--------|------|---------|
| `m_pRcvBuffer` | 30,008 bytes | Receive buffer |
| `m_pSndBuffer` | 30,008 bytes | Send buffer |
| `SO_RCVBUF` | 40,960 bytes | OS receive buffer |
| `SO_SNDBUF` | 40,960 bytes | OS send buffer |

### Queue Capacity

- Maximum queued messages: 300 (configurable)
- Each queue entry: variable size (dynamically allocated)
- Total potential queue memory: Unbounded (limited by available heap)

### Typical Message Sizes

| Message Type | Typical Size | Max Size |
|--------------|--------------|----------|
| Keepalive | 6 bytes | 6 bytes |
| Motion command | 10-20 bytes | ~50 bytes |
| Chat message | 50-300 bytes | ~500 bytes |
| Item list | 500-2000 bytes | ~5000 bytes |
| Full object data | 5000-20000 bytes | 30000 bytes |

---

## Known Issues and Limitations

### 1. Encryption Weakness
The XOR-based encryption is trivially breakable with known-plaintext attacks. The message structure is well-known, making the first few bytes predictable.

### 2. No Message Integrity
There's no checksum or MAC to verify message integrity. Corrupted packets will be processed as valid.

### 3. Memory Leaks on Error
If the application crashes during message processing, queued unsent data is leaked (though this is moot since the process terminates).

### 4. No Timeout Handling
XSocket has no built-in timeout for connections or operations. The game handles this at a higher level using `m_dwTime` comparisons.

### 5. Single-Threaded Design
All I/O is tied to the UI thread, which can cause lag during heavy network activity.

### 6. Blocking DNS Resolution
`bBlockConnect()` uses `gethostbyname()` which blocks. However, this method appears unused.

### 7. Fixed Buffer Size
Once `bInitBufferSize()` is called, the buffer size cannot be changed without potential data loss.

---

## Related Files

| File | Purpose |
|------|---------|
| `NetMessages.h` | Protocol message ID constants |
| `Game.h` | Socket member declarations, `DEF_SOCKETBLOCKLIMIT` |
| `Game.cpp` | Socket usage, event handlers, `bSendCommand()` |
| `Wmain.cpp` | Window message dispatch, `G_pCalcSocket` |

---

## Summary

The XSocket class provides a foundational networking layer for the Helbreath client with:

- **Asynchronous I/O** via Windows message-based events
- **Message framing** with a 3-byte header (key + size)
- **Simple encryption** using position-dependent XOR
- **Congestion handling** via a circular queue for unsent data
- **Automatic reconnection** on connection failure

While functional for its era (2002), the design has several limitations by modern standards, including weak encryption, no message integrity verification, and tight coupling to the Windows message loop.
