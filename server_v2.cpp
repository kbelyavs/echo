#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#include <vector>
#include <thread>
#include <mutex>

std::mutex mu;
std::vector<int> connections;
fd_set readfds, copy;
int fd_max = 0;

void callback() {
    // printf("callback launched\n");
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;  // 0.1s
    std::vector<int> connects;
    while (true) {
        {
            std::lock_guard<std::mutex> lg(mu);  // use unique_lock instead
            connects = connections;
            FD_COPY(&readfds, &copy);
        }
        int ret = select(fd_max+1, &copy, NULL, NULL, &tv);
        if (ret == -1) {
            perror("select()");
            exit(EXIT_FAILURE);
        } else if (!ret) {
            printf("no data, timeout %d usec", tv.tv_usec);
            continue;
        }
        for (auto connect : connects) {
            std::vector<char> buf(256);
            if (FD_ISSET(connect, &copy)) {
                int len = recv(connect, buf.data(), buf.size(), 0);
                if (len == 0)
                    continue;  // to update connections list
                else if (len < 0) {
                    perror("recv()");
                    exit(EXIT_FAILURE);
                }
                // printf("Received (server) : %s\n", buf.data());
                send(connect, buf.data(), buf.size(), 0);
            }
        }
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
    srv_addr.sin_family = PF_INET;
    srv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    srv_addr.sin_port = htons(1234);
    FD_ZERO(&readfds);
    if (bind(server, (struct sockaddr*) &srv_addr, addr_size) != 0) {
        perror("bind()");
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
            perror("accept()");
            exit(EXIT_FAILURE);
        }
        // printf("A new connection established\n");
        {
            std::lock_guard<std::mutex> lg(mu);
            FD_SET(connect, &readfds);
            connections.push_back(connect);
            if (connect > fd_max)
                fd_max = connect;
        }
        if (!run) {
            run = true;
            std::thread t(callback);
            t.detach();
        }
    }
    for (int con : connections)
        close(con);
    close(server);
    return 0;
}
