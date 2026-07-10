import SwiftUI

struct HiveView: View {
    @Environment(AppViewModel.self) private var viewModel

    var body: some View {
        NavigationStack {
            List {
                Section("Collective summary") {
                    Text(viewModel.hive.summary.text)
                        .font(.body)
                    Text("Updated \(viewModel.hive.summary.updatedAt.formatted(date: .abbreviated, time: .shortened))")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }

                Section("Open questions") {
                    if viewModel.hive.openQuestions.isEmpty {
                        Text("No unresolved questions detected.")
                            .foregroundStyle(.secondary)
                    } else {
                        ForEach(viewModel.hive.openQuestions, id: \.self) { question in
                            Text(question)
                        }
                    }
                }

                Section("Memory (\(viewModel.hive.memory.count) snippets)") {
                    ForEach(viewModel.hive.memory.reversed()) { entry in
                        VStack(alignment: .leading, spacing: 4) {
                            Text(entry.sourceChannel)
                                .font(.caption)
                                .foregroundStyle(.secondary)
                            Text(entry.snippet)
                                .font(.footnote)
                        }
                    }
                }
            }
            .navigationTitle("Hive Mind")
            .toolbar {
                ToolbarItemGroup(placement: .topBarTrailing) {
                    Button("Synthesize") {
                        Task { await viewModel.synthesize() }
                    }
                    .disabled(viewModel.isBusy)

                    Button("Clear", role: .destructive) {
                        viewModel.hive.clear()
                    }
                }
            }
        }
    }
}
