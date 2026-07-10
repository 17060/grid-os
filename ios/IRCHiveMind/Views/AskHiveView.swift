import SwiftUI

struct AskHiveView: View {
    @Environment(AppViewModel.self) private var viewModel

    var body: some View {
        @Bindable var viewModel = viewModel

        NavigationStack {
            VStack(alignment: .leading, spacing: 16) {
                Text("Ask the hive")
                    .font(.title2.bold())

                Text("Queries run against collective IRC memory. Enable AI in Settings for answers.")
                    .font(.subheadline)
                    .foregroundStyle(.secondary)

                TextField("What did we decide about auth?", text: $viewModel.askText, axis: .vertical)
                    .textFieldStyle(.roundedBorder)
                    .lineLimit(3...6)

                Button {
                    Task { await viewModel.askHive() }
                } label: {
                    if viewModel.isBusy {
                        ProgressView()
                    } else {
                        Text("Ask Hive")
                            .frame(maxWidth: .infinity)
                    }
                }
                .buttonStyle(.borderedProminent)
                .disabled(viewModel.isBusy || viewModel.askText.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty)

                GroupBox("Latest hive voice") {
                    ScrollView {
                        Text(viewModel.hive.summary.text)
                            .frame(maxWidth: .infinity, alignment: .leading)
                    }
                    .frame(minHeight: 180)
                }

                Spacer()
            }
            .padding()
            .navigationTitle("Ask")
        }
    }
}
