#include "printer_data.h"

int fsize(FILE *fp){
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz=ftell(fp);
    fseek(fp,prev,SEEK_SET);
    return sz;
}

void pdfReaderUSB(FILE *printer,char pdfFileName[]) {
    FILE *image = fopen(pdfFileName,"rb");
    if(image==NULL){
	printf("Can not open %s\n",pdfFileName);
	return;
    }
    long int size = fsize(image);
    fread(record,size,1,image);
    strcat(record,"\f\0\f");
    fwrite(record,size,1,printer);
    fclose(image);
}

void pdfReaderNETwork(char pdfFileName[]) {
    FILE *image = fopen(pdfFileName,"rb");
    if(image==NULL){
	printf("Can not open %s\n",pdfFileName);
	return;
    }
    long int size = fsize(image);
    //printf("%li\n",size);
    fread(record,size,1,image);
    strcat(record,"\f\0\f");
    fclose(image);
}
