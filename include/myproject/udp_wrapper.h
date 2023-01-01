#pragma once
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>

class UdpWrapper
{
public:
    UdpWrapper(std::string ip_addr, unsigned int port);
    ~UdpWrapper();
    void send(std::string msg);
    std::string recv(struct timeval timeout = timeval{0, 0});

private:
    int sockfd_;
    struct sockaddr_in server_addr_;
};