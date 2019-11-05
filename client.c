#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

const int MAX_SZ = 256;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("usage: %s port\n", argv[0]);
        return 0;
    }
    int port = atoi(argv[1]);
    if (port < 0 || port > 65535) {
        printf("bad port number (%d)\n", port);
        printf("usage: %s port\n", argv[0]);
        return 0;
    }
    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in srv_addr, clnt_addr;
    clnt_addr.sin_family = AF_INET;
    clnt_addr.sin_port = htons(port);
    clnt_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(1234);
    srv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(client, (struct sockaddr*) &clnt_addr, sizeof clnt_addr) != 0) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }
    int con = connect(client, (struct sockaddr*) &srv_addr, sizeof srv_addr);
    if (con < 0) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }
    printf("Client Connected\n");
    struct timespec before, after;
    clockid_t id = CLOCK_MONOTONIC;
    char data_in[MAX_SZ], data_out[MAX_SZ];
    snprintf(data_out, MAX_SZ, "Hello from %d", port);
    sranddev();
    uint64_t ping_total = 0;
    int count = 0;
    while (1) {
        int size = strlen(data_out) + 1, ret;
        char *ptr = data_out;
        while (size > 0 && (ret = send(client, ptr, size, 0)) != 0) {
            if (ret == -1) {
                if (errno == EINTR)
                    continue;
                perror("send()");
                break;
            }
            size -= ret;
            ptr += ret;
        }
        clock_gettime(id, &before);
        if (recv(client, data_in, MAX_SZ, 0) <= 0)
            break;
        clock_gettime(id, &after);
        int ping = after.tv_nsec - before.tv_nsec;
        if (ping < 0)
            ping += 1000000000;
        assert(strncmp(data_in, data_out, MAX_SZ) == 0);
        ping = ping / 1000;  // microseconds
        ping_total += ping;
        count++;
        printf("Client %d: ping %6d us, avg ping %6llu us\n", port, ping,
               ping_total / count);
        usleep(rand() % 1000000);  // NOLINT(runtime/threadsafe_fn)
    }
    close(client);
    return 0;
}
