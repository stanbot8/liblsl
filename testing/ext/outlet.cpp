#include <catch2/catch_all.hpp>
#include <chrono>
#include <lsl_cpp.h>
#include <memory>
#include <thread>

namespace {

bool wait_for_no_consumers(lsl::stream_outlet &outlet) {
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
	while (outlet.have_consumers() && std::chrono::steady_clock::now() < deadline)
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	return !outlet.have_consumers();
}

TEST_CASE("outlet tracks idle consumer disconnects", "[outlet][disconnect]") {
	const auto transport = GENERATE(transp_default, transp_sync_blocking);
	CAPTURE(transport);
	lsl::stream_info info("IdleDisconnect", "Test", 1, 100, lsl::cf_float32, "idle_disconnect");
	lsl::stream_outlet outlet(info, 0, 360, transport);
	auto resolved = lsl::resolve_stream("name", info.name(), 1, 5.0);
	REQUIRE(resolved.size() == 1);

	auto connect = [&] {
		auto inlet = std::make_unique<lsl::stream_inlet>(resolved.front());
		inlet->open_stream(2);
		REQUIRE(outlet.wait_for_consumers(2));
		return inlet;
	};

	auto inlet1 = connect();
	auto inlet2 = connect();
	inlet1.reset();
	CHECK(outlet.have_consumers());
	inlet2.reset();
	REQUIRE(wait_for_no_consumers(outlet));

	auto inlet3 = connect();
	CHECK(outlet.have_consumers());
	inlet3.reset();
	CHECK(wait_for_no_consumers(outlet));
}

} // namespace
