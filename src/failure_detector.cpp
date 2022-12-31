#include <iostream>
#include <chrono>
#include <set>
#include <map>
#include <mutex>
#include <thread>

constexpr std::chrono::milliseconds MAX_RESPONSE_TIME(1000);

enum Status
{
    ALIVE,
    SUSPECTED
};

struct ProcessStatus
{
    Status alive;
    std::chrono::steady_clock::time_point last_heard;
};

class FailureDetector
{
public:
    FailureDetector(const std::set<std::string> &processes)
    {
        for (auto process : processes)
        {
            // initialize ll processes as alive at first
            statuses_[process] = {ALIVE, std::chrono::steady_clock::now()};
        }
    }

    // join a process
    void join_process(std::string process)
    {
        std::cout << "Join process " << process << std::endl;
        mark_process_alive_(process);
        std::thread t([=]
                      { 
                std::cout << "Start heartbeat for process " << process << std::endl;
                while (true) {
                    // sleep a period and then do heartbeat
                    std::this_thread::sleep_for(MAX_RESPONSE_TIME);
                    if (heartbeat_(process)) {
                        // target process is still alive
                        if (this -> statuses_[process].alive == SUSPECTED) {
                            // if suspect this process, mark it as alive
                            this -> mark_process_alive_(process);
                        }
                    } else {
                        // target process maybe dead
                        if (this -> statuses_[process].alive == ALIVE) {
                            // suspect this process if its status is alive
                            this -> mark_process_suspected_(process);
                        } else {
                            // mark this process as dead if we have suspected this process
                            this -> mark_process_dead_(process);
                            // TODO: do something if need when detect a process has become dead
                            return;
                        }
                    }
                } });
        t.detach();
    }

    void leave_process(std::string process)
    {
        mark_process_dead_(process);
    }

    void get_alive_process()
    {
        std::lock_guard lock(mutex_);
        for (auto [process, status] : statuses_)
        {
            std::cout << "| " << process << " |";
        }
        std::cout << std::endl;
    }

private:
    std::map<std::string, ProcessStatus> statuses_;
    std::mutex mutex_;

    // mark a process as alive
    void mark_process_alive_(std::string process)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        statuses_[process].alive = ALIVE;
        statuses_[process].last_heard = std::chrono::steady_clock::now();
    }

    // mark a process as suspected
    void mark_process_suspected_(std::string process)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        statuses_[process].alive = SUSPECTED;
    }

    // handle failed process
    void mark_process_dead_(std::string process)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!statuses_.erase(process))
            // TODO: log this fail message
            std::cout << "Delete failed process failed" << std::endl;
    }

    bool heartbeat_(std::string process)
    {
        // TODO: complete heartbeat part
        // std::cout << "Send heartbeat to process " << process << std::endl;
        return true;
    }
};

int main()
{
    std::set<std::string> processes = {"first_process", "second_process", "third_process"};
    FailureDetector detector({});

    for (auto process : processes)
    {
        detector.join_process(process);
    }

    std::cout << "All processes joined, we have: " << std::endl;
    detector.get_alive_process();

    int num = 0;
    while (true)
    {
        num++;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        if (num == 5)
        {
            std::cout << "10 seconds passed, let me kill the first process" << std::endl;
            detector.leave_process("first_process");
            std::cout << "Now we have:" << std::endl;
            detector.get_alive_process();
        }
    }
}