import SwiftUI

struct SettingsView: View {
    @Environment(AppViewModel.self) private var viewModel

    var body: some View {
        @Bindable var viewModel = viewModel

        NavigationStack {
            Form {
                Section("Connection mode") {
                    Picker("Mode", selection: $viewModel.settings.connectionMode) {
                        ForEach(IRCConnectionMode.allCases, id: \.self) { mode in
                            Text(mode.label).tag(mode)
                        }
                    }
                    .pickerStyle(.segmented)

                    if viewModel.settings.connectionMode == .znc {
                        SecureField("ZNC password", text: $viewModel.settings.zncPassword)
                        TextField("ZNC network (nick/network)", text: $viewModel.settings.zncNetwork)
                            .textInputAutocapitalization(.never)
                            .autocorrectionDisabled()
                        Text("Connect to your ZNC host. Password is sent as PASS before NICK/USER.")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    } else {
                        Text("Direct IRC — use with make irc-bridge or any public network.")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }
                }

                Section("IRC server") {
                    TextField("Host", text: $viewModel.settings.serverHost)
                        .textInputAutocapitalization(.never)
                        .autocorrectionDisabled()
                    TextField("Port", value: $viewModel.settings.serverPort, format: .number)
                    TextField("Nickname", text: $viewModel.settings.nickname)
                    TextField("Channels (comma-separated)", text: channelsBinding)
                }

                Section("Notifications") {
                    Toggle("Enable notifications", isOn: $viewModel.settings.notifyEnabled)
                    Toggle("Mentions", isOn: $viewModel.settings.notifyMentions)
                    Toggle("Consensus summaries", isOn: $viewModel.settings.notifyConsensus)
                    Toggle("Open questions", isOn: $viewModel.settings.notifyOpenQuestions)
                    TextField("Keywords (comma-separated)", text: keywordsBinding)
                    Text("Local alerts fire for keywords, @mentions, hive synthesis, and open questions.")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }

                Section("Push relay (optional)") {
                    TextField("Relay URL", text: $viewModel.settings.pushRelayURL)
                        .textInputAutocapitalization(.never)
                        .autocorrectionDisabled()
                    if !viewModel.settings.pushDeviceToken.isEmpty {
                        Text("Device token: \(viewModel.settings.pushDeviceToken.prefix(16))…")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    } else {
                        Text("Run on device for APNs token; relay via make hive-push-relay.")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }
                }

                Section("Hive memory") {
                    Stepper("Memory limit: \(viewModel.settings.memoryLimit)", value: $viewModel.settings.memoryLimit, in: 50...500, step: 25)
                    Stepper("Auto-summarize every \(viewModel.settings.autoSummarizeEvery) messages", value: $viewModel.settings.autoSummarizeEvery, in: 0...50)
                }

                Section("AI synthesis") {
                    Toggle("Enable AI", isOn: $viewModel.settings.aiEnabled)
                    SecureField("API key", text: $viewModel.settings.aiAPIKey)
                    TextField("Endpoint", text: $viewModel.settings.aiEndpoint)
                        .textInputAutocapitalization(.never)
                        .autocorrectionDisabled()
                    TextField("Model", text: $viewModel.settings.aiModel)
                }

                Section {
                    Text(viewModel.statusMessage)
                        .font(.footnote)
                        .foregroundStyle(.secondary)
                    Button("Save settings") {
                        viewModel.saveSettings()
                        viewModel.statusMessage = "Settings saved."
                    }
                }
            }
            .navigationTitle("Settings")
        }
    }

    private var channelsBinding: Binding<String> {
        Binding(
            get: { viewModel.settings.channels.joined(separator: ", ") },
            set: { raw in
                viewModel.settings.channels = raw
                    .split(separator: ",")
                    .map { $0.trimmingCharacters(in: .whitespacesAndNewlines) }
                    .filter { !$0.isEmpty }
            }
        )
    }

    private var keywordsBinding: Binding<String> {
        Binding(
            get: { viewModel.settings.notifyKeywords.joined(separator: ", ") },
            set: { raw in
                viewModel.settings.notifyKeywords = raw
                    .split(separator: ",")
                    .map { $0.trimmingCharacters(in: .whitespacesAndNewlines) }
                    .filter { !$0.isEmpty }
            }
        )
    }
}
