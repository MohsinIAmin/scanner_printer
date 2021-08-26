#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "print.h"

void submit_file(int fd, int socket_fd, const char *filename, size_t nbyte) {
    int nr, nw;
    struct printreq req;
    struct printresp res;
    char buf[IOBUFSZ];

    req.size = htonl(nbyte);
    req.flags = 1;
    strcpy(req.usernm, "printing");
    strcpy(req.jobnm, "printing.pdf");

    nw = writen(socket_fd, &req, sizeof(struct printreq));

    while ((nr = read(fd, buf, IOBUFSZ)) != 0) {
        nw = writen(socket_fd, buf, nr);
        if (nw != nr) {
            exit(1);
        }
    }

    if ((nr = readn(socket_fd, &res, sizeof(struct printresp))) != sizeof(struct printresp)) {
        printf("Cannot read from printer\n");
    }

    if (res.retcode != 0) {
        printf("rejected: %s\n", res.msg);
        exit(1);
    } else {
        printf("job ID %ld\n", (long)ntohl(res.jobid));
    }

    return;
}

int print_to_printer(char *filename) {
    int fd, sfd, err;
    struct stat sbuf;
    char *host;
    struct addrinfo *ailist, *aip;

    fd = open(filename, O_RDONLY);
    fstat(fd, &sbuf);

    if ((host = get_printserver()) == NULL) {
        printf("Error on find printer\n");
        return -1;
    }
    if ((err = getaddrlist(host, "print", &ailist)) != 0) {
        printf("Error cannot find printer\n");
        return -1;
    }
    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        if ((sfd = connect_retry(AF_INET, SOCK_STREAM, 0, aip->ai_addr, aip->ai_addrlen)) < 0) {
            err = errno;
        } else {
            submit_file(fd, sfd, filename, sbuf.st_size);
            return 0;
        }
    }
    printf("Cannot print\n");
    return -1;
}

int server_to_printer(char buffer[8192]) {
    FILE *fp = fopen("printing.pdf", "w");
    fwrite(buffer, 8192, 1, fp);
    fclose(fp);
    // return print_to_printer("printing.pdf");
    return 0;
}
