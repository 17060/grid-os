# Grid OS Networking (6.4)

Grid OS includes a minimal virtio-net stack, static DNS names, a multi-session TCP client, and plain HTTP/1.1 fetch.

## Addresses

| Role | IPv4 | DNS alias |
|------|------|-----------|
| Guest (Grid) | `10.0.2.15` | `grid`, `host` |
| QEMU gateway | `10.0.2.2` | `gateway`, `gw`, `ai`, `btc` |
| Localhost | `127.0.0.1` | `localhost` |

Host bridges (`make ai-bridge`, `make btc-bridge`) listen on the gateway address from the guest's perspective.

## Static DNS

`net_resolve_host()` accepts literal IPv4 or a hostname from the table above (case-insensitive):

```text
grid> net ping gateway
grid> http get grid /
```

GridBASIC:

```basic
10 PRINT GRID.PING("gateway")
20 END
```

Unknown hostnames return an error — there is no real DNS resolver.

## Multi-session TCP

Up to **8 concurrent outbound TCP connections**, each with a unique local port. Inbound segments dispatch by `(remote_ip, remote_port, local_port)`.

This allows IRC, HTTP, AI bridge, and BTC bridge sessions to coexist.

Host tests (`make test-host-tcp`) verify dual/triple-port dispatch and slot limits.

## HTTP client

```text
grid> http get gateway /
```

- HTTP/1.1 request with `Connection: keep-alive`
- Reuses the TCP session for repeated requests to the same host:port
- Server `Connection: close` or socket errors tear down the pool
- Response body truncated at caller buffer size (2048 bytes in shell)

Paths must start with `/`. Overlong paths are rejected before send.

## IRC + HTTP + bridges together

Typical workflow:

1. `make ai-bridge` and/or `make btc-bridge` on the host
2. `make run` in another terminal
3. In Grid OS:
   ```text
   grid> irc connect gateway 6667 mynick
   grid> http get gateway /
   grid> ai ask hello
   grid> btc status
   ```

Each service uses its own TCP slot.

## ICMP

```text
grid> net ping 10.0.2.2
grid> net ping gateway
grid> net status
```

Idle polling in the shell and IDE drains the RX queue so ARP/ICMP replies are handled promptly.

## See also

- [GETTING_STARTED.md](GETTING_STARTED.md) — first boot
- [COMMANDS.md](COMMANDS.md) — full command reference
