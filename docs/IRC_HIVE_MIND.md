# IRC Hive Mind

Collective intelligence over IRC — **iOS client** + **Grid OS bot**.

## Concept

Many IRC participants contribute fragments; the hive ingests channel traffic, builds rolling memory, and synthesizes summaries (optionally via AI). Grid OS runs a companion bot on Flynn's Grid; the iOS app is your pocket window into the swarm.

```
  IRC channels (#grid, #swift, …)
        │
        ├─► iOS IRCHiveMind app  (ingest → memory → Ask / Synthesize)
        │
        └─► Grid OS hivemind bot (vault memory → GRID.AI.ASK$ → IRC.SAY)
```

---

## Grid OS (Flynn's Grid)

### Run the hive bot

Prerequisites on the **host**:

```bash
# Terminal 1: Grid OS
make run

# Terminal 2: AI bridge (optional, for summaries)
make ai-bridge

# Terminal 3: IRC server reachable as gateway:6667 from the guest
# (any IRC daemon bound for QEMU user-net forwarding)
```

From the Flynn shell:

```text
grid> basic run /programs/irc-hive-mind.bas
```

Or use the IDE module guide:

```text
grid> basic mod run irc-hive-mind
```

### What the bot does

1. Connects to `gateway:6667` as `hivemind`
2. Joins `#grid` and announces itself
3. Polls IRC for 10 ticks, ingesting lines into vault key `hive_memory`
4. Calls `GRID.AI.ASK$` to summarize (falls back if no AI bridge)
5. Posts a short summary to `#grid` and stores `hive_summary` in the vault

### Vault keys

| Key | Contents |
|-----|----------|
| `hive_memory` | Rolling IRC transcript (last ~900 chars) |
| `hive_summary` | Latest AI synthesis |

---

## iOS app

### Requirements

- Xcode 15+
- iOS 17+ (SwiftUI, `@Observable`)
- An IRC server (Libera, ergo, local ZNC, or Grid OS `gateway:6667` via tunnel)

### Setup in Xcode

1. **File → New → Project → iOS App**
   - Product name: `IRCHiveMind`
   - Interface: SwiftUI
   - Language: Swift
2. Delete the template `ContentView.swift` and `IRCHiveMindApp.swift`.
3. Drag the entire `ios/IRCHiveMind/` folder into the project (copy items if needed).
4. Add **Outgoing Connections (Client)** capability if you use a custom network entitlement profile.
5. Build and run on simulator or device.

### App tabs

| Tab | Purpose |
|-----|---------|
| **Channels** | Connect, chat, channel picker |
| **Hive** | Collective memory, open questions, manual Synthesize |
| **Ask** | Query the hive memory with AI |
| **Settings** | Server, nick, channels, AI endpoint + API key |

### AI (optional)

Enable **AI synthesis** in Settings and provide an OpenAI-compatible API key + endpoint. The hive uses `chat/completions` to:

- **Synthesize** — bullet summary of channel memory
- **Ask** — answer questions grounded in memory

Works with OpenAI, local LM Studio, Ollama OpenAI shim, etc.

### Pairing with Grid OS

Point the iOS app at the same IRC server and channels the Grid bot uses (e.g. `#grid`). Both nodes feed the same channel organism — one from your phone, one from Flynn's Grid.

---

## Files

### Grid OS

- `programs/irc-hive-mind.bas` — hive bot
- `packages/flynn-ide-tools/modules/irc-hive-mind.bas` — IDE guide module

### iOS

- `ios/IRCHiveMind/` — SwiftUI source
- `ios/IRCHiveMind/IRCHiveMindApp.swift` — entry point
- `ios/IRCHiveMind/Services/IRCClient.swift` — TCP IRC client
- `ios/IRCHiveMind/Services/HiveBrain.swift` — memory + open questions
- `ios/IRCHiveMind/Services/AIService.swift` — OpenAI-compatible synthesis
