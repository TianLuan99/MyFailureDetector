#include <iostream>
#include <chrono>
#include <set>
#include <map>
#include <mutex>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

constexpr std::chrono::milliseconds MAX_RESPONSE_TIME(1000);
const int MAX_TIMEOUT_COUNT = 3;

class FailureDetector
{
public:
    FailureDetector(const std::set<std::string> &processes)
    {
        for (auto process : processes)
        {
            // initialize all processes count as 0 at first to indicate it is alive
            statuses_[process] = 0;
        }
    }

    // join a process
    void join_process(std::string process)
    {
        std::cout << "Join process " << process << std::endl;
        mark_process_alive_(process);

        std::thread check_t([=]
                            { 
                std::cout << "Start heartbeat for process " << process << std::endl;
                while (true) {
                    // sleep a period and then do heartbeat
                    std::this_thread::sleep_for(MAX_RESPONSE_TIME);
                    this -> statuses_[process]++;
                    if (this -> statuses_[process] == MAX_TIMEOUT_COUNT) {
                        // target process is dead
                        this -> mark_process_dead_(process);
                        // TODO: do something if need when detect a process has become dead
                        std::cout << "Process " << process << " dead!" << std::endl;
                        return;
                    }
                } });
        check_t.detach();
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
    std::map<std::string, int> statuses_;
    std::mutex mutex_;

    // mark a process as alive
    void mark_process_alive_(std::string process)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        statuses_[process] = 0;
    }

    // handle failed process
    void mark_process_dead_(std::string process)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!statuses_.erase(process))
            // TODO: log this fail message
            std::cout << "Delete failed process failed" << std::endl;
    }
};

void listen(int port)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        std::cout << "Error when creating socket." << std::endl;
        return;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (sockaddr *)&addr, sizeof(addr) < 0) < 0)
    {
        std::cout << "Error binding socket." << std::endl;
        return;
    }

    sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);
    char buffer[1024];

    while (true)
    {
        int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, (sockaddr *)&sender_addr, &sender_addr_len);
        if (bytes_received < 0)
        {
            std::cout << "Error receiveing message." << std::endl;
            return;
        }
    }
};

int main()
{
    std::set<std::string> processes = {"127.0.0.1", "127.0.0.2", "127.0.0.3"};
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
            detector.leave_process("127.0.0.1");
            std::cout << "Now we have:" << std::endl;
            detector.get_alive_process();
        }
    }
}