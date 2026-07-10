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
    var isBackgrounded = false

    let irc = IRCClient()
    let hive = HiveBrain()
    private let ai = AIService()
    private let notifications = NotificationService.shared
    private var lastSummaryText = HiveSummary.empty.text
    private var seenOpenQuestions = Set<String>()

    init() {
        if let data = UserDefaults.standard.data(forKey: "irc.hive.settings"),
           let decoded = try? JSONDecoder().decode(AppSettings.self, from: data) {
            settings = decoded
        }
        selectedChannel = settings.channels.first ?? "#grid"
        irc.onMessage = { [weak self] message in
            Task { @MainActor in
                self?.handleIncomingMessage(message)
            }
        }
    }

    func bootstrap() async {
        let granted = await notifications.requestAuthorization()
        statusMessage = granted ? "Notifications enabled." : "Notifications denied — local alerts off."
    }

    func registerPushToken(_ token: String) {
        settings.pushDeviceToken = token
        saveSettings()
        Task {
            await PushRelayClient.shared.registerDevice(token: token, settings: settings)
            statusMessage = "Push token registered with relay."
        }
    }

    func saveSettings() {
        if let data = try? JSONEncoder().encode(settings) {
            UserDefaults.standard.set(data, forKey: "irc.hive.settings")
        }
        if !settings.channels.contains(selectedChannel) {
            selectedChannel = settings.channels.first ?? selectedChannel
        }
    }

    func connect() {
        saveSettings()
        irc.connect(settings: settings)
        let mode = settings.connectionMode == .znc ? "ZNC" : "IRC"
        statusMessage = "Connecting \(mode) → \(settings.serverHost):\(settings.serverPort)…"
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

    func handleIncomingMessage(_ message: IRCMessage) {
        notifications.evaluateMessage(message, settings: settings)
        syncHive()
    }

    func syncHive() {
        let previousQuestions = Set(hive.openQuestions)
        hive.ingest(messages: irc.messages, limit: settings.memoryLimit)

        for question in hive.openQuestions where !seenOpenQuestions.contains(question) {
            seenOpenQuestions.insert(question)
            if previousQuestions.isEmpty || !previousQuestions.contains(question) {
                notifications.notifyOpenQuestion(question, settings: settings)
            }
        }

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
            if text != lastSummaryText {
                lastSummaryText = text
                notifications.notifySummary(text, settings: settings)
                if settings.notifyConsensus {
                    notifications.notifyConsensus(summary: text, settings: settings)
                }
            }
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
            notifications.notifySummary(answer, settings: settings)
            statusMessage = "Hive answered."
        } catch {
            statusMessage = error.localizedDescription
        }
    }

    func setBackgrounded(_ backgrounded: Bool) {
        isBackgrounded = backgrounded
        if backgrounded, settings.notifyEnabled {
            notifications.notify(
                title: "IRC Hive Mind",
                body: "Backgrounded — local notifications remain active while connected.",
                reason: .summary,
                settings: settings
            )
        }
    }
}
