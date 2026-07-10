import SwiftUI

@main
struct IRCHiveMindApp: App {
    @UIApplicationDelegateAdaptor(AppDelegate.self) private var appDelegate
    @State private var viewModel = AppViewModel()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environment(viewModel)
                .task { await viewModel.bootstrap() }
                .onAppear {
                    appDelegate.onDeviceToken = { token in
                        viewModel.registerPushToken(token)
                    }
                }
        }
    }
}
