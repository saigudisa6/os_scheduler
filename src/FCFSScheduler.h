#ifndef FCFS_SCHEDULER_H
#define FCFS_SCHEDULER_H


#include <chrono>
#include <iostream>
#include <mutex>

#include "Process.h"
#include "CircularBuffer.h"
#include "SchedulerStats.h"

using namespace std::chrono;

class FCFSScheduler{
public:
    // Constructor
    explicit FCFSScheduler(int queue_capacity_);

    // add a process to the "ready" queue
    void add_process(Process* p);

    // get the next ready process
    Process* get_next_process();

    // begin the simulation
    void run_simulation();

    // record current performance metrics
    SchedulerStats get_stats() const;

    // helper
    bool is_simulation_complete();
private:
    // ready queue for tasks
    CircularBuffer ready_queue_;

    // mutex to make the task queue thread-safe
    mutable std::mutex scheduler_mutex_;

    // track current time in the simulation
    std::chrono::nanoseconds current_sim_time_ = 0ns;

    // just to keep track of all stats after simulation
    std::vector<Process*> all_processes_;

    // object to store metrics in
    SchedulerStats stats_;

    // is simulation still running or has it completed
    std::atomic<bool> simulation_active_ = false;

    // let process run
    void dispatch_process(Process* p);

    // retrieve all the processes who would have arrived at current simulation time
    void handle_new_arrivals();

    bool all_processes_finished();

    void update_stats_for_completed_processes();
};

#endif