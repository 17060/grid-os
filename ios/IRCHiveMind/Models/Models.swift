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

enum IRCConnectionMode: String, Codable, CaseIterable, Sendable {
    case direct
    case znc

    var label: String {
        switch self {
        case .direct: return "Direct IRC"
        case .znc: return "ZNC bouncer"
        }
    }
}

struct AppSettings: Codable, Equatable, Sendable {
    var serverHost: String
    var serverPort: UInt16
    var nickname: String
    var username: String
    var realName: String
    var channels: [String]
    var connectionMode: IRCConnectionMode
    var zncPassword: String
    var zncNetwork: String
    var aiEnabled: Bool
    var aiEndpoint: String
    var aiAPIKey: String
    var aiModel: String
    var memoryLimit: Int
    var autoSummarizeEvery: Int
    var notifyEnabled: Bool
    var notifyMentions: Bool
    var notifyKeywords: [String]
    var notifyConsensus: Bool
    var notifyOpenQuestions: Bool
    var pushRelayURL: String
    var pushDeviceToken: String

    static let `default` = AppSettings(
        serverHost: "127.0.0.1",
        serverPort: 6667,
        nickname: "HiveMind",
        username: "hive",
        realName: "IRC Hive Mind",
        channels: ["#grid"],
        connectionMode: .direct,
        zncPassword: "",
        zncNetwork: "",
        aiEnabled: false,
        aiEndpoint: "https://api.openai.com/v1/chat/completions",
        aiAPIKey: "",
        aiModel: "gpt-4o-mini",
        memoryLimit: 200,
        autoSummarizeEvery: 12,
        notifyEnabled: true,
        notifyMentions: true,
        notifyKeywords: ["!hive", "consensus", "urgent"],
        notifyConsensus: true,
        notifyOpenQuestions: true,
        pushRelayURL: "http://127.0.0.1:8770",
        pushDeviceToken: ""
    )

    enum CodingKeys: String, CodingKey {
        case serverHost, serverPort, nickname, username, realName, channels
        case connectionMode, zncPassword, zncNetwork
        case aiEnabled, aiEndpoint, aiAPIKey, aiModel
        case memoryLimit, autoSummarizeEvery
        case notifyEnabled, notifyMentions, notifyKeywords
        case notifyConsensus, notifyOpenQuestions
        case pushRelayURL, pushDeviceToken
    }

    init(
        serverHost: String,
        serverPort: UInt16,
        nickname: String,
        username: String,
        realName: String,
        channels: [String],
        connectionMode: IRCConnectionMode,
        zncPassword: String,
        zncNetwork: String,
        aiEnabled: Bool,
        aiEndpoint: String,
        aiAPIKey: String,
        aiModel: String,
        memoryLimit: Int,
        autoSummarizeEvery: Int,
        notifyEnabled: Bool,
        notifyMentions: Bool,
        notifyKeywords: [String],
        notifyConsensus: Bool,
        notifyOpenQuestions: Bool,
        pushRelayURL: String,
        pushDeviceToken: String
    ) {
        self.serverHost = serverHost
        self.serverPort = serverPort
        self.nickname = nickname
        self.username = username
        self.realName = realName
        self.channels = channels
        self.connectionMode = connectionMode
        self.zncPassword = zncPassword
        self.zncNetwork = zncNetwork
        self.aiEnabled = aiEnabled
        self.aiEndpoint = aiEndpoint
        self.aiAPIKey = aiAPIKey
        self.aiModel = aiModel
        self.memoryLimit = memoryLimit
        self.autoSummarizeEvery = autoSummarizeEvery
        self.notifyEnabled = notifyEnabled
        self.notifyMentions = notifyMentions
        self.notifyKeywords = notifyKeywords
        self.notifyConsensus = notifyConsensus
        self.notifyOpenQuestions = notifyOpenQuestions
        self.pushRelayURL = pushRelayURL
        self.pushDeviceToken = pushDeviceToken
    }

    init(from decoder: Decoder) throws {
        let c = try decoder.container(keyedBy: CodingKeys.self)
        serverHost = try c.decodeIfPresent(String.self, forKey: .serverHost) ?? Self.default.serverHost
        serverPort = try c.decodeIfPresent(UInt16.self, forKey: .serverPort) ?? Self.default.serverPort
        nickname = try c.decodeIfPresent(String.self, forKey: .nickname) ?? Self.default.nickname
        username = try c.decodeIfPresent(String.self, forKey: .username) ?? Self.default.username
        realName = try c.decodeIfPresent(String.self, forKey: .realName) ?? Self.default.realName
        channels = try c.decodeIfPresent([String].self, forKey: .channels) ?? Self.default.channels
        connectionMode = try c.decodeIfPresent(IRCConnectionMode.self, forKey: .connectionMode) ?? .direct
        zncPassword = try c.decodeIfPresent(String.self, forKey: .zncPassword) ?? ""
        zncNetwork = try c.decodeIfPresent(String.self, forKey: .zncNetwork) ?? ""
        aiEnabled = try c.decodeIfPresent(Bool.self, forKey: .aiEnabled) ?? false
        aiEndpoint = try c.decodeIfPresent(String.self, forKey: .aiEndpoint) ?? Self.default.aiEndpoint
        aiAPIKey = try c.decodeIfPresent(String.self, forKey: .aiAPIKey) ?? ""
        aiModel = try c.decodeIfPresent(String.self, forKey: .aiModel) ?? Self.default.aiModel
        memoryLimit = try c.decodeIfPresent(Int.self, forKey: .memoryLimit) ?? Self.default.memoryLimit
        autoSummarizeEvery = try c.decodeIfPresent(Int.self, forKey: .autoSummarizeEvery) ?? Self.default.autoSummarizeEvery
        notifyEnabled = try c.decodeIfPresent(Bool.self, forKey: .notifyEnabled) ?? true
        notifyMentions = try c.decodeIfPresent(Bool.self, forKey: .notifyMentions) ?? true
        notifyKeywords = try c.decodeIfPresent([String].self, forKey: .notifyKeywords) ?? Self.default.notifyKeywords
        notifyConsensus = try c.decodeIfPresent(Bool.self, forKey: .notifyConsensus) ?? true
        notifyOpenQuestions = try c.decodeIfPresent(Bool.self, forKey: .notifyOpenQuestions) ?? true
        pushRelayURL = try c.decodeIfPresent(String.self, forKey: .pushRelayURL) ?? Self.default.pushRelayURL
        pushDeviceToken = try c.decodeIfPresent(String.self, forKey: .pushDeviceToken) ?? ""
    }

    var effectiveNickname: String {
        guard connectionMode == .znc, !zncNetwork.isEmpty else { return nickname }
        return "\(nickname)/\(zncNetwork)"
    }
}

enum IRCConnectionState: Equatable, Sendable {
    case disconnected
    case connecting
    case connected
    case error(String)
}

enum HiveNotificationReason: String, Sendable {
    case mention
    case keyword
    case consensus
    case openQuestion
    case summary
}
