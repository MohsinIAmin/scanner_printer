#include "server.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include <stdio.h>
#include <string.h>

int get_client_number()
{
    FILE *clients = fopen("clients.txt", "a");
    long int size = ftell(clients);
    if (size == 0)
        return 0;
    else
    {
        fclose(clients);
        clients = fopen("clients.txt", "r");
        fscanf(clients, "%ld", &size);
        fclose(clients);
        return size;
    }
    return 0;
}
int get_all_client(int num_client, char clients_name[MAX_CLIENT][20], int client_limit[MAX_CLIENT])
{
    FILE *clients_list = fopen("clients.txt", "r");
    int client_num;
    fscanf(clients_list, "%d", &client_num);
    for (int i = 0; i < num_client; i++)
    {
        fscanf(clients_list, "%s%d", clients_name[i], client_limit[i]);
    }
    fclose(clients_list);
    return 0;
}

int new_client(int num_client, char clients_name[MAX_CLIENT][20], char client_name[20])
{
    for (int i = 0; i < num_client; i++)
    {
        if (!strcmp(client_name, clients_name[i]))
        {
            return i;
        }
    }
    return -1;
}

int add_new_client(int num_client, char clients_name[MAX_CLIENT][20], int client_limit[MAX_CLIENT], char client_name[20])
{
    FILE *clients_list = fopen("clients.txt", "w");
    fprintf(clients_list, "%d\n", num_client + 1);
    for (int i = 0; i < num_client; i++)
    {
        fprintf(clients_list, "%s %d\n", clients_name[i], client_limit[i]);
    }
    fprintf(clients_list, "%s %d\n", client_name, 5);
    fclose(clients_list);
    get_all_client(num_client + 1, clients_name, client_limit);
    return 0;
}

int process_request(int client_socket)
{
    int num_client = get_client_number();
    char clients_name[MAX_CLIENT][20];
    int client_limit[MAX_CLIENT];
    if (num_client == 0)
    {
    }
    else
    {
        get_all_client(num_client, clients_name, client_limit);
    }
    int ret;
    char buffer[2048];
    ret = recv(client_socket, buffer, 2048, 0);
    buffer[ret] = '\0';
    int index = new_client(num_client, clients_name, buffer);
    if (index == -1)
    {
        add_new_client(num_client, clients_name, client_limit, buffer);
        //send(client_limit[num_client])
        sprintf(buffer, "%03d", client_limit[num_client]);
        ret = send(client_socket, buffer, 3, 0);
    }
    else
    {
        //send(client_limit[index])
        sprintf(buffer, "%03d", client_limit[index]);
        ret = send(client_socket, buffer, 3, 0);
    }

    ret = recv(client_socket, buffer, 2048, 0);

    //send to printer

    return 0;
}