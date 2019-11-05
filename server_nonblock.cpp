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
const auto _10ms = std::chrono::milliseconds(10);

void callback() {
    std::vector<char> buf(256);
    while (true) {
        bool recv_any = false;
        int i = 0, connect;
        while (true) {
            {  // during resize size() may return surprisingly big value :-)
                std::lock_guard<std::mutex> lg(mu);
                if (i >= connections.size())
                    break;
                connect = connections[i];
            }
            bool ok = false;
            while (true) {
                int ret;
                if ((ret = recv(connect, buf.data(), buf.capacity(), 0)) <= 0) {
                    if (ret == 0)  // connection closed
                        break;
                    if (errno == EINTR) {  // interrupted, try again
                        continue;
                    } else if (errno == EAGAIN) {  // no data
                        ok = true;
                        break;
                    }
                    perror("recv()");  // close and update connections list
                    break;
                }
                recv_any = ok = true;
                int size = ret;
                char *ptr = buf.data();
                while (size > 0 && (ret = send(connect, ptr, size, 0)) != 0) {
                    if (ret == -1) {
                        if (errno == EINTR)
                            continue;
                        perror("send()");
                        break;
                    }
                    size -= ret;
                    ptr += ret;
                }
                break;
            }
            if (ok) {
                i++;
            } else {  // close and update connections list
                close(connect);
                std::lock_guard<std::mutex> lg(mu);
                connections[i] = connections[connections.size() - 1];
                connections.pop_back();
            }
        }
        if (!recv_any)
            std::this_thread::sleep_for(_10ms);
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
                std::this_thread::sleep_for(_10ms);
                continue;
            } else {
                perror("accept()");
                exit(EXIT_FAILURE);
            }
        }
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
