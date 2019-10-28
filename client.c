#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
  
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("usage: %s port\n", argv[0]);
        return 0;
    }
    int port = atoi(argv[1]);
    if (port < 0 || port >= 65536) {
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
    char data_in[256], data_out[256];
    sprintf(data_out, "Hello from %d", port);
    sranddev();
    while (1) {
        send(client, data_out, 256, 0);
        if (recv(client, data_in, 256, 0) <= 0)
            break;
        assert(strncmp(data_in, data_out, 256) == 0);
        printf("Received (client) : %s\n", data_in);
        usleep(rand() % 1000000);
    }
    close(client);
    return 0;
}
