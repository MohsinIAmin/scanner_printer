#include "printer_data.h"

int main(int argc, char *argv[])
{
    int status = 0;
    if(argc == 2)
    {
        strcpy(file_name,argv[1]);
        FILE *usbPrinter = fopen("/dev/usb/lp0","wb");
        if(usbPrinter==NULL) {
            printf("No USB printer\n");
            return 0;
        }
        pdfReaderUSB(usbPrinter,file_name);
    }
    if(argc == 4)
    {
        strcpy(ip_address,argv[1]);
        port = atoi(argv[2]);
        strcpy(file_name,argv[3]);
	//pdfReaderNETwork(file_name);
        status = network_printer(file_name);
    }

    //printf("%d\n",argc);
    //printf("%s\n",ip_address);
    //printf("%d\n",port);
    //printf("%s\n",file_name);
    return 0;
}
