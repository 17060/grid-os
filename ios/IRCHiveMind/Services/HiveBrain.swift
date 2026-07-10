import Foundation

@MainActor
@Observable
final class HiveBrain {
    private(set) var memory: [HiveMemoryEntry] = []
    private(set) var summary = HiveSummary.empty
    private(set) var openQuestions: [String] = []

    private let storageKey = "irc.hive.memory"
    private let summaryKey = "irc.hive.summary"

    init() {
        load()
    }

    func ingest(messages: [IRCMessage], limit: Int) {
        let chat = messages.filter { !$0.isSystem && $0.sender != "hive" }
        for message in chat.suffix(20) {
            let snippet = "[\(message.channel)] <\(message.sender)> \(message.text)"
            guard !memory.contains(where: { $0.snippet == snippet }) else { continue }
            memory.append(HiveMemoryEntry(sourceChannel: message.channel, snippet: snippet))
        }
        if memory.count > limit {
            memory.removeFirst(memory.count - limit)
        }
        detectOpenQuestions(from: chat)
        persist()
    }

    func shouldAutoSummarize(messageCount: Int, every: Int) -> Bool {
        guard every > 0 else { return false }
        return messageCount > 0 && messageCount % every == 0
    }

    func applySummary(_ text: String, messageCount: Int) {
        summary = HiveSummary(text: text, updatedAt: .now, messageCount: messageCount)
        persist()
    }

    func memoryContext(maxCharacters: Int = 3000) -> String {
        memory.map(\.snippet).joined(separator: "\n").suffix(maxCharacters).description
    }

    func clear() {
        memory = []
        summary = .empty
        openQuestions = []
        persist()
    }

    private func detectOpenQuestions(from messages: [IRCMessage]) {
        let cues = messages
            .map(\.text)
            .filter { $0.contains("?") || $0.lowercased().hasPrefix("how ") || $0.lowercased().hasPrefix("what ") }
            .suffix(5)
        openQuestions = Array(cues)
    }

    private func persist() {
        let encoder = JSONEncoder()
        if let memData = try? encoder.encode(memory) {
            UserDefaults.standard.set(memData, forKey: storageKey)
        }
        if let sumData = try? encoder.encode(summary) {
            UserDefaults.standard.set(sumData, forKey: summaryKey)
        }
    }

    private func load() {
        let decoder = JSONDecoder()
        if let memData = UserDefaults.standard.data(forKey: storageKey),
           let decoded = try? decoder.decode([HiveMemoryEntry].self, from: memData) {
            memory = decoded
        }
        if let sumData = UserDefaults.standard.data(forKey: summaryKey),
           let decoded = try? decoder.decode(HiveSummary.self, from: sumData) {
            summary = decoded
        }
    }
}
