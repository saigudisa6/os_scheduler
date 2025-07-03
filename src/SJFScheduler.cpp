#include "SJFScheduler.h" 
#include <algorithm> 
#include <limits>    

SJFScheduler::SJFScheduler(int capacity)
    : queue_capacity_(capacity),
      current_sim_time_(0ns),
      simulation_active_(false)
{
    std::cout << "SJFScheduler initialized with ready queue capacity: " << queue_capacity_ << std::endl;
}

void SJFScheduler::add_process(Process* p){
    if(!p){
        std::cerr << "ERROR: Attempted to add null process to all_processes_ list" << std::endl;
        return;
    }

    std::lock_guard<std::mutex> lock(scheduler_mutex_);
    all_processes_.push_back(p);
    std::cout << "Process " << p->pid << " added to all_processes_ list (arrival at: "
              << p->arrival_time.count() << "ns)" << std::endl;
}

Process* SJFScheduler::get_next_process(){
    std::lock_guard<std::mutex> lock(scheduler_mutex_);
    if (ready_queue_.empty()){
        return nullptr;
    }
    // get and remove next process
    Process* next_p = ready_queue_.top(); 
    ready_queue_.pop();                   
    return next_p;
}

void SJFScheduler::update_stats_for_completed_processes() {
    std::lock_guard<std::mutex> lock(scheduler_mutex_);

    for(Process* p : all_processes_){
        if(p->current_state.load() == Process::State::COMPLETED &&
           p->start_time.load().count() != 0 && 
           pids_with_updated_stats_.find(p->pid) == pids_with_updated_stats_.end()){ 
            
            stats_.total_processes_completed++;
            stats_.add_process_stats(p);
            pids_with_updated_stats_.insert(p->pid); 

            std::cout << "Stats for Process " << p->pid
                      << ": Turnaround=" << p->get_turnaround_time().count() << "ns, Waiting= "
                      << p->get_waiting_time().count() << "ns." << std::endl;
        }
    }
}

void SJFScheduler::run_simulation(){
    simulation_active_ = true;
    std::cout << "Starting SJF Scheduler Simulation" << std::endl;

    while(simulation_active_ || !all_processes_finished()){
        std::cout << "\n-------------------------------------" << std::endl;
        std::cout << "Current sim time: " << current_sim_time_.count() << "ns" << std::endl;

        // handle new arrivals at the current simulation time
        handle_new_arrivals();

        Process* curr_process = nullptr;
        // get and dispatch the next shortest job if available
        if((curr_process = get_next_process()) != nullptr){
            std::cout << " Dispatching Process " << curr_process->pid << " (Remaining : "
                      << curr_process->remaining_time.load().count() << "ns)" << std::endl;
            dispatch_process(curr_process); 

            update_stats_for_completed_processes();
        } else {
            // advance simulation time to the next arrival
            if (!all_processes_finished()) {
                std::cout << " Ready queue is empty. Skipping to next available process arrival." << std::endl;
                std::chrono::nanoseconds next_arrival = std::chrono::nanoseconds::max();

                std::lock_guard<std::mutex> lock(scheduler_mutex_);
                for (const Process* p : all_processes_){
                    if (p->current_state.load() != Process::State::COMPLETED &&
                        p->start_time.load().count() == 0 && // Check if it hasn't started yet
                        p->arrival_time > current_sim_time_ &&
                        p->arrival_time < next_arrival){
                        next_arrival = p->arrival_time;
                    }
                }

                if(next_arrival != std::chrono::nanoseconds::max()){
                    current_sim_time_ = next_arrival;
                } else {
                    current_sim_time_ += 1ns;
                    std::cerr << "WARNING: No future arrivals found and ready queue empty, advancing time by 1ns." << std::endl;
                }
            }
        }

        // Check if simulation is complete
        if(is_simulation_complete()){
            simulation_active_ = false;
        }
    }

    std::cout << "\nSJFScheduler Simulation Finished" << std::endl;
    // Final update of stats to ensure all completed processes are accounted for
    update_stats_for_completed_processes();
}

SchedulerStats SJFScheduler::get_stats() const {
    std::lock_guard<std::mutex> lock(scheduler_mutex_);
    return stats_;
}

bool SJFScheduler::is_simulation_complete() {
    std::lock_guard<std::mutex> lock(scheduler_mutex_);
    // Simulation is complete if ready queue is empty AND all processes have finished.
    return ready_queue_.empty() && all_processes_finished();
}

bool SJFScheduler::all_processes_finished(){
    for(Process* p : all_processes_){
        if(p->current_state.load() != Process::State::COMPLETED){
            return false;
        }
    }
    return true;
}

void SJFScheduler::dispatch_process(Process* p){
    if (!p){
        std::cerr << "ERROR: Attempted to dispatch null process" << std::endl;
        return;
    }

    if(p->current_state.load() == Process::State::READY){
        p->start_time.store(current_sim_time_); // Record start time

        p->last_latency.store(current_sim_time_ - p->arrival_time);
        p->current_state.store(Process::State::RUNNING);
        std::cout << "Process " << p->pid << " starting running at " << current_sim_time_.count() << "ns." << std::endl;
    } else if (p->current_state.load() == Process::State::BLOCKED){
        p->context_switches.fetch_add(1); // Increment context switch count
        p->last_latency.store(current_sim_time_ - p->last_run_timestamp.load()); // Time spent blocked
        p->current_state.store(Process::State::RUNNING);
        std::cout << "Process " << p->pid << " resuming from BLOCKED at " << current_sim_time_.count() << "ns." << std::endl;
    }

    p->execute_slice(current_sim_time_);

    current_sim_time_ += p->burst_time; // Use original burst_time for time advancement

    std::cout << " Process: " << p->pid << " COMPLETED at " << p->completion_time.load().count()
              << "ns (Burst: " << p->burst_time.count() << "ns)." << std::endl;
}


void SJFScheduler::handle_new_arrivals() {
    std::lock_guard<std::mutex> lock(scheduler_mutex_);

    for (Process* p : all_processes_){
        if (p->arrival_time <= current_sim_time_ &&
            p->current_state.load() == Process::State::READY && // It's in the initial READY state
            p->start_time.load().count() == 0) // And it hasn't been started/dispatched yet
        {
            // Check if there's space in the ready queue
            if (ready_queue_.size() < queue_capacity_) {
                // The process state remains READY; it will change to RUNNING when dispatched.
                ready_queue_.push(p);
                std::cout << "Process " << p->pid << " arrived and added to ready queue (SJF order) at "
                          << current_sim_time_.count() << "ns. Queue size: " << ready_queue_.size() << std::endl;
            } else {
                std::cerr << "WARNING: SJF ready queue full (capacity: " << queue_capacity_
                          << "), dropping process " << p->pid
                          << " (arrived at " << p->arrival_time.count() << "ns, sim_time: "
                          << current_sim_time_.count() << "ns)." << std::endl;
            }
        }
    }
}