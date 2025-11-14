# Awale Game Server

## Overview

A terminal-based multiplayer implementation of the traditional African strategy game **Awale** (also known as Oware, Wari, or Mancala), written  in pure C. This project includes a game server with support for concurrent multiplayer matches, spectating, friend lists, and ranking systems.

## Game Rules

The Awale game follows these standard rules:

1. **Setup**: The board has 12 pits (6 per player), each starting with 4 seeds
2. **Turns**: Players alternate turns, selecting a pit on their side
3. **Sowing**: Seeds from the selected pit are sown counter-clockwise, one per pit
4. **Empty Start**: The starting pit is left empty (seeds go to following pits)
5. **Capturing**: After sowing, if the last seed lands in an opponent's pit and that pit now has 2 or 3 seeds, those seeds are captured
6. **Chain Capturing**: Continue capturing backwards if previous opponent pits also have 2 or 3 seeds
7. **Feeding Rule**: If your opponent has no seeds, you must give them seeds if possible
8. **Protection**: You cannot capture all of your opponent's seeds
9. **Winning**: Game ends when one player captures 25+ seeds or no more moves are possible
10. **Victory**: Player with the most seeds wins

## Compilation

### Prerequisites
- GCC compiler (or any C99 compatible compiler)
- Make build tool
- Unix-like environment (Linux, macOS, or similar)

### Build Instructions

```bash
# Clean previous builds (optional)
make clean

# Compile all binaries
make
```

This will generate four executables in the `bin/` directory:
- `bin/server` - The multiplayer game server
- `bin/client` - The client application
- `bin/offline` - Standalone single-player game
- `bin/test` - Test mode for custom board configurations

## Running the Game

### Starting the Server

```bash
./bin/server --port <port_number>
```

**Options:**
- `--port <port_number>` - Specify the port number (default: 9000)
- `--help` - Display help information

**Example:**
```bash
./bin/server --port 9000
```

### Starting the Client

```bash
./bin/client --name <username> [--ip <address>] [--port <port>]
```

**Options:**
- `--name <username>` - Your username (required)
- `--ip <address>` - Server IP address (default: 127.0.0.1)
- `--port <port>` - Server port number (default: 9000)
- `--help` - Display help information

**Example:**
```bash
./bin/client --name alice --ip 127.0.0.1 --port 9000
```

### Offline Mode

To play a single-player game without connecting to a server:

```bash
./bin/offline
```

### Test Mode

To test the game with a custom board configuration:

```bash
./bin/test
```

## Client Commands

Once connected to the server, you can use the following commands:

| Command | Usage | Description |
|---------|-------|-------------|
| `challenge <username>` | `challenge alice` | Challenge another player to a game |
| `accept <username>` | `accept alice` | Accept a challenge from another player |
| `refuse <username>` | `refuse alice` | Decline a challenge from another player |
| `cancel <username>` | `cancel alice` | Cancel a challenge you sent |
| `move <pit>` | `move 2` | Make a move by selecting a pit (0-11) |
| `games` | `games` | List all currently running games |
| `watch <match_id>` | `watch 1` | Watch a live match |
| `unwatch <match_id>` | `unwatch 1` | Stop watching a match |
| `watchreplay <match_id>` | `watchreplay 1` | Watch a replay of a previous match |

### Chat & Messaging

| Command | Usage | Description |
|---------|-------|-------------|
| `msg <message>` | `msg Hello everyone!` | Send a public chat message |
| `pm <username> <message>` | `pm alice Hi there` | Send a private message to a player |
| `list` | `list` | Show all connected players |

### User Profile

| Command | Usage | Description |
|---------|-------|-------------|
| `bio <text>` | `bio I love Awale!` | Set your user biography |
| `getbio <username>` | `getbio alice` | View another player's biography |
| `friends` | `friends` | List your friends |
| `addfriend <username>` | `addfriend alice` | Send a friend request |
| `acceptfriend <username>` | `acceptfriend alice` | Accept a friend request |
| `refusefriend <username>` | `refusefriend alice` | Decline a friend request |
| `private <on\|off>` | `private on` | Toggle private mode (only friends can watch) |

### Rankings & Stats

| Command | Usage | Description |
|---------|-------|-------------|
| `ranking` | `ranking` | View the player ranking list |

### General

| Command | Usage | Description |
|---------|-------|-------------|
| `help` | `help` | Display all available commands |
| `quit` | `quit` | Disconnect from the server |

## Quick Start Example

### Terminal 1 - Start the Server
```bash
./bin/server --port 9000
```

### Terminal 2 - First Player
```bash
./bin/client --name alice --port 9000
> challenge bob
```

### Terminal 3 - Second Player
```bash
./bin/client --name bob --port 9000
> accept alice
```

Now alice and bob are in a game and can make moves using the `move` command.