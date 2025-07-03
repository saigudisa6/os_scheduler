#ifndef SJF_SCHEDULER_H
#define SJF_SCHEDULER_H

#include <chrono>
#include <iostream>
#include <mutex>
#include <vector>
#include <queue>    
#include <atomic>   
#include <set>      

#include "Process.h" 
#include "SchedulerStats.h"

using namespace std::chrono;

// sort based on remaining time
struct ProcessComparator {
    bool operator()(const Process* a, const Process* b) const {
        return a->remaining_time.load() > b->remaining_time.load();
    }
};

class SJFScheduler{
public:
    explicit SJFScheduler(int queue_capacity);

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
    // ready priority queue for tasks - now uses the custom comparator
    std::priority_queue<Process*, std::vector<Process*>, ProcessComparator> ready_queue_;

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

    // Member to store the maximum capacity of the ready queue
    const int queue_capacity_;

    // NEW: Set to keep track of PIDs for which stats have already been updated
    std::set<int> pids_with_updated_stats_;

    // let process run
    void dispatch_process(Process* p);

    // retrieve all the processes who would have arrived at current simulation time
    void handle_new_arrivals();

    bool all_processes_finished();

    void update_stats_for_completed_processes();
};

#endif 