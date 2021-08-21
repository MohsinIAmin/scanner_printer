#include <stdio.h>
#include <string.h>

#include "server.h"

void first_printer_config(FILE *server, char printer_ip[16], int *port_no) {
    printf("Welcome\n");
    printf("Please Enter Printer IP address: ");
    fgets(printer_ip, 16, stdin);
    printer_ip[strlen(printer_ip) - 1] = '\0';
    printf("Please Enter Printer Port: ");
    fscanf(stdin, "%d", port_no);
    fprintf(server, "%s\n", printer_ip);
    fprintf(server, "%d\n", *port_no);
}

void read_printer_config(char printer_ip[16], int *port_no) {
    FILE *server = fopen("server.txt", "r");
    fgets(printer_ip, 16, server);
    printer_ip[strlen(printer_ip) - 1] = '\0';
    fscanf(server, "%d", port_no);
}

void server_init(char printer_ip[16], int *port_no) {
    FILE *server = fopen("server.txt", "a");
    long int len = ftell(server);
    if (len == 0) {
        first_printer_config(server, printer_ip, port_no);
        fclose(server);
    } else if (len > 0) {
        fclose(server);
        read_printer_config(printer_ip, port_no);
    } else {
        fprintf(stderr, "Unknown error\n");
    }
}

int main() {
    int ret;
    char printer_ip[16];
    int port_no;
    server_init(printer_ip, &port_no);
    fprintf(stdout, "Printer IP : %s\n\n", printer_ip);
    fprintf(stdout, "Printer Port : %d\n\n", port_no);

    char server_ip[16];
    ret = get_ip_address(server_ip);
    if (ret == -1) {
        fprintf(stderr, "cannot find a valid ip address\n");
        return -1;
    }
    fprintf(stdout, "Use Server IP %s To Add Client\n", server_ip);

    int server_port;
    fprintf(stdout, "\nEnter server port no : ");
    fscanf(stdin, "%d", &server_port);

    ret = start_server(server_port);

    return 0;
}