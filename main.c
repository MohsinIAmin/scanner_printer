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
	if(port == 0){
	    printf("Enter a valid port\n");
	    printf("Please use IP port file_name\n");
	    return 0;
	}
        strcpy(file_name,argv[3]);
	//pdfReaderNETwork(file_name);
        status = network_printer(file_name);
	if(status == -20 ){
	    printf("Please give a valid IP\n");
	    printf("Please use IP port file_name\n");
	}if(status == -30 ){
	    printf("Cannot connect to the printer\n");
	    printf("Please use IP port file_name\n");
	}
    }

    //printf("%d\n",argc);
    //printf("%s\n",ip_address);
    //printf("%d\n",port);
    //printf("%s\n",file_name);
    return 0;
}
