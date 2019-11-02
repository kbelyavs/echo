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
    struct kevent evList[MAX_CONN];
    std::vector<char> buf(256);
    while (true) {
        int nev = kevent(kq, NULL, 0, evList, MAX_CONN, NULL);
        if (nev == -1) {
            if (errno == EINTR)  // interrupted, try again
                continue;
            perror("kevent()");
            return;
        }
        for (int i = 0; i < nev; i++) {
            if (evList[i].filter != EVFILT_READ)
                continue;
            int connect = static_cast<int>(evList[i].ident);
            while (true) {
                int ret = recv(connect, buf.data(), buf.capacity(), 0);
                if (ret <= 0) {
                    if (ret == -1 && errno == EINTR)  // interrupted, try again
                        continue;
                    if (ret == -1)
                        perror("recv()");
                    printf("client %d disconnected\n", connect);
                    close(connect);  // close will also remove associated event
                    break;
                }
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
