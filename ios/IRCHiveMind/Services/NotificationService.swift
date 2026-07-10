import Foundation
import UserNotifications

@MainActor
final class NotificationService {
    static let shared = NotificationService()

    private init() {}

    func requestAuthorization() async -> Bool {
        let center = UNUserNotificationCenter.current()
        do {
            return try await center.requestAuthorization(options: [.alert, .sound, .badge])
        } catch {
            return false
        }
    }

    func notify(
        title: String,
        body: String,
        reason: HiveNotificationReason,
        settings: AppSettings
    ) {
        guard settings.notifyEnabled else { return }

        let content = UNMutableNotificationContent()
        content.title = title
        content.body = body
        content.sound = .default
        content.categoryIdentifier = reason.rawValue

        let trigger = UNTimeIntervalNotificationTrigger(timeInterval: 0.5, repeats: false)
        let request = UNNotificationRequest(
            identifier: UUID().uuidString,
            content: content,
            trigger: trigger
        )
        UNUserNotificationCenter.current().add(request)

        Task {
            await PushRelayClient.shared.sendRemoteIfConfigured(
                title: title,
                body: body,
                settings: settings
            )
        }
    }

    func evaluateMessage(_ message: IRCMessage, settings: AppSettings) {
        guard settings.notifyEnabled, !message.isSystem else { return }
        let lower = message.text.lowercased()
        let nick = settings.nickname.lowercased()

        if settings.notifyMentions,
           lower.contains(nick) || message.text.contains(settings.nickname) {
            notify(
                title: "Hive mention in \(message.channel)",
                body: "\(message.sender): \(message.text)",
                reason: .mention,
                settings: settings
            )
            return
        }

        for keyword in settings.notifyKeywords where !keyword.isEmpty {
            if lower.contains(keyword.lowercased()) {
                notify(
                    title: "Hive keyword: \(keyword)",
                    body: "\(message.channel) — \(message.sender): \(message.text)",
                    reason: .keyword,
                    settings: settings
                )
                return
            }
        }
    }

    func notifyConsensus(summary: String, settings: AppSettings) {
        guard settings.notifyEnabled, settings.notifyConsensus else { return }
        notify(
            title: "Hive consensus",
            body: String(summary.prefix(180)),
            reason: .consensus,
            settings: settings
        )
    }

    func notifyOpenQuestion(_ question: String, settings: AppSettings) {
        guard settings.notifyEnabled, settings.notifyOpenQuestions else { return }
        notify(
            title: "Open hive question",
            body: question,
            reason: .openQuestion,
            settings: settings
        )
    }

    func notifySummary(_ summary: String, settings: AppSettings) {
        guard settings.notifyEnabled else { return }
        notify(
            title: "Hive synthesized",
            body: String(summary.prefix(180)),
            reason: .summary,
            settings: settings
        )
    }
}
