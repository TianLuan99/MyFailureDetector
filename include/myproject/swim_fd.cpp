#include "swim_fd.h"

const unsigned int PORT = 12345;
const unsigned int PING_REQ_PORT = 12346;
const unsigned int PING_REQ_ACK_PORT = 12347;
const unsigned int FAIL_PORT = 12348;

SWIM::SWIM(std::string id, int num_processes, std::vector<std::string> processes)
    : id_(id),
      num_processes_(num_processes),
      processes_(std::move(processes)),
      rng_(std::random_device{}()){};

void SWIM::ping(std::string target)
{
    // TODO: all following logic
    // ping target and wait for ack
    struct timeval timeout
    {
        1, 0
    };
    UdpWrapper wrapper(target, PORT);

    Msg ping_msg{PING, id_, ""};
    wrapper.send(convert_message_(ping_msg));

    while (true)
    {
        std::string recv_str = wrapper.recv(timeout);
        if (recv_str != "timeout")
        {
            Msg recv_msg = resolve_message_(recv_str);
            if (recv_msg.msg_type == ACK && recv_msg.src_id == target && recv_msg.payload == "")
                return;
        }
        else
        {
            std::vector<std::string> helpers = {"1", "2", "3"};
            ping_req(target, helpers);
            return;
        }
    }
};

void SWIM::ping_req(std::string target, std::vector<std::string> helpers)
{
    UdpWrapper wrapper("", PING_REQ_ACK_PORT);
    struct timeval timeout
    {
        3, 0
    };
    for (auto helper : helpers)
    {
        // send msg to let helper ping target process
        std::thread t([&]
                      {
            UdpWrapper helper_wrapper(helper, PING_REQ_PORT);
            Msg send_msg = {PING_REQ, id_, target};
            std::ostringstream formatter;
            formatter << send_msg.msg_type << " " << send_msg.src_id << " " << send_msg.payload;
            helper_wrapper.send(formatter.str()); });
        t.detach();
    }

    while (true)
    {
        std::string recv_str = wrapper.recv(timeout);
        if (recv_str == "timeout")
        {
            fail(target);
            return;
        }
        else
        {
            Msg recv_msg = resolve_message_(recv_str);
            std::istringstream converter(recv_str);
            if (recv_msg.msg_type == ACK && recv_msg.payload == target)
                return;
        }
    }
};

void SWIM::ack(std::string target)
{
    // send back msg to let target know this process is still alive
    UdpWrapper wrapper(target, PORT);

    Msg ping_msg{PING, id_, ""};
    std::ostringstream formatter;
    formatter << ping_msg.msg_type << " " << ping_msg.src_id << " " << ping_msg.payload;
    wrapper.send(formatter.str());
};

void SWIM::join(std::string process)
{
    // handle join message
    std::lock_guard<std::mutex> lock(mu_);
    ++num_processes_;
    processes_.emplace_back(process);
};

void SWIM::fail(std::string process)
{
    std::lock_guard<std::mutex> lock(mu_);
    --num_processes_;
    for (auto it = processes_.begin(); it != processes_.end(); ++it) 
    {
        if (*it == id_)
            continue;
            
        if (*it == process)
        {
            processes_.erase(it);
            break;
        }
        else
        {
            UdpWrapper failWrapper(*it, FAIL_PORT);
            Msg failMsg = {FAIL, id_, process};
            failWrapper.send(convert_message_(failMsg));
        }
    }
    return;
}

void SWIM::listen()
{
    UdpWrapper wrapper("", PING_REQ_PORT);
    while (true)
    {
        std::string recv_str = wrapper.recv();
        Msg msg = resolve_message_(recv_str);
        if (msg.msg_type == PING_REQ)
        {
            std::string target = msg.payload;
            Msg send_msg{PING, id_, ""};
            UdpWrapper ping_wrapper(target, PORT);
            ping_wrapper.send(convert_message_(send_msg));
            timeval timeout{1, 0};
            try
            {
                std::string bytes_recv = ping_wrapper.recv(timeout);
                Msg recv_msg = resolve_message_(bytes_recv);
                if (recv_msg.msg_type == ACK && recv_msg.src_id == target)
                {
                    UdpWrapper ack_wrapper(msg.src_id, PING_REQ_ACK_PORT);
                    Msg ack_msg{ACK, id_, target};
                    ack_wrapper.send(convert_message_(ack_msg));
                    return;
                }
            }
            catch (const std::exception &e)
            {
                return;
            }
        }
    }
}

void SWIM::run()
{
    std::thread t(&listen);
    t.detach();

    // peoridically send ping message
    while (true)
    {
        std::uniform_int_distribution<> dist(0, num_processes_ - 1);
        int target_idx = dist(rng_);
        if (processes_[target_idx] == id_)
            continue;
        ping(processes_[target_idx]);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    };
}

Msg SWIM::resolve_message_(std::string msg)
{
    int msg_type;
    Msg ret_msg;
    std::istringstream converter(msg);
    converter >> msg_type;
    converter >> ret_msg.src_id;
    converter >> ret_msg.payload;
    ret_msg.msg_type = (Type)msg_type;
    return ret_msg;
}

std::string SWIM::convert_message_(Msg msg)
{
    std::ostringstream formatter;
    formatter << msg.msg_type << " " << msg.src_id << " " << msg.payload;
    return formatter.str();
}