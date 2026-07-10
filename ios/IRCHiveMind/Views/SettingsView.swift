import SwiftUI

struct SettingsView: View {
    @Environment(AppViewModel.self) private var viewModel

    var body: some View {
        @Bindable var viewModel = viewModel

        NavigationStack {
            Form {
                Section("IRC server") {
                    TextField("Host", text: $viewModel.settings.serverHost)
                        .textInputAutocapitalization(.never)
                        .autocorrectionDisabled()
                    TextField("Port", value: $viewModel.settings.serverPort, format: .number)
                    TextField("Nickname", text: $viewModel.settings.nickname)
                    TextField("Channels (comma-separated)", text: channelsBinding)
                }

                Section("Hive memory") {
                    Stepper("Memory limit: \(viewModel.settings.memoryLimit)", value: $viewModel.settings.memoryLimit, in: 50...500, step: 25)
                    Stepper("Auto-summarize every \(viewModel.settings.autoSummarizeEvery) messages", value: $viewModel.settings.autoSummarizeEvery, in: 0...50)
                    Text("Set auto-summarize to 0 to disable.")
                        .font(.caption)
                        .foregroundStyle(.secondary)
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
}
