#pragma once

#include <algorithm>
#include <thread>
#include <iostream>
#include <random>
#include <vector>
#include <chrono>
#include <map>
#include <set>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "include/myproject/udp_wrapper.h"
#include <sstream>
#include <sys/time.h>

const unsigned int PORT = 12345;

enum Type 
{
    PING,
    ACK,
    PING_REQ
};

struct Msg
{
    Type msg_type;
    std::string src_id;
    std::string payload;
};

class SWIM
{
public:
    SWIM(std::string id, int num_processes, std::vector<std::string> processes);
    void ping(std::string target);
    void ping_req(std::string target, std::vector<std::string> helpers);
    void ack(std::string target);
    void join(std::string process);
    void fail(std::string process);
    void listen();
    void run();

private:
    std::string id_;
    int num_processes_;
    std::mt19937 rng_;
    std::vector<std::string> processes_;
    std::mutex mu_;
    Msg resolve_message_(std::string msg);
    std::string convert_message_(Msg msg);
};