#include <stdio.h>
#include <stdlib.h>

int server_to_printer(char buffer[8192]) {
    FILE *fp = fopen("printing.pdf", "w");
    fwrite(buffer, 8192, 1, fp);
    fclose(fp);
    return 0;
}