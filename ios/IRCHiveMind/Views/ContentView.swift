import SwiftUI

struct ContentView: View {
    @Environment(AppViewModel.self) private var viewModel
    @Environment(\.scenePhase) private var scenePhase

    var body: some View {
        @Bindable var viewModel = viewModel

        TabView {
            ChannelsView()
                .tabItem { Label("Channels", systemImage: "bubble.left.and.bubble.right") }

            HiveView()
                .tabItem { Label("Hive", systemImage: "brain.head.profile") }

            AskHiveView()
                .tabItem { Label("Ask", systemImage: "questionmark.circle") }

            SettingsView()
                .tabItem { Label("Settings", systemImage: "gearshape") }
        }
        .onChange(of: viewModel.irc.messages.count) { _, _ in
            viewModel.syncHive()
        }
        .onChange(of: scenePhase) { _, phase in
            viewModel.setBackgrounded(phase != .active)
        }
    }
}
