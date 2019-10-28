#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#include <vector>
#include <thread>

int kq = kqueue();
int MAX_CONN = 1000;

void callback() {
    // printf("callback launched\n");
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;  // 0.1s
    struct kevent evList[MAX_CONN];
    while (true) {
        int nev = kevent(kq, NULL, 0, evList, MAX_CONN, NULL);
        for (int i = 0; i < nev; i++) {
            std::vector<char> buf(256);
            if (evList[i].filter == EVFILT_READ) {
                int len = recv((int)evList[i].ident, buf.data(), buf.size(), 0);
                if (len == 0)
                    continue;  // to update connections list
                else if (len < 0) {
                    perror("recv()");
                    exit(EXIT_FAILURE);
                }
                // printf("Received (server) : %s\n", buf.data());
                send((int)evList[i].ident, buf.data(), buf.size(), 0);
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
    struct kevent evSet;
    if (bind(server, (struct sockaddr*) &srv_addr, addr_size) != 0) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }
    if (listen(server, MAX_CONN) != 0) {
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
        EV_SET(&evSet, connect, EVFILT_READ, EV_ADD, 0, 0, NULL);
        kevent(kq, &evSet, 1, NULL, 0, NULL);
        if (!run) {
            run = true;
            std::thread t(callback);
            t.detach();
        }
    }
    return 0;
}
