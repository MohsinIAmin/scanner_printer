#include <stdio.h>
#include <stdlib.h>

int is_installed() {
    int ret = 0;
    ret = system("which scanimage > /dev/null");
    if (ret == 0) {
        ret = system("which convert > /dev/null");
    }
    if (ret == 0) {
        return ret;
    }
    printf("Please install sane-util imagemagick\n\n");
    printf("\t$sudo apt install sane-util imagemagick\n");
    printf("\n\t\tor\n\n");
    printf("\t#apt install sane-util imagemagick\n");
    return -1;
}