#ifndef PROCESS_H
#define PROCESS_H

#include <chrono>
#include <atomic>
#include <string>
#include <functional>
#include <iostream>

using namespace std::chrono;
struct Process {
    enum class State { READY, RUNNING, BLOCKED, COMPLETED };
    
    int pid;
    std::chrono::nanoseconds arrival_time;
    std::chrono::nanoseconds burst_time;
    std::atomic<std::chrono::nanoseconds> remaining_time;
    std::atomic<std::chrono::nanoseconds> start_time;
    std::atomic<std::chrono::nanoseconds> completion_time;
    std::atomic<std::chrono::nanoseconds> last_run_timestamp;
    std::atomic<State> current_state;

    std::atomic<int> context_switches{0};
    std::atomic<std::chrono::nanoseconds> last_latency{std::chrono::nanoseconds(0)};

    std::atomic_flag paused = ATOMIC_FLAG_INIT;

    std::function<void()> task;

    Process(int pid, std::chrono::nanoseconds arrival,std::chrono::nanoseconds burst)
      : pid(pid), arrival_time(arrival), burst_time(burst), remaining_time(burst),
        start_time(std::chrono::nanoseconds(0)), completion_time(std::chrono::nanoseconds(0)), last_run_timestamp(std::chrono::nanoseconds(0)),
        current_state(State::READY){
        task=[this]() {
            std::cout << "Executing task" << std::endl;
            // simulating simple work done by CPU
            volatile int x = 0; // volatile to make sure CPU doesn't optimize
            for(int i = 0; i < 10; ++i){ i++; }
        };
    } 

    void execute_slice(std::chrono::nanoseconds curr_sim_time) {
        // using load to make sure operation is atomic
        if (current_state.load() == State::READY) {
            start_time.store(curr_sim_time);
            last_latency.store(curr_sim_time-arrival_time);
            current_state.store(State::RUNNING);
        } else if (current_state.load() == State::BLOCKED){
            // update number of context switches when running a blocked process since the process will start to run
            context_switches++;
            // amt of time current process was blocked for
            last_latency.store(curr_sim_time-last_run_timestamp.load());
            current_state.store(State::RUNNING);
        }

        task();
        // remaining_time.store(remaining_time.load() - 1ns);
        remaining_time.store(0ns);
        last_run_timestamp.store(curr_sim_time + 1ns);

        if(remaining_time.load() <= std::chrono::nanoseconds(0)){
            // completion_time.store(curr_sim_time + 1ns);
            completion_time.store(curr_sim_time + burst_time);
            current_state.store(State::COMPLETED);
        } else {
            current_state.store(State::BLOCKED);
        }
        std::cout << "Remaining time seen: " << remaining_time.load().count() << std::endl;
    }

    // metrics relating to the process
    std::chrono::nanoseconds get_turnaround_time() const { return completion_time.load() - arrival_time; }
    std::chrono::nanoseconds get_waiting_time() const { return get_turnaround_time() - burst_time; }
    std::chrono::nanoseconds get_response_time() const { return start_time.load() - arrival_time; }
};

#endif