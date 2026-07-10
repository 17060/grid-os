import SwiftUI

struct ChannelsView: View {
    @Environment(AppViewModel.self) private var viewModel

    var body: some View {
        @Bindable var viewModel = viewModel

        NavigationStack {
            VStack(spacing: 0) {
                connectionBar

                List(filteredMessages) { message in
                    VStack(alignment: .leading, spacing: 4) {
                        HStack {
                            Text(message.sender)
                                .font(.headline)
                            Spacer()
                            Text(message.channel)
                                .font(.caption)
                                .foregroundStyle(.secondary)
                        }
                        Text(message.text)
                            .font(.body)
                    }
                    .padding(.vertical, 2)
                }
                .listStyle(.plain)

                composer
            }
            .navigationTitle("IRC Hive")
        }
    }

    private var filteredMessages: [IRCMessage] {
        viewModel.irc.messages.filter { message in
            message.isSystem || message.channel == viewModel.selectedChannel
        }
    }

    private var connectionBar: some View {
        HStack {
            Picker("Channel", selection: $viewModel.selectedChannel) {
                ForEach(viewModel.settings.channels, id: \.self) { channel in
                    Text(channel).tag(channel)
                }
            }
            .pickerStyle(.menu)

            Spacer()

            switch viewModel.irc.state {
            case .connected:
                Button("Disconnect") { viewModel.disconnect() }
            default:
                Button("Connect") { viewModel.connect() }
            }
        }
        .padding(.horizontal)
        .padding(.vertical, 8)
        .background(.bar)
    }

    private var composer: some View {
        HStack {
            TextField("Message #\(viewModel.selectedChannel)", text: $viewModel.composerText)
                .textFieldStyle(.roundedBorder)
            Button("Send") { viewModel.sendMessage() }
                .disabled(viewModel.irc.state != .connected)
        }
        .padding()
    }
}
