# IRC Hive Mind

Collective intelligence over IRC — **iOS client** + **Grid OS bot** + **host bridges**.

## Concept

Many IRC participants contribute fragments; the hive ingests channel traffic, builds rolling memory, and synthesizes summaries (optionally via AI). Grid OS runs a companion bot on Flynn's Grid; the iOS app is your pocket window into the swarm.

```
  IRC channels (#grid, …)
        │
        ├─► iOS IRCHiveMind app   (ZNC/direct, push alerts, hive memory)
        │
        ├─► Host make irc-bridge  (gateway:6667 → local #grid or relay)
        │
        └─► Grid OS hivemind bot  (vault memory → GRID.AI.ASK$ → IRC.SAY)
```

---

## Quick start (full stack)

**Terminal 1 — Grid OS**

```bash
make run
```

**Terminal 2 — IRC bridge (local #grid hive server)**

```bash
make irc-bridge
```

**Terminal 3 — AI summaries (optional)**

```bash
make ai-bridge
```

**Terminal 4 — Push relay for iOS (optional)**

```bash
make hive-push-relay
```

**Grid OS shell**

```text
grid> basic run /programs/irc-hive-mind.bas
```

**iOS app** — Settings → host `127.0.0.1` (or Mac LAN IP), port `6667`, channel `#grid`, mode **Direct IRC**.

---

## Grid OS (Flynn's Grid)

### IRC host bridge

| Mode | Command | Use case |
|------|---------|----------|
| **local** (default) | `make irc-bridge` | Offline demos; built-in `#grid` + `hivebot` |
| **relay** | `GRIDIRC_MODE=relay make irc-bridge` | Proxy to `irc.libera.chat:6667` |
| **znc** | `GRIDIRC_MODE=znc GRIDIRC_UPSTREAM=127.0.0.1:6697 GRIDIRC_ZNC_PASS=secret make irc-bridge` | Proxy through ZNC (injects PASS for Grid clients) |

Guest address: `gateway:6667` (QEMU user-net → host `0.0.0.0:6667`).

### Hive bot

```text
grid> basic run /programs/irc-hive-mind.bas
grid> basic mod run irc-hive-mind
```

1. Connects as `hivemind`, joins `#grid`
2. Ingests IRC into vault `hive_memory`
3. Summarizes via `GRID.AI.ASK$`
4. Posts summary to `#grid`, stores `hive_summary`

---

## iOS app

### Requirements

- Xcode 15+, iOS 17+
- IRC server, `make irc-bridge`, or ZNC bouncer

### Xcode setup

1. New iOS SwiftUI project `IRCHiveMind`
2. Add all files from `ios/IRCHiveMind/`
3. Enable **Push Notifications** capability (for remote token)
4. Merge `Info.plist` background modes and notification usage string
5. Build on device for APNs; simulator works for local notifications

### Connection modes

| Mode | Settings | When to use |
|------|----------|-------------|
| **Direct IRC** | Host + port | `make irc-bridge`, Libera, etc. |
| **ZNC bouncer** | ZNC host, password, optional `nick/network` | Always-on phone; ZNC holds backlog |

ZNC flow: `PASS` → `NICK nick/network` → `USER` → auto-join configured channels.

### Push notifications

**Local notifications** (no server required):

- @mentions of your nick
- Configurable keywords (`!hive`, `consensus`, …)
- Hive synthesis / consensus summaries
- New open questions detected in channel

**Remote push relay** (dev/lab):

```bash
make hive-push-relay   # http://0.0.0.0:8770
```

iOS Settings → Push relay URL → `http://<your-mac-lan-ip>:8770`

The app registers its APNs device token with `POST /register`. Notifications also call `POST /notify`. Production deployments should forward to Apple Push Notification service.

### App tabs

| Tab | Purpose |
|-----|---------|
| **Channels** | Connect, chat, channel picker |
| **Hive** | Memory, open questions, Synthesize |
| **Ask** | AI query against hive memory |
| **Settings** | IRC/ZNC, notifications, push relay, AI |

### Pairing with Grid OS

Use the same IRC server and `#grid`. Run `make irc-bridge` locally; iOS points at the Mac's IP. Both nodes feed one hive.

---

## Files

| Path | Role |
|------|------|
| `tools/gridirc_bridge.py` | Host IRC bridge (`make irc-bridge`) |
| `tools/hive_push_relay.py` | Push token relay (`make hive-push-relay`) |
| `programs/irc-hive-mind.bas` | Grid OS hive bot |
| `ios/IRCHiveMind/` | SwiftUI iOS app |
| `docs/NETWORKING.md` | Bridge ports and workflow |
