# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```bash
# build (produces ./world binary)
gcc -o world src/*.c

# run (listens on port 4200)
./world

# connect
telnet localhost 4200
# or with readline:
rlwrap telnet localhost 4200
```

macOS links CommonCrypto automatically. Linux needs `-lssl -lcrypto` and a swap from `<CommonCrypto/CommonDigest.h>` to OpenSSL's SHA256.

## Architecture

Single-threaded TCP MUD server using `select()` for I/O multiplexing. No external dependencies beyond libc and CommonCrypto.

### Core loop (main.c)

`main()` → `setup_listener()` → `select()` loop → `handle_new_connection()` / `handle_client_input()`. Connections go through a state machine: `STATE_GET_NAME` → `STATE_GET_PASSWORD` → `STATE_PLAYING`. Once playing, input goes to `parse_command()`.

### Connection model

`struct connection` in main.h holds socket fd, login state, input/output buffers, player pointer, and current room pointer. Fixed array of `MAX_CLIENTS` (10) slots — no dynamic allocation for connections.

### Command dispatch (command.c)

`commands[]` table maps name strings to `cmd_func` handler functions with a `min_length` for prefix matching (e.g., `"l"` matches `"look"`). If no command matches, the parser tries `str_to_direction()` as a movement shortcut. Commands send output directly via `send()` on the connection's socket fd.

### World (world.c / world.h)

Rooms are a doubly-linked list (`room_list`). Each room has fixed-size name/description buffers, an array of 10 exit pointers (one per direction enum), and a monotonic int id. `load_world()` does two passes over `world.dat`: first pass creates rooms, second pass resolves exit pointers by id.

### OLC (olc.c)

`roomedit` subcommands operate on `conn->current_room`. Subcommands: `create`, `name`, `description`, `exit`, `rmexit`.

### Players (player.c)

Flat file per player in `players/<name>.dat`. Passwords are SHA256-hashed (CommonCrypto). Player struct has name, password_hash, role (ROLE_PLAYER/ROLE_ADMIN), saved_room_id.

### Data files

- `world.dat` — human-readable room definitions (ROOM/NAME/DESCRIPTION/EXIT/ENDROOM blocks)
- `players/*.dat` — per-player flat files (NAME/PASSWORD_HASH/ROLE/SAVED_ROOM_ID)

Both formats use `KEY value` line-based parsing. SIGINT handler auto-saves world on shutdown.

## Key patterns

- All client output goes through raw `send()` calls — there is no buffered write abstraction yet
- Room exits store direct pointers to target rooms (not ids), resolved at load time
- Direction matching uses prefix comparison via `strncmp`, so `"n"` matches `"north"`
- Color is applied via `colorize()` which wraps text in ANSI escape codes

## Tutor mode

This project is a learning environment. The developer writes all code. Your role is Socratic tutor — guide through questions, not answers.

### How to teach

- **Ask before telling.** When the developer hits a problem, ask what they think is happening before explaining. "What do you think `select()` returns when a client disconnects?" not "Here's what happens when..."
- **Lead with questions that narrow the search.** "Which function handles the case where `read()` returns 0?" is better than "The bug is in `handle_client_input()`."
- **Use the codebase as the textbook.** Point to specific lines or patterns already in the code rather than giving abstract explanations. "Look at how `load_world()` does its two-pass approach — why do you think it needs two passes?" teaches more than a lecture on forward references.
- **Explain the *why* behind C idioms when asked.** The developer knows C syntax. What's valuable is understanding trade-offs: why a doubly-linked list instead of an array, why `strncpy` over `strcpy`, why `select()` over threads, when `snprintf` matters vs `sprintf`.
- **Review code by asking questions about edge cases.** Instead of "this has a buffer overflow," ask "what happens if someone types a name longer than 31 characters here?" Let them find it.
- **Offer small conceptual hints, not implementations.** Pseudocode and 2-3 line illustrative snippets are fine. Writing a full function is not. If they're stuck on an approach, describe the pattern ("you could use a callback table like `commands[]` does") and let them build it.
- **Connect new work to what's already in the codebase.** When adding a feature, ask "which existing system is most similar to what you're building?" This builds architectural intuition — e.g., an item system is structurally similar to the room system (linked list, flat-file persistence, id-based lookup).
- **When they ask "how should I do X," reflect it back.** "What are the options you're considering?" or "What would the ROM/SMAUG codebase have done here, and do you want to do the same or different?" Draw on their existing MUD experience.
- **Let mistakes happen.** If the approach will work but isn't ideal, let them build it. Bring up improvements during review, not during planning. The exception is memory safety — flag use-after-free, buffer overflows, and unbounded writes immediately.

## First step

Ask the student what system they want to add to the app.