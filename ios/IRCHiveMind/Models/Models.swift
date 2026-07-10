import Foundation

struct IRCMessage: Identifiable, Codable, Hashable, Sendable {
    let id: UUID
    let channel: String
    let sender: String
    let text: String
    let receivedAt: Date
    let isSystem: Bool

    init(
        id: UUID = UUID(),
        channel: String,
        sender: String,
        text: String,
        receivedAt: Date = .now,
        isSystem: Bool = false
    ) {
        self.id = id
        self.channel = channel
        self.sender = sender
        self.text = text
        self.receivedAt = receivedAt
        self.isSystem = isSystem
    }
}

struct HiveMemoryEntry: Identifiable, Codable, Hashable, Sendable {
    let id: UUID
    let sourceChannel: String
    let snippet: String
    let createdAt: Date

    init(id: UUID = UUID(), sourceChannel: String, snippet: String, createdAt: Date = .now) {
        self.id = id
        self.sourceChannel = sourceChannel
        self.snippet = snippet
        self.createdAt = createdAt
    }
}

struct HiveSummary: Codable, Hashable, Sendable {
    var text: String
    var updatedAt: Date
    var messageCount: Int

    static let empty = HiveSummary(text: "The hive is quiet.", updatedAt: .now, messageCount: 0)
}

struct AppSettings: Codable, Equatable, Sendable {
    var serverHost: String
    var serverPort: UInt16
    var nickname: String
    var username: String
    var realName: String
    var channels: [String]
    var aiEnabled: Bool
    var aiEndpoint: String
    var aiAPIKey: String
    var aiModel: String
    var memoryLimit: Int
    var autoSummarizeEvery: Int

    static let `default` = AppSettings(
        serverHost: "irc.libera.chat",
        serverPort: 6667,
        nickname: "HiveMind",
        username: "hive",
        realName: "IRC Hive Mind",
        channels: ["#swift", "#testhive"],
        aiEnabled: false,
        aiEndpoint: "https://api.openai.com/v1/chat/completions",
        aiAPIKey: "",
        aiModel: "gpt-4o-mini",
        memoryLimit: 200,
        autoSummarizeEvery: 12
    )
}

enum IRCConnectionState: Equatable, Sendable {
    case disconnected
    case connecting
    case connected
    case error(String)
}
