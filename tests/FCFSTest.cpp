#include <gtest/gtest.h>
#include "../src/FCFSScheduler.h"  // Your actual header

class FCFSSchedulerTest : public ::testing::Test {
protected:
    void SetUp() override {
        scheduler = std::make_unique<FCFSScheduler>(10);
    }

    std::unique_ptr<FCFSScheduler> scheduler;
    std::vector<std::unique_ptr<Process>> processes;
};

TEST_F(FCFSSchedulerTest, HandleSingleProcess){
    auto p = std::make_unique<Process>(1, 0ns, 100ns);
    p->arrival_time = 0ns;

    scheduler->add_process(p.get());
    processes.push_back(std::move(p));

    scheduler->run_simulation();

    auto stats = scheduler->get_stats();

    EXPECT_EQ(stats.total_processes_completed, 1);
    EXPECT_EQ(stats.turnaround_times[0].count(), 100);
    EXPECT_EQ(stats.waiting_times[0].count(), 0);
    EXPECT_TRUE(scheduler->is_simulation_complete());
}