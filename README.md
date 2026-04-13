# discord-mud

A social world server. Think Discord, but you can walk around and build the world together.

Connect, build rooms, explore, talk. The server speaks raw TCP — telnet works for testing, web client coming soon.

## What is this

A MUD-style server written from scratch in C. Not a game — a **place**. Users connect via telnet, land in a shared world, and build it collaboratively using OLC (Online Creation) commands. Chat happens in rooms you can walk between and shape.

## Features

- **Telnet server** — raw TCP, select()-based multiplexing, handles multiple concurrent connections
- **Room system** — linked room graph with 10 compass directions (8 cardinals + up/down)
- **OLC (Online Creation)** — create and edit rooms from inside the world
  - `roomedit create` — create a new room
  - `roomedit name <text>` — set room name
  - `roomedit description <text>` — set room description
  - `roomedit exit <direction> <room_id>` — link an exit
  - `roomedit rmexit <direction>` — remove an exit
- **Movement** — walk between rooms by typing directions (`north`, `n`, `down`, `d`, etc.)
- **Chat** — `say <message>` broadcasts to everyone in your room
- **Player accounts** — name/password login with SHA256 hashed passwords
- **Persistence** — world and player data save to flat files, auto-saves on shutdown
- **ANSI color** — colored room names, descriptions, and exits
- **Clean shutdown** — ctrl-c saves the world and closes all connections gracefully

## Build

### macOS

```
make
```

Uses CommonCrypto (ships with macOS, no extra dependencies).

### Linux

```
sudo apt install libssl-dev   # Debian/Ubuntu
make
```

Uses OpenSSL for SHA256. The Makefile auto-detects the platform.

### Windows

Use [WSL](https://learn.microsoft.com/en-us/windows/wsl/install) (Windows Subsystem for Linux), then follow the Linux instructions above.

## Run

```
make run
```

Builds and starts the server on port 4200 with AddressSanitizer leak detection enabled. Or run directly:

```
./world
```

## Connect

```
telnet localhost 4200
```

Or with readline support (command history, arrow keys):

```
rlwrap telnet localhost 4200
```

## Commands

| Command | Description |
|---|---|
| `look` | See the current room |
| `north`, `south`, `east`, `west`, `ne`, `se`, `nw`, `sw`, `up`, `down` | Move (abbreviations work) |
| `say <message>` | Talk to others in the room |
| `roomedit <subcommand>` | Edit rooms (see OLC above) |
| `save` | Save the world to disk |
| `quit` | Disconnect |

## Architecture

```
server/
  main.c      — server loop, connections, login flow
  main.h      — core includes, defines, connection struct
  world.c     — room management, linked list, save/load, directions
  world.h     — room struct, direction enum
  command.c   — command table, parser, look/move/say/quit
  command.h   — command struct, function declarations
  olc.c       — online creation commands (roomedit)
  olc.h       — OLC declarations
  player.c    — player save/load, password hashing
  player.h    — player struct, role defines
  compat.h    — platform abstraction (macOS/Linux crypto)
  color.c     — ANSI color helpers
  color.h     — color code defines
```

Single-threaded, select()-based event loop. No dependencies beyond libc and CommonCrypto (macOS) or OpenSSL (Linux).

## World file format

Human-readable flat file (`world.dat`):

```
ROOM 1
NAME The Void
DESCRIPTION A swirling mist with no floor, ceiling, or walls.
EXIT down 2
ENDROOM
```

## Roadmap

- Room flags (dark, no-chat, safe zone)
- Embedded scripting language for room triggers and automation
- Item/object system
- Persistent chat history per room
- Custom telnet client with readline and color support
- Web client (the real frontend — telnet is just for testing the server)

## License

Do whatever you want with it.
