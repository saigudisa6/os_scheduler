#include <iostream>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <mutex>
#include "Process.h"

class CircularBuffer{
    std::vector<Process*> queue_;
    int capacity_;
    int current_size_ = 0;
    int head_ = 0;
    int tail_ = 0;
    std::mutex mutex_;

public:
    explicit CircularBuffer(int capacity) : capacity_(capacity), queue_(capacity) {
        std::cout << "Initialized Circular Buffer with capacity: " << capacity_ << std::endl;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mutex_);
        return this->current_size_ == 0;
    }

    bool full(){
        std::lock_guard<std::mutex> lock(mutex_);
        return current_size_ == capacity_;
    }

    bool enqueue(Process* p){
        if(!p){
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        if(this->full()){
            // we have too many tasks, need to get rid of the oldest one
            std::cerr << "WARNING: Process Queue full, cannot enqueue new process: " << p->pid << std::endl;
            return false;
        } 

        queue_[tail_] = p;
        tail_ = (tail_ + 1) % capacity_;
        current_size_++;
        return true;
    }

    Process* dequeue(){
        std::lock_guard<std::mutex> lock(mutex_);

        if (empty()){
            return nullptr;
        }

        Process* p = queue_[head_];
        head_ = (1+head_) % capacity_;
        current_size_--;

        return p;
    }

    Process* peek() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (empty()){
            return nullptr;
        }

        return queue_[head_];
    }
};