import Foundation

struct AIService {
    enum AIError: LocalizedError {
        case disabled
        case missingKey
        case badResponse

        var errorDescription: String? {
            switch self {
            case .disabled: return "AI synthesis is disabled in Settings."
            case .missingKey: return "Add an API key in Settings to use hive AI."
            case .badResponse: return "The AI service returned an unexpected response."
            }
        }
    }

    func summarize(memory: String, settings: AppSettings) async throws -> String {
        guard settings.aiEnabled else { throw AIError.disabled }
        guard !settings.aiAPIKey.isEmpty else { throw AIError.missingKey }

        let prompt = """
        You are the prefrontal cortex of an IRC hive mind. Summarize the collective \
        channel memory in 3 short bullet points. Note any open questions or emerging consensus.

        Memory:
        \(memory)
        """

        return try await chat(prompt: prompt, settings: settings)
    }

    func answer(question: String, memory: String, settings: AppSettings) async throws -> String {
        guard settings.aiEnabled else { throw AIError.disabled }
        guard !settings.aiAPIKey.isEmpty else { throw AIError.missingKey }

        let prompt = """
        You answer on behalf of an IRC hive mind. Use only the memory below. \
        If the memory lacks evidence, say what the hive does not know yet.

        Question: \(question)

        Memory:
        \(memory)
        """

        return try await chat(prompt: prompt, settings: settings)
    }

    private func chat(prompt: String, settings: AppSettings) async throws -> String {
        guard let url = URL(string: settings.aiEndpoint) else { throw AIError.badResponse }

        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        request.setValue("Bearer \(settings.aiAPIKey)", forHTTPHeaderField: "Authorization")

        let body: [String: Any] = [
            "model": settings.aiModel,
            "messages": [
                ["role": "system", "content": "You are IRC Hive Mind, concise and factual."],
                ["role": "user", "content": prompt]
            ],
            "temperature": 0.3
        ]
        request.httpBody = try JSONSerialization.data(withJSONObject: body)

        let (data, response) = try await URLSession.shared.data(for: request)
        guard let http = response as? HTTPURLResponse, (200...299).contains(http.statusCode) else {
            throw AIError.badResponse
        }

        guard
            let json = try JSONSerialization.jsonObject(with: data) as? [String: Any],
            let choices = json["choices"] as? [[String: Any]],
            let first = choices.first,
            let message = first["message"] as? [String: Any],
            let content = message["content"] as? String
        else {
            throw AIError.badResponse
        }

        return content.trimmingCharacters(in: .whitespacesAndNewlines)
    }
}
