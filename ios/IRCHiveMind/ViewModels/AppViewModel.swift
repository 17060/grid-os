import Foundation
import SwiftUI

@MainActor
@Observable
final class AppViewModel {
    var settings = AppSettings.default
    var composerText = ""
    var selectedChannel = ""
    var askText = ""
    var statusMessage = "Disconnected"
    var isBusy = false

    let irc = IRCClient()
    let hive = HiveBrain()
    private let ai = AIService()

    init() {
        if let data = UserDefaults.standard.data(forKey: "irc.hive.settings"),
           let decoded = try? JSONDecoder().decode(AppSettings.self, from: data) {
            settings = decoded
        }
        selectedChannel = settings.channels.first ?? "#testhive"
    }

    func saveSettings() {
        if let data = try? JSONEncoder().encode(settings) {
            UserDefaults.standard.set(data, forKey: "irc.hive.settings")
        }
        if settings.channels.contains(selectedChannel) == false {
            selectedChannel = settings.channels.first ?? selectedChannel
        }
    }

    func connect() {
        saveSettings()
        irc.connect(settings: settings)
        statusMessage = "Connecting to \(settings.serverHost):\(settings.serverPort)…"
    }

    func disconnect() {
        irc.disconnect()
        statusMessage = "Disconnected"
    }

    func sendMessage() {
        let text = composerText.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !text.isEmpty else { return }
        irc.say(selectedChannel, text)
        composerText = ""
        syncHive()
    }

    func syncHive() {
        hive.ingest(messages: irc.messages, limit: settings.memoryLimit)
        if hive.shouldAutoSummarize(messageCount: irc.messages.count, every: settings.autoSummarizeEvery) {
            Task { await synthesize() }
        }
    }

    func synthesize() async {
        isBusy = true
        defer { isBusy = false }
        do {
            let text = try await ai.summarize(memory: hive.memoryContext(), settings: settings)
            hive.applySummary(text, messageCount: irc.messages.count)
            statusMessage = "Hive summary updated."
        } catch {
            statusMessage = error.localizedDescription
        }
    }

    func askHive() async {
        let question = askText.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !question.isEmpty else { return }
        isBusy = true
        defer { isBusy = false }
        do {
            let answer = try await ai.answer(question: question, memory: hive.memoryContext(), settings: settings)
            hive.applySummary("Q: \(question)\n\n\(answer)", messageCount: irc.messages.count)
            askText = ""
            statusMessage = "Hive answered."
        } catch {
            statusMessage = error.localizedDescription
        }
    }
}
