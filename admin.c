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

#define MAX_CLIENT 100

int get_all_client(int *client_num, char clients_name[MAX_CLIENT][20], int client_limit[MAX_CLIENT]) {
    FILE *clients_list = fopen("admin.txt", "r");
    fscanf(clients_list, "%d", client_num);
    for (int i = 0; i < *client_num; i++) {
        fscanf(clients_list, "%s%d", clients_name[i], &client_limit[i]);
    }
    fclose(clients_list);
    return 0;
}

void show_list(int client_count, char clients_name[MAX_CLIENT][20], int client_limit[MAX_CLIENT]) {
    printf("\nindex\tname\tlimit\n");
    for (int i = 0; i < client_count; i++) {
        printf("%d\t%s\t%d\n", i, clients_name[i], client_limit[i]);
    }
    printf("\n");
}

int add_limit(int client_count, char clients_name[MAX_CLIENT][20], int client_limit[MAX_CLIENT]) {
    show_list(client_count, clients_name, client_limit);
    printf("Enter index to change limit(c to commit changes): ");
    char buffer[32];
    fgets(buffer, 32, stdin);
    buffer[strlen(buffer) - 1] = '\0';
    if (buffer[0] == 'c') {
        return 0;
    } else {
        int ind = atoi(buffer);
        if (ind < 0 && ind > client_count) {
            printf("Please enter valid index.\n");
            return -1;
        }
        printf("Enter new limit for %s : ", clients_name[ind]);
        int limit = 0;
        fscanf(stdin, "%d", &limit);
        if (limit < 0 && limit > 100) {
            printf("Invalid limit\n");
            return -1;
        }
        client_limit[ind] = limit;
    }
    return 0;
}

int admin(int socket_peer) {
    char buffer[2048];
    int ret = recv(socket_peer, buffer, 2048, 0);
    FILE *fp = fopen("admin.txt", "w");
    fwrite(buffer, ret, 1, fp);
    fclose(fp);
    char clients_name[MAX_CLIENT][20];
    int client_limit[MAX_CLIENT];
    int client_count = 0;
    get_all_client(&client_count, clients_name, client_limit);
    while (add_limit(client_count, clients_name, client_limit))
        ;
    fp = fopen("admin.txt", "w");
    fprintf(fp, "%d\n", client_count + 1);
    for (int i = 0; i < client_count; i++) {
        fprintf(fp, "%s %d\n", clients_name[i], client_limit[i]);
    }
    fclose(fp);

    fp = fopen("admin.txt", "r");
    int prev = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz = ftell(fp);
    fseek(fp, prev, SEEK_SET);
    fread(buffer, sz, 1, fp);

    send(socket_peer, buffer, sz, 0);

    printf("New limits commited to server\n");

    return 0;
}