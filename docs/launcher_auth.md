# Launcher Authentication Handoff

## Overview

The game client needs to receive authentication credentials from an external launcher without exposing them through inspectable channels like command-line arguments or environment variables.

## Approach: stdin Pipe

The launcher spawns the client process with stdin redirected to a pipe, writes a JSON auth payload, then closes the pipe. The client reads this on startup before entering the main loop.

### Why stdin

- **Cross-platform**: `std::cin` works identically on Windows and Linux with zero platform-specific code for the read path
- **No disk artifacts**: nothing written to the filesystem
- **No process-list exposure**: unlike command-line args, stdin data doesn't appear in `ps`, `/proc/<pid>/cmdline`, or Task Manager
- **No lingering state**: unlike temp files, there's nothing to clean up if the client crashes
- **Language-agnostic**: every launcher framework (Python, C#, Electron, Qt, Rust) supports writing to a child process's stdin natively

### Alternatives considered

| Approach | Rejected because |
|----------|-----------------|
| Command-line args | Visible in process list on all platforms |
| Environment variables | Readable via `/proc/<pid>/environ` (Linux) and Process Explorer (Windows) |
| Temp file | Platform-specific permission models (chmod vs ACLs), lingering files on crash |
| Named pipes | Completely different APIs on Windows vs Linux - effectively two systems |

## Client Behavior

### Startup detection

On startup, the client checks whether stdin is a pipe (launcher mode) or a terminal (standalone mode):

```cpp
#ifdef _WIN32
#include <io.h>
bool has_piped_input = !_isatty(_fileno(stdin));
#else
#include <unistd.h>
bool has_piped_input = !isatty(STDIN_FILENO);
#endif
```

### Auth payload

The launcher writes a single line of JSON to stdin:

```json
{
    "token": "session-token-from-auth-server",
    "server": "play.helbreath.com",
    "port": 2848,
    "account": "player_name"
}
```

The exact fields will be defined when the launcher is built. The client reads one line, parses it as JSON, and uses it to skip the login screen and connect directly.

### Fallback

When no piped input is detected (standalone launch, development, debugging), the client shows the normal login screen. This keeps the client fully functional without a launcher.

## Launcher Requirements

Any launcher implementation must:

1. Spawn the client with stdin connected to a pipe (not inherited from the launcher's own stdin)
2. Write the JSON payload as a single line followed by a newline
3. Close the pipe after writing (so the client's read doesn't block)
4. Not depend on the client's stdout/stderr for the handoff (those remain available for logging)

## Status

**Not yet implemented.** This document captures the design intent for when a launcher is built.
