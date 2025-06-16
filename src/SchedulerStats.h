#include <chrono>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cmath>

using namespace std::chrono;

struct SchedulerStats {
    nanoseconds total_sim_time = 0ns;
    nanoseconds total_cpu_burst_time = 0ns;

    std::vector<nanoseconds> turnaround_times;
    std::vector<nanoseconds> waiting_times;
    std::vector<nanoseconds> response_times;
    std::vector<nanoseconds> context_switch_latencies;

    int total_processes_completed = 0;
    int total_context_switches = 0;

    void add_process_stats(const Process* p){
        // make sure the process is completed first
        if (p->current_state.load() == Process::State::COMPLETED) {
            turnaround_times.push_back(p->get_turnaround_time());
            waiting_times.push_back(p->get_waiting_time());
            response_times.push_back(p->get_response_time());
            total_cpu_burst_time += p->burst_time;
        }
        total_context_switches += p->context_switches.load();
    }
    
    // returns the item at the given percentile
    nanoseconds calculate_percentile(const std::vector<nanoseconds>& data, double percentile) const {
        if(data.empty()) return 0ns;

        std::vector<nanoseconds> sorted_data = data;
        std::sort(sorted_data.begin(), sorted_data.end());
        size_t index = static_cast<size_t>(ceil(percentile / 100.0 * sorted_data.size())) - 1;

        return sorted_data[index];
    }

    void print() const {
        std::cout << "Avg Turnaround: " << calculate_average(turnaround_times) << "ns\n";
        std::cout << "P99 Turnaround: " << calculate_percentile(turnaround_times, 99.0) << "ns\n";
        std::cout << "Avg Waiting: " << calculate_average(waiting_times) << "ns\n";
        std::cout << "P99 Waiting: " << calculate_percentile(waiting_times, 99.0) << "ns\n";
        std::cout << "Avg Response: " << calculate_average(response_times) << "ns\n";
        std::cout << "P99 Response: " << calculate_percentile(response_times, 99.0) << "ns\n";
        std::cout << "Total Context Switches: " << total_context_switches << "\n";
        std::cout << "CPU Utilization: " << (static_cast<double>(total_cpu_burst_time.count()) / total_sim_time.count()) * 100.0 << "%\n";
    }

private:
    nanoseconds calculate_average(const std::vector<nanoseconds>& data) const {
        if(data.empty()) return 0ns;
        return std::accumulate(data.begin(), data.end(), 0ns) / data.size();
    }
};