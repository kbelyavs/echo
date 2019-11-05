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
fd_set readfds, _readfds;  // FD_SETSIZE 1024 by default
int fd_max = 0;

void callback() {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;  // 10ms
    std::vector<int> _connections;
    std::vector<char> buf(256);
    while (true) {
        {
            std::lock_guard<std::mutex> lg(mu);
            _connections = connections;
            FD_COPY(&readfds, &_readfds);
        }
        int ret = select(fd_max+1, &_readfds, NULL, NULL, &tv);
        if (ret == -1) {
            perror("select()");
            exit(EXIT_FAILURE);
        } else if (!ret) {
            printf("no data, timeout %d usec", tv.tv_usec);
            continue;
        }
        int nconns = _connections.size();
        for (int i = 0; i < nconns; i++) {
            int connect = _connections[i];
            if (!FD_ISSET(connect, &_readfds))
                continue;
            bool ok = false;
            while (true) {
                if ((ret = recv(connect, buf.data(), buf.capacity(), 0)) <= 0) {
                    if (ret == 0)  // connection closed
                        break;
                    if (errno == EINTR)  // interrupted, try again
                        continue;
                    perror("recv()");  // close and update connections list
                    break;
                }
                ok = true;
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
            if (!ok) {  // close and update connections list
                close(connect);
                std::lock_guard<std::mutex> lg(mu);
                connections[i] = connections[connections.size() - 1];
                connections.pop_back();
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
    srv_addr.sin_family = AF_INET;
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
    return 0;
}
