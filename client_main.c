#include "client.h"

#include <stdio.h>
#include <string.h>

void first_server_config(FILE *client, char server_ip[16], char port_no[6])
{
    printf("Please Enter Server IP address: ");
    fgets(server_ip, 16, stdin);
    server_ip[strlen(server_ip) - 1] = '\0';
    printf("Please Enter Server Port: ");
    fgets(port_no, 6, stdin);
    port_no[strlen(port_no) - 1] = '\0';
    fprintf(client, "%s\n", server_ip);
    fprintf(client, "%s\n", port_no);
}

void read_server_config(char server_ip[16], char port_no[6])
{
    FILE *client = fopen("client.txt", "r");
    fgets(server_ip, 16, client);
    server_ip[strlen(server_ip) - 1] = '\0';
    fgets(port_no, 6, client);
    port_no[strlen(port_no) - 1] = '\0';
}

void client_init(char server_ip[16], char port_no[6])
{
    FILE *client = fopen("client.txt", "a");
    long int len = ftell(client);
    if (len == 0)
    {
        first_server_config(client, server_ip, port_no);
        fclose(client);
    }
    else if (len > 0)
    {
        fclose(client);
        read_server_config(server_ip, port_no);
    }
    else
    {
        fprintf(stderr, "Unknown error\n");
    }
}

int main(int argc, char *argv[])
{
    char server_ip[16];
    char port_no[6];
    char client_name[21];
    printf("Welcome\n");
    printf("Please Enter your name : ");
    fgets(client_name, 21, stdin);
    client_name[strlen(client_name) - 1] = '\0';

    client_init(server_ip, port_no);

    printer_client(server_ip, port_no, client_name);

    fprintf(stdout, "%s\n", server_ip);
    fprintf(stdout, "%s\n", port_no);

    return 0;
}