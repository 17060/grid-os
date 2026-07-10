import Foundation
import Network

@MainActor
@Observable
final class IRCClient {
    private(set) var state: IRCConnectionState = .disconnected
    private(set) var messages: [IRCMessage] = []

    var onMessage: ((IRCMessage) -> Void)?

    private var connection: NWConnection?
    private var buffer = ""
    private var settings = AppSettings.default
    private var registered = false
    private var handshakeSent = false

    func connect(settings: AppSettings) {
        disconnect()
        self.settings = settings
        self.handshakeSent = false
        state = .connecting

        let host = NWEndpoint.Host(settings.serverHost)
        let port = NWEndpoint.Port(rawValue: settings.serverPort) ?? 6667
        let conn = NWConnection(host: host, port: port, using: .tcp)
        connection = conn

        conn.stateUpdateHandler = { [weak self] newState in
            Task { @MainActor in
                guard let self else { return }
                switch newState {
                case .ready:
                    self.sendHandshake()
                    self.state = .connected
                case .failed(let error):
                    self.state = .error(error.localizedDescription)
                    self.disconnect()
                case .cancelled:
                    self.state = .disconnected
                default:
                    break
                }
            }
        }

        conn.start(queue: .global(qos: .userInitiated))
        receiveLoop()
    }

    private func sendHandshake() {
        guard !handshakeSent else { return }
        handshakeSent = true

        if settings.connectionMode == .znc, !settings.zncPassword.isEmpty {
            sendRaw("PASS \(settings.zncPassword)")
        }

        sendRaw("NICK \(settings.effectiveNickname)")
        sendRaw("USER \(settings.username) 0 * :\(settings.realName)")
    }

    func disconnect() {
        if case .connected = state {
            sendRaw("QUIT :hivemind signing off")
        }
        connection?.cancel()
        connection = nil
        registered = false
        handshakeSent = false
        state = .disconnected
    }

    func join(_ channel: String) {
        let normalized = channel.hasPrefix("#") ? channel : "#\(channel)"
        sendRaw("JOIN \(normalized)")
    }

    func say(_ target: String, _ text: String) {
        let trimmed = text.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !trimmed.isEmpty else { return }
        sendRaw("PRIVMSG \(target) :\(trimmed)")
        appendLocalMessage(channel: target, sender: settings.nickname, text: trimmed)
    }

    private func sendRaw(_ line: String) {
        guard let connection else { return }
        let payload = (line + "\r\n").data(using: .utf8) ?? Data()
        connection.send(content: payload, completion: .contentProcessed { _ in })
    }

    private func receiveLoop() {
        connection?.receive(minimumIncompleteLength: 1, maximumLength: 4096) { [weak self] data, _, _, error in
            Task { @MainActor in
                guard let self else { return }
                if let data, !data.isEmpty {
                    self.buffer += String(decoding: data, as: UTF8.self)
                    self.drainBuffer()
                }
                if error != nil {
                    self.state = .error(error?.localizedDescription ?? "receive failed")
                    self.disconnect()
                    return
                }
                self.receiveLoop()
            }
        }
    }

    private func drainBuffer() {
        while let range = buffer.range(of: "\r\n") ?? buffer.range(of: "\n") {
            let line = String(buffer[buffer.startIndex..<range.lowerBound])
            buffer = String(buffer[range.upperBound...])
            handleServerLine(line)
        }
    }

    private func handleServerLine(_ line: String) {
        guard !line.isEmpty else { return }

        if line.hasPrefix("PING ") || line.hasPrefix(":") && line.contains(" PING ") {
            let token: String
            if let ping = line.range(of: " PING ") {
                token = String(line[ping.upperBound...]).trimmingCharacters(in: .whitespaces)
            } else {
                token = String(line.dropFirst(5))
            }
            sendRaw("PONG :\(token.trimmingCharacters(in: CharacterSet(charactersIn: ":")))")
            return
        }

        if settings.connectionMode == .znc,
           line.localizedCaseInsensitiveContains("znc") && line.contains("***") {
            appendSystem("ZNC bouncer connected.")
        }

        if line.contains(" 001 ") || line.contains(" 376 ") || line.contains(" 905 ") {
            if !registered {
                registered = true
                for channel in settings.channels {
                    join(channel)
                }
                let modeLabel = settings.connectionMode == .znc ? "ZNC bouncer" : "IRC"
                appendSystem("Joined hive channels via \(modeLabel).")
            }
            return
        }

        if let priv = parsePrivmsg(line) {
            let message = IRCMessage(channel: priv.channel, sender: priv.sender, text: priv.text)
            appendMessage(message)
            onMessage?(message)
        }
    }

    private func parsePrivmsg(_ line: String) -> (sender: String, channel: String, text: String)? {
        guard let privRange = line.range(of: " PRIVMSG ") ?? line.range(of: " NOTICE ") else { return nil }
        let prefix = String(line[..<privRange.lowerBound])
        let rest = String(line[privRange.upperBound...])

        guard let colon = rest.firstIndex(of: ":") else { return nil }
        let target = String(rest[..<colon]).trimmingCharacters(in: .whitespaces)
        let text = String(rest[rest.index(after: colon)...])

        let sender: String
        if prefix.hasPrefix(":") {
            let nickPart = String(prefix.dropFirst()).split(separator: "!").first.map(String.init) ?? "unknown"
            sender = nickPart
        } else {
            sender = "server"
        }

        return (sender, target, text)
    }

    private func appendMessage(_ message: IRCMessage) {
        messages.append(message)
        if messages.count > 500 {
            messages.removeFirst(messages.count - 500)
        }
    }

    private func appendLocalMessage(channel: String, sender: String, text: String) {
        let message = IRCMessage(channel: channel, sender: sender, text: text)
        appendMessage(message)
        onMessage?(message)
    }

    private func appendSystem(_ text: String) {
        appendMessage(IRCMessage(channel: "system", sender: "hive", text: text, isSystem: true))
    }
}
