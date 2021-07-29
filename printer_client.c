#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

int fsize(FILE *fp)
{
    int prev = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz = ftell(fp);
    fseek(fp, prev, SEEK_SET);
    return sz;
}

int get_file(int sockfd)
{
    int ret;
    char file_name[256];
    printf("Enter File name : ");
    fscanf(stdin, "%s", file_name);
    FILE *fp;
    if (fp = fopen(file_name, "r"))
    {
        int f_size = fsize(fp);
        char file_content[f_size];
        fread(file_content, f_size, 1, fp);
        fclose(fp);
        ret = send(sockfd, file_content, f_size, 0);
        return 0;
    }
    else
    {
        printf("No such file : %s\n", file_name);
        return -1;
    }
    return -1;
}

int printer_client(char server_ip[16], char port_no[6], char client_name[21])
{
    int ret;
    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *peer_address;
    if (getaddrinfo(server_ip, port_no, &hints, &peer_address))
    {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", errno);
        return -1;
    }

    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen, address_buffer, sizeof(address_buffer), service_buffer, sizeof(service_buffer), NI_NUMERICHOST);
    printf("%s : %s\n", address_buffer, service_buffer);

    printf("Creating socket...\n");
    int socket_peer;
    socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype, peer_address->ai_protocol);
    if (socket_peer < 0)
    {
        fprintf(stderr, "socket() failed. (%d)\n", errno);
        return -1;
    }

    printf("Connecting...\n");
    if (connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen))
    {
        fprintf(stderr, "connect() failed. (%d)\n", errno);
        return -1;
    }
    freeaddrinfo(peer_address);

    printf("Connected..\n");

    ret = send(socket_peer, client_name, strlen(client_name), 0);

    char client_lim[4];
    ret = recv(socket_peer, client_lim, 3, 0);
    int client_limit = atoi(client_lim);
    printf("You have %d page left.\n", client_limit);

    get_file(socket_peer);

    printf("Closing socket...\n");
    close(socket_peer);

    return 0;
}