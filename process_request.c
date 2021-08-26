#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "server.h"

int get_client_number() {
    FILE *clients = fopen("clients.txt", "a");
    long int size = ftell(clients);
    // fclose(clients);
    if (size == 0)
        return 0;
    else {
        fclose(clients);
        clients = fopen("clients.txt", "r");
        fscanf(clients, "%ld", &size);
        ftell(clients);
        fclose(clients);
        return size;
    }
    return 0;
}

int get_all_client(int num_client, char clients_name[MAX_CLIENT][20], int client_limit[MAX_CLIENT]) {
    FILE *clients_list = fopen("clients.txt", "r");
    int client_num;
    fscanf(clients_list, "%d", &client_num);
    for (int i = 0; i < num_client; i++) {
        fscanf(clients_list, "%s%d", clients_name[i], &client_limit[i]);
    }
    fclose(clients_list);
    return 0;
}

int new_client(int num_client, char clients_name[MAX_CLIENT][20], char client_name[20]) {
    for (int i = 0; i < num_client; i++) {
        if (!strcmp(client_name, clients_name[i])) {
            return i;
        }
    }
    return -1;
}

int add_new_client(int num_client, char clients_name[MAX_CLIENT][20], int client_limit[MAX_CLIENT], char client_name[20]) {
    FILE *clients_list = fopen("clients.txt", "w");
    fprintf(clients_list, "%d\n", num_client + 1);
    for (int i = 0; i < num_client; i++) {
        fprintf(clients_list, "%s %d\n", clients_name[i], client_limit[i]);
    }
    fprintf(clients_list, "%s %d\n", client_name, 5);
    fclose(clients_list);
    get_all_client(num_client + 1, clients_name, client_limit);
    return 0;
}

void update_client_list(int index, char clients_name[MAX_CLIENT][20], int client_limit[MAX_CLIENT]) {
    int num_client = get_client_number();
    FILE *clients_list = fopen("clients.txt", "w");
    fprintf(clients_list, "%d\n", num_client);
    for (int i = 0; i < num_client; i++) {
        if (i == index) {
            if (client_limit[i] <= 0) {
                fprintf(clients_list, "%s %d\n", clients_name[i], 0);
            } else {
                fprintf(clients_list, "%s %d\n", clients_name[i], client_limit[i] - 1);
            }
        } else {
            fprintf(clients_list, "%s %d\n", clients_name[i], client_limit[i]);
        }
    }
    fclose(clients_list);
    return;
}

int process_request(int client_socket) {
    int num_client = get_client_number();
    char clients_name[MAX_CLIENT][20];
    int client_limit[MAX_CLIENT];
    if (num_client == 0) {
    } else {
        get_all_client(num_client, clients_name, client_limit);
    }
    int ret;
    char buffer[8192];
    ret = recv(client_socket, buffer, 8192, 0);
    buffer[ret] = '\0';
    buffer[ret + 1] = '\0';
    buffer[ret + 2] = '\0';
    buffer[ret + 3] = '\0';
    buffer[ret + 4] = '\0';

    if (strcmp(buffer, "admin") == 0) {
        serve_admin(client_socket);
    } else {
        int index = new_client(num_client, clients_name, buffer);
        if (index == -1) {
            add_new_client(num_client, clients_name, client_limit, buffer);
            //send(client_limit[num_client])
            sprintf(buffer, "%03d", client_limit[num_client]);
            ret = send(client_socket, buffer, 3, 0);
        } else {
            //send(client_limit[index])
            sprintf(buffer, "%03d", client_limit[index]);
            ret = send(client_socket, buffer, 3, 0);
            if (client_limit[index] <= 0) {
                return 0;
            }
        }

        ret = recv(client_socket, buffer, 8192, 0);

        //send to printer

        ret = server_to_printer(buffer);

        if (index == -1) {
            printf("\nFile printed : %s\n", clients_name[num_client]);
            if (ret == 0) {
                update_client_list(num_client, clients_name, client_limit);
                sprintf(buffer, "%03d", client_limit[num_client] - 1);
            } else {
                sprintf(buffer, "%03d", client_limit[num_client]);
            }
            ret = send(client_socket, buffer, 3, 0);
        } else {
            printf("\nFile printed : %s\n", clients_name[index]);
            if (ret == 0) {
                update_client_list(index, clients_name, client_limit);
                sprintf(buffer, "%03d", client_limit[index] - 1);
            } else {
                sprintf(buffer, "%03d", client_limit[index]);
            }
            ret = send(client_socket, buffer, 3, 0);
        }
    }

    return 0;
}