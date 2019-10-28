#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#include <vector>
#include <thread>
#include <mutex>

std::mutex mu;
std::vector<int> connections;

void callback() {
    // printf("callback launched\n");
    std::vector<int> connects;
    while (true) {
        {
            std::lock_guard<std::mutex> lg(mu);  // use unique_lock
            connects = connections;
        }
        if (connects.size() == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        bool read = false;
        for (auto connect : connects) {
            std::vector<char> buf(256);
            if (recv(connect, buf.data(), buf.size(), 0) <= 0)
                continue;  // to update connections list 0; -1 && !EAGAIN -> err
            read = true;
            // printf("Received (server) : %s\n", buf.data());
            send(connect, buf.data(), buf.size(), 0);
        }
        if (!read)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in srv_addr, clnt_addr;
    uint32_t addr_size = sizeof(srv_addr);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    srv_addr.sin_port = htons(1234);
    if (bind(server, (struct sockaddr*) &srv_addr, addr_size) != 0) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }
    int flags = fcntl(server, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(get)");
        exit(EXIT_FAILURE);
    }
    if (fcntl(server, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl(set)");
        exit(EXIT_FAILURE);
    }
    if (listen(server, 1000) != 0) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }
    bool run = false;
    while (true) {
        int connect = accept(server, (struct sockaddr*) &clnt_addr, &addr_size);
        if (connect < 0) {
            if (errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            } else {
                perror("accept()");
                exit(EXIT_FAILURE);
            }
        }
        // printf("A new connection established\n");
        {
            std::lock_guard<std::mutex> lg(mu);
            connections.push_back(connect);
        }
        if (!run) {
            run = true;
            std::thread t(callback);
            t.detach();
        }
    }
    return 0;
}
