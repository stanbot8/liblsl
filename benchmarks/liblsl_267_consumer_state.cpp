#include <lsl_cpp.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

double run_trial(lsl_transport_options_t transport, int trial) {
	constexpr int channels = 8;
	constexpr int samples_per_chunk = 32;
	constexpr int sample_count = 1048576;
	const std::string suffix =
		std::to_string(static_cast<int>(transport)) + "_" + std::to_string(trial);
	const std::string name = "AlphaBridgeBenchmark_" + suffix;
	lsl::stream_info info(name, "Benchmark", channels, 1000, lsl::cf_float32, name);
	lsl::stream_outlet outlet(info, samples_per_chunk, 2048, transport);
	auto resolved = lsl::resolve_stream("name", name, 1, 5.0);
	if (resolved.size() != 1) throw std::runtime_error("The stream was not resolved.");

	lsl::stream_inlet inlet(resolved.front(), 2048, 0, true);
	inlet.open_stream(2);
	if (!outlet.wait_for_consumers(2)) throw std::runtime_error("The consumer did not connect.");

	std::size_t received = 0;
	std::thread reader([&] {
		std::vector<float> buffer(channels * samples_per_chunk * 16);
		while (received < static_cast<std::size_t>(sample_count)) {
			const auto elements =
				inlet.pull_chunk_multiplexed(buffer.data(), nullptr, buffer.size(), 0, 5.0);
			if (elements == 0) throw std::runtime_error("The sample pull timed out.");
			received += elements / channels;
		}
	});

	std::vector<float> chunk(channels * samples_per_chunk, 1.0F);
	const auto started = std::chrono::steady_clock::now();
	for (int sent = 0; sent < sample_count; sent += samples_per_chunk)
		outlet.push_chunk_multiplexed(chunk);
	reader.join();
	const auto finished = std::chrono::steady_clock::now();
	if (received != static_cast<std::size_t>(sample_count))
		throw std::runtime_error("The sample count was not exact.");
	return std::chrono::duration<double>(finished - started).count();
}

} // namespace

int main(int argc, char **argv) {
	if (argc != 2) return 2;
	const auto transport =
		std::string(argv[1]) == "sync" ? transp_sync_blocking : transp_default;
	std::vector<double> durations;
	for (int trial = 0; trial < 3; ++trial) durations.push_back(run_trial(transport, trial));
	std::sort(durations.begin(), durations.end());
	for (std::size_t index = 0; index < durations.size(); ++index) {
		if (index) std::cout << ',';
		std::cout << durations[index];
	}
	std::cout << '\n';
}
