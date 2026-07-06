#include <catch2/catch_test_macros.hpp>
#include "core/jobs.hpp"
#include <atomic>
#include <vector>

using namespace TheSeed::Jobs;

TEST_CASE("Jobs: Initialize and Shutdown", "[jobs]") {
    JobSystem js;
    REQUIRE(!js.IsRunning());

    js.Initialize(4);
    REQUIRE(js.IsRunning());
    REQUIRE(js.WorkerCount() == 4);

    js.Shutdown();
    REQUIRE(!js.IsRunning());
}

TEST_CASE("Jobs: 1M Tasks no deadlock", "[jobs]") {
    JobSystem js;
    js.Initialize(4);

    std::atomic<int> counter{0};
    for (int i = 0; i < 1'000'000; ++i) {
        js.Submit([&counter]() {
            counter.fetch_add(1);
        });
    }

    js.WaitForAll();
    REQUIRE(counter.load() == 1'000'000);

    js.Shutdown();
}

TEST_CASE("Jobs: Parallel Sum correctness", "[jobs]") {
    JobSystem js;
    js.Initialize(4);

    std::vector<int> values(100'000, 1);
    std::atomic<int> sum{0};

    for (size_t i = 0; i < values.size(); ++i) {
        js.Submit([&sum, &values, i]() {
            sum.fetch_add(values[i]);
        });
    }

    js.WaitForAll();
    REQUIRE(sum.load() == 100'000);

    js.Shutdown();
}

TEST_CASE("Jobs: Nested Tasks", "[jobs]") {
    JobSystem js;
    js.Initialize(4);

    std::atomic<int> counter{0};

    js.Submit([&]() {
        for (int i = 0; i < 100; ++i) {
            js.Submit([&counter]() {
                counter.fetch_add(1);
            });
        }
    });

    js.WaitForAll();
    REQUIRE(counter.load() == 100);

    js.Shutdown();
}

TEST_CASE("Jobs: SubmitAndWait", "[jobs]") {
    JobSystem js;
    js.Initialize(2);

    std::atomic<int> value{0};
    js.SubmitAndWait([&value]() {
        value.store(42);
    });

    REQUIRE(value.load() == 42);
    js.Shutdown();
}

TEST_CASE("Jobs: Fallback when not initialized", "[jobs]") {
    JobSystem js;
    // Nicht initialisiert

    bool ran = false;
    js.Submit([&ran]() { ran = true; });

    REQUIRE(ran); // Synchron ausgeführt
}
