#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int serve_admin(int client_socket) {
    FILE *fp = fopen("clients.txt", "r");
    int prev = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz = ftell(fp);
    fseek(fp, prev, SEEK_SET);
    char buffer[2048];
    fread(buffer, sz, 1, fp);

    send(client_socket, buffer, sz, 0);

    int ret = recv(client_socket, buffer, 2048, 0);
    buffer[ret] = '\0';
    fp = fopen("clients.txt", "w");
    fwrite(buffer, ret, 1, fp);
    fclose(fp);

    return 0;
}