#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "print.h"

#define MAXCFGLINE 512
#define MAXKWLEN 16
#define MAXFMTLEN 16

#define MAXSLEEP 128

int initserver(int type, const struct sockaddr *addr, socklen_t alen, int qlen) {
    int fd, err;
    int reuse = 1;

    if ((fd = socket(addr->sa_family, type, 0)) < 0)
        return (-1);
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
        goto errout;
    if (bind(fd, addr, alen) < 0)
        goto errout;
    if (type == SOCK_STREAM || type == SOCK_SEQPACKET)
        if (listen(fd, qlen) < 0)
            goto errout;
    return (fd);

errout:
    err = errno;
    close(fd);
    errno = err;
    return (-1);
}

ssize_t writen(int fd, const void *ptr, size_t n) {
    size_t nleft;
    ssize_t nwritten;

    nleft = n;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) < 0) {
            if (nleft == n)
                return (-1);
            else
                break;
        } else if (nwritten == 0) {
            break;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return (n - nleft);
}

ssize_t readn(int fd, void *ptr, size_t n) {
    size_t nleft;
    ssize_t nread;

    nleft = n;
    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0) {
            if (nleft == n)
                return (-1);
            else
                break;
        } else if (nread == 0) {
            break;
        }
        nleft -= nread;
        ptr += nread;
    }
    return (n - nleft);
}

int connect_retry(int domain, int type, int protocol, const struct sockaddr *addr, socklen_t alen) {
    int numsec, fd;
    for (numsec = 1; numsec <= MAXSLEEP; numsec <<= 1) {
        if ((fd = socket(domain, type, protocol)) < 0)
            return (-1);
        if (connect(fd, addr, alen) == 0) {
            return (fd);
        }
        close(fd);
        if (numsec <= MAXSLEEP / 2)
            sleep(numsec);
    }
    return (-1);
}

int getaddrlist(const char *host, const char *service, struct addrinfo **ailistpp) {
    int err;
    struct addrinfo hint;

    hint.ai_flags = AI_CANONNAME;
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = 0;
    hint.ai_addrlen = 0;
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;
    err = getaddrinfo(host, service, &hint, ailistpp);
    return (err);
}

static char *scan_configfile(char *keyword) {
    int n, match;
    FILE *fp;
    char keybuf[MAXKWLEN], pattern[MAXFMTLEN];
    char line[MAXCFGLINE];
    static char valbuf[MAXCFGLINE];

    if ((fp = fopen(CONFIG_FILE, "r")) == NULL) {
        printf("can't open %s", CONFIG_FILE);
        exit(1);
    }
    sprintf(pattern, "%%%ds %%%ds", MAXKWLEN - 1, MAXCFGLINE - 1);
    match = 0;
    while (fgets(line, MAXCFGLINE, fp) != NULL) {
        n = sscanf(line, pattern, keybuf, valbuf);
        if (n == 2 && strcmp(keyword, keybuf) == 0) {
            match = 1;
            break;
        }
    }
    fclose(fp);
    if (match != 0)
        return (valbuf);
    else
        return (NULL);
}

char *get_printserver(void) {
    return (scan_configfile("printserver"));
}

struct addrinfo *get_printaddr(void) {
    int err;
    char *p;
    struct addrinfo *ailist;

    if ((p = scan_configfile("printer")) != NULL) {
        if ((err = getaddrlist(p, "ipp", &ailist)) != 0) {
            return (NULL);
        }
        return (ailist);
    }
    return (NULL);
}

ssize_t treadn(int fd, void *buf, size_t nbytes, unsigned int timout) {
    size_t nleft;
    ssize_t nread;

    nleft = nbytes;
    while (nleft > 0) {
        if ((nread = tread(fd, buf, nleft, timout)) < 0) {
            if (nleft == nbytes) {
                return (-1);
            } else {
                break;
            }
        } else if (nread == 0) {
            break;
        }
        nleft -= nread;
        buf += nread;
    }
    return (nbytes - nleft);
}

ssize_t tread(int fd, void *buf, size_t nbytes, unsigned int timout) {
    int nfds;
    fd_set readfds;
    struct timeval tv;

    tv.tv_sec = timout;
    tv.tv_usec = 0;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    nfds = select(fd + 1, &readfds, NULL, NULL, &tv);
    if (nfds <= 0) {
        if (nfds == 0)
            errno = ETIME;
        return (-1);
    }
    return (read(fd, buf, nbytes));
}
