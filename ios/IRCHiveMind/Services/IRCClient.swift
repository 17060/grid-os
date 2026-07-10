import Foundation
import Network

@MainActor
@Observable
final class IRCClient {
    private(set) var state: IRCConnectionState = .disconnected
    private(set) var messages: [IRCMessage] = []

    private var connection: NWConnection?
    private var buffer = ""
    private var settings = AppSettings.default
    private var registered = false

    func connect(settings: AppSettings) {
        disconnect()
        self.settings = settings
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
                    self.sendRaw("NICK \(self.settings.nickname)")
                    self.sendRaw("USER \(self.settings.username) 0 * :\(self.settings.realName)")
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

    func disconnect() {
        if case .connected = state {
            sendRaw("QUIT :hivemind signing off")
        }
        connection?.cancel()
        connection = nil
        registered = false
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

        if line.hasPrefix("PING ") {
            let token = String(line.dropFirst(5))
            sendRaw("PONG :\(token)")
            return
        }

        if line.contains(" 001 ") || line.contains(" 376 ") {
            registered = true
            for channel in settings.channels {
                join(channel)
            }
            appendSystem("Joined hive channels.")
            return
        }

        if let priv = parsePrivmsg(line) {
            appendMessage(channel: priv.channel, sender: priv.sender, text: priv.text)
        }
    }

    private func parsePrivmsg(_ line: String) -> (sender: String, channel: String, text: String)? {
        guard let privRange = line.range(of: " PRIVMSG ") else { return nil }
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

    private func appendMessage(channel: String, sender: String, text: String) {
        messages.append(IRCMessage(channel: channel, sender: sender, text: text))
        if messages.count > 500 {
            messages.removeFirst(messages.count - 500)
        }
    }

    private func appendLocalMessage(channel: String, sender: String, text: String) {
        appendMessage(channel: channel, sender: sender, text: text)
    }

    private func appendSystem(_ text: String) {
        messages.append(IRCMessage(channel: "system", sender: "hive", text: text, isSystem: true))
    }
}
