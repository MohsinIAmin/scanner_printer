#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

#define DATA_SIZE 100000

int port;
char ip_address[16];
char file_name[20];
unsigned char record[DATA_SIZE];

int network_printer(char pdfFileName[]);
int fsize(FILE *fp);
void pdfReaderUSB(FILE *printer,char pdfFileName[]);
void pdfReaderNETwork(char pdfFileName[]);

