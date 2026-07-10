# Grid OS Networking (6.5)

Grid OS includes a minimal virtio-net stack, static DNS names, GFS `/etc/hosts`, a UDP DNS resolver, a multi-session TCP client, and plain HTTP/1.1 GET/POST.

## Addresses

| Role | IPv4 | DNS alias |
|------|------|-----------|
| Guest (Grid) | `10.0.2.15` | `grid`, `host` |
| QEMU gateway | `10.0.2.2` | `gateway`, `gw`, `ai`, `btc` |
| Localhost | `127.0.0.1` | `localhost` |

Host bridges (`make ai-bridge`, `make btc-bridge`, `make https-bridge`) listen on the gateway address from the guest's perspective.

## Name resolution order

`net_resolve_host()` tries, in order:

1. Literal IPv4 (`10.0.2.2`)
2. Built-in static table (`gateway`, `grid`, `localhost`, `ai`, `btc`, …)
3. GFS `/etc/hosts` (editable; seeded on Flynn disk)
4. UDP DNS A query to `10.0.2.2:53` (QEMU user-net resolver)

```text
grid> net ping gateway
grid> http get grid /
grid> cat /etc/hosts
```

GridBASIC:

```basic
10 PRINT GRID.PING("gateway")
20 END
```

## Multi-session TCP

Up to **8 concurrent outbound TCP connections**, each with a unique local port. Inbound segments dispatch by `(remote_ip, remote_port, local_port)`.

This allows IRC, HTTP, AI bridge, BTC bridge, and HTTPS proxy sessions to coexist.

Host tests (`make test-host-tcp`) verify dual/triple-port dispatch and slot limits.

## HTTP client

```text
grid> http get gateway /
grid> http get gateway 8080 /api
grid> http post gateway /hook {"event":"grid"}
```

- HTTP/1.1 with `Connection: keep-alive`
- Reuses the TCP session for repeated requests to the same host:port
- POST sends `Content-Length` + plain-text body
- Honors response `Content-Length` (surplus bytes stay in the pool for the next request)
- Sends the resolved `Host:` header (not a hardcoded name)
- Server `Connection: close` or socket errors tear down the pool
- Response body truncated at caller buffer size (2048 bytes in shell)

Paths must start with `/`. Overlong paths are rejected before send.

Host-side keep-alive reuse can be exercised with `tools/http_test_server.py` (not wired into CI).
E2E (`make test-e2e`) covers basictest, `net ping gateway`, spawn gridsh, and poweroff.

## HTTPS (host bridge)

In-guest TLS is not supported. Use the host proxy:

```bash
make https-bridge   # listens on :8768 by default
```

From Grid OS, open a TCP connection to `gateway:8768` and send a normal HTTP/1.1 request with a `Host:` header for the real HTTPS site. The bridge forwards over TLS and returns the upstream response.

## IRC + HTTP + bridges together

Typical workflow:

1. `make irc-bridge`, `make ai-bridge`, `make btc-bridge`, and/or `make https-bridge` on the host
2. `make run` in another terminal
3. In Grid OS:
   ```text
   grid> irc connect gateway 6667 mynick
   grid> irc join #grid
   grid> basic run /programs/irc-hive-mind.bas
   grid> http get gateway /
   grid> ai ask hello
   grid> btc status
   ```

Each service uses its own TCP slot.

## IRC host bridge (7.1+)

Grid OS has no in-guest IRC daemon. Use the host bridge so `gateway:6667` reaches a real or local IRC server:

```bash
make irc-bridge                              # local #grid hive server (default)
GRIDIRC_MODE=relay make irc-bridge           # relay to irc.libera.chat:6667
GRIDIRC_MODE=znc GRIDIRC_UPSTREAM=127.0.0.1:6697 GRIDIRC_ZNC_PASS=secret make irc-bridge
```

From Grid OS:

```text
grid> irc connect gateway 6667 hivemind
grid> irc join #grid
grid> irc say #grid hello hive
```

See [IRC_HIVE_MIND.md](IRC_HIVE_MIND.md) for the collective-memory bot and iOS companion app.

## Hive push relay (iOS companion)

Optional dev relay for IRC Hive Mind push tokens:

```bash
make hive-push-relay   # http://0.0.0.0:8770
```

iOS Settings → Push relay URL → `http://<host-ip>:8770`

## ICMP

```text
grid> net ping 10.0.2.2
grid> net ping gateway
grid> net status
```

Idle polling in the shell and IDE drains the RX queue so ARP/ICMP/UDP replies are handled promptly.

## See also

- [GETTING_STARTED.md](GETTING_STARTED.md) — first boot walkthrough
- [COMMANDS.md](COMMANDS.md) — shell reference
