#include "FCFSScheduler.h"

#include <iostream>
#include <mutex>
#include <vector>
#include <chrono>

using namespace std::chrono;

FCFSScheduler::FCFSScheduler(int queue_capacity)
    : ready_queue_(queue_capacity),
      current_sim_time_(0ns),
      simulation_active_(false)
{
    std::cout << "FCFSScheduler task queue initialized with capacity: " << queue_capacity << std::endl;
}

void FCFSScheduler::add_process(Process* p){
    if(!p){
        std::cerr << "ERROR: Attempted to add null process to task queue";
    }

    std::lock_guard<std::mutex> lock(scheduler_mutex_);
    all_processes_.push_back(p);
    std::cout << "Process " << p->pid << " added to all_processes_ list (arrival at: "
            << p->arrival_time.count() << "ns)" << std::endl;
}

Process* FCFSScheduler::get_next_process(){
    std::lock_guard<std::mutex> lock(scheduler_mutex_);
    return ready_queue_.dequeue();
}

void FCFSScheduler::update_stats_for_completed_processes() {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);

        for(Process* p : all_processes_){
            // check that the process actually ran
            if(p->current_state.load() == Process::State::COMPLETED && p->start_time.load().count() != -1){
                

                stats_.total_processes_completed++;

                stats_.add_process_stats(p);

                std::cout   << "Stats for Process" << p->pid 
                            << ": Turnaround=" << p->get_turnaround_time() << "ns, Waiting= " 
                            << p->get_waiting_time() << "ns." << std::endl;
            }
        }
    }

void FCFSScheduler::run_simulation(){
    simulation_active_ = true;
    std::cout << "Starting FCFS Scheduler Simulation" << std::endl;

    while(simulation_active_ || !all_processes_finished()){
        std::cout << "Current sim time: " << current_sim_time_.count() << "ns" << std::endl;

        // first check if any new processes have arrived
        handle_new_arrivals();

        Process* curr_process = nullptr;
        while((curr_process = get_next_process()) != nullptr){
            // dispact process
            std::cout   << " Dispatching Process " << curr_process->pid << " (Remaining : " 
                        << curr_process->remaining_time.load().count() << "ns)" << std::endl;
            dispatch_process(curr_process);

            // update the stats for any completed processes
            update_stats_for_completed_processes();


            // stop early if we're done
            if (all_processes_finished()){
                simulation_active_ = false;
                break;
            }
        }

        // if we don't have any tasks ready but not all processes are done, we need to advance the time to catch up to the next process
        if(ready_queue_.empty() && !all_processes_finished()) {
            std::cout << " Task queue is empty. Skipping to next available process." << std::endl;
            std::chrono::nanoseconds next_arrival = std::chrono::nanoseconds::max();

            // lock to protect all processes
            std::lock_guard<std::mutex> lock(scheduler_mutex_);
            for (const Process* p : all_processes_){
                if (p->arrival_time > current_sim_time_ && p->arrival_time < next_arrival){
                    next_arrival = p->arrival_time;
                }
            }

            // we found a next time we can use
            if(next_arrival != std::chrono::nanoseconds::max()){
                current_sim_time_ = next_arrival;
            } else {
                // something's wrong so let's advance the current time
                current_sim_time_ += 1ns;
            }
        }

        // we're done
        if(ready_queue_.empty() && all_processes_finished()){
            simulation_active_ = false;
            break;
        }

        std::cout << "FCFSScheduler Simulation Finished" << std::endl;
        update_stats_for_completed_processes();
    }
}

SchedulerStats FCFSScheduler::get_stats() const {
    std::lock_guard<std::mutex> lock(scheduler_mutex_);
    return stats_;
}

bool FCFSScheduler::is_simulation_complete() {
    std::lock_guard<std::mutex> lock(scheduler_mutex_);
    if(!ready_queue_.empty()){
        return false;
    }

    return all_processes_finished();
}

bool FCFSScheduler::all_processes_finished(){
    for(Process* p : all_processes_){
        if(p->current_state != Process::State::COMPLETED){
            return false;
        }
    }

    return true;
}

void FCFSScheduler::dispatch_process(Process* p){
    if (!p){
        std::cerr << "ERROR: Attempted to dispatch null process" << std::endl;
        return;
    }

    if(p->current_state == Process::State::READY){
        p->start_time = current_sim_time_;
        p->current_state = Process::State::RUNNING;
        std::cout << "Process  " << p->pid << " starting running at " << current_sim_time_.count() << "ns." << std::endl;
    }

    // simulate running the process until it's done
    p->execute_slice(current_sim_time_);
    p->current_state = Process::State::COMPLETED;

    std::cout << " Process: " << p->pid << " COMPLETED at " << p->completion_time.load().count() << "ns." << std::endl;

    p->last_run_timestamp = current_sim_time_;
}

void FCFSScheduler::handle_new_arrivals() {
    std::lock_guard<std::mutex> lock(scheduler_mutex_);

    for (Process* p : all_processes_){
        // std::cout << p->arrival_time << " " << current_sim_time_ << std::endl;
        if(p->arrival_time <= current_sim_time_){
            if(ready_queue_.enqueue(p));
            std::cout   << "Process " << p->pid << " arrived and added to task queue at " 
                        << current_sim_time_.count() << "ns." << std::endl;
        } else {
            std::cerr   << "WARNING: FCFS task queue full, dropping proces " << p->pid 
                        << " (arrived at " << p->arrival_time.count() << "ns, sim_time: " 
                        << current_sim_time_.count() << "ns)" << std::endl;
        }
    }
}