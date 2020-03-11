#include "printer_data.h"

int network_printer(char pdfFileName[])
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        //printf("Cannot creat socket\n");
        return -10;
    }
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if(inet_pton(AF_INET, ip_address , &serv_addr.sin_addr)<=0)
    {
        //printf("Invalid address/ Address not supported");
        return -20;
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        //printf("Connection Failed");
        return -30;
    }
    FILE *image = fopen(pdfFileName,"rb");
    long int size = fsize(image);
    //printf("%li\n",size);
    fread(record,size,1,image);
    send(sock, record , size, 0 );
    return 0;
}

