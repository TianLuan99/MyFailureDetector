#include "udp_wrapper.h"

const int BUF_SIZE = 1024;

UdpWrapper::UdpWrapper(std::string ip_addr, unsigned int port)
{
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0)
    {
        perror("error when creating socket");
        exit(0);
    }
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(port);
    server_addr_.sin_addr.s_addr = (ip_addr == "") ? inet_addr(ip_addr.c_str()) : htonl(INADDR_ANY);
}

UdpWrapper::~UdpWrapper()
{
    close(sockfd_);
}

void UdpWrapper::send(std::string msg)
{
    sendto(sockfd_, msg.c_str(), msg.size(), 0, reinterpret_cast<sockaddr *>(&server_addr_), sizeof(server_addr_));
}

std::string UdpWrapper::recv(struct timeval timeout)
{
    char buf[BUF_SIZE];
    socklen_t addr_len = sizeof(server_addr_);
    if (timeout.tv_sec != 0 || timeout.tv_usec != 0) 
    {
        setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, (const char*)& timeout, sizeof(timeout));
    }
    try
    {
        int bytes_recv = recvfrom(sockfd_, buf, BUF_SIZE, 0, reinterpret_cast<sockaddr *>(&server_addr_), &addr_len);
        if (bytes_recv < 0)
        {
            perror("error when receiving message");
            exit(1);
        }

        return std::string(buf, bytes_recv);
    }
    catch(const std::exception& e)
    {
        return "timeout";
    }
}