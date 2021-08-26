#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ipp.h"
#include "print.h"
/*
 * These are for the HTTP response from the printer.
 */
#define HTTP_INFO(x) ((x) >= 100 && (x) <= 199)
#define HTTP_SUCCESS(x) ((x) >= 200 && (x) <= 299)

/*
 * Describes a print job.
 */
struct job {
    struct job *next;    /* next in list */
    struct job *prev;    /* previous in list */
    int32_t jobid;       /* job ID */
    struct printreq req; /* copy of print request */
};

/*
 * Describes a thread processing a client request.
 */
struct worker_thread {
    struct worker_thread *next; /* next in list */
    struct worker_thread *prev; /* previous in list */
    pthread_t tid;              /* thread ID */
    int sockfd;                 /* socket */
};

/*
 * Printer-related stuff.
 */
struct addrinfo *printer;
char *printer_name;
pthread_mutex_t configlock = PTHREAD_MUTEX_INITIALIZER;
int reread;

/*
 * Thread-related stuff.
 */
struct worker_thread *workers;
pthread_mutex_t workerlock = PTHREAD_MUTEX_INITIALIZER;
sigset_t mask;

/*
 * Job-related stuff.
 */
struct job *jobhead, *jobtail;
int jobfd;
int32_t nextjob;
pthread_mutex_t joblock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t jobwait = PTHREAD_COND_INITIALIZER;

void daemonize(const char *);

int lock_reg(int fd, off_t offset, int whence, off_t len) {
    struct flock lock;

    lock.l_type = F_WRLCK;
    lock.l_start = offset;
    lock.l_whence = whence;
    lock.l_len = len;

    return (fcntl(fd, F_SETLK, &lock));
}

void init_request(void) {
    int n;
    char name[FILENMSZ];

    sprintf(name, "%s/%s", SPOOLDIR, JOBFILE);
    jobfd = open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (lock_reg(jobfd, 0, SEEK_SET, 0) < 0) {
        exit(1);
    }

    if ((n = read(jobfd, name, FILENMSZ)) < 0) {
        exit(1);
    }
    if (n == 0) {
        nextjob = 1;
    } else {
        nextjob = atol(name);
    }
}

void init_printer(void) {
    printer = get_printaddr();
    if (printer == NULL) {
        exit(1);
    }
    printer_name = printer->ai_canonname;
    if (printer_name == NULL) {
        printer_name = "printer";
    }
}

void kill_workers(void) {
    struct worker_thread *wtp;

    pthread_mutex_lock(&workerlock);
    for (wtp = workers; wtp != NULL; wtp = wtp->next) {
        pthread_cancel(wtp->tid);
    }
    pthread_mutex_unlock(&workerlock);
}

void *signal_thread(void *arg) {
    int err, signo;
    for (;;) {
        err = sigwait(&mask, &signo);
        if (err != 0) {
            exit(1);
        }
        switch (signo) {
            case SIGHUP:
                pthread_mutex_lock(&configlock);
                reread = 1;
                pthread_mutex_unlock(&configlock);
                break;

            case SIGTERM:
                kill_workers();
                exit(0);

            default:
                kill_workers();
                exit(1);
        }
    }
}

void client_cleanup(void *arg) {
    struct worker_thread *wtp;
    pthread_t tid;

    tid = (pthread_t)((long)arg);
    pthread_mutex_lock(&workerlock);
    for (wtp = workers; wtp != NULL; wtp = wtp->next) {
        if (wtp->tid == tid) {
            if (wtp->next != NULL) {
                wtp->next->prev = wtp->prev;
            }
            if (wtp->prev != NULL) {
                wtp->prev->next = wtp->next;
            } else {
                workers = wtp->next;
            }
            break;
        }
    }
    pthread_mutex_unlock(&workerlock);
    if (wtp != NULL) {
        close(wtp->sockfd);
        free(wtp);
    }
}

void add_worker(pthread_t tid, int sockfd) {
    struct worker_thread *wtp;

    if ((wtp = malloc(sizeof(struct worker_thread))) == NULL) {
        pthread_exit((void *)1);
    }

    wtp->tid = tid;
    wtp->sockfd = sockfd;

    pthread_mutex_lock(&workerlock);
    wtp->prev = NULL;
    wtp->next = workers;
    if (workers == NULL) {
        workers = wtp;
    } else {
        workers->prev = wtp;
    }
    pthread_mutex_unlock(&workerlock);
}

void add_job(struct printreq *reqp, int32_t jobid) {
    struct job *jp;

    if ((jp = malloc(sizeof(struct job))) == NULL) {
        exit(1);
    }
    memcpy(&jp->req, reqp, sizeof(struct printreq));

    jp->jobid = jobid;
    jp->next = NULL;

    pthread_mutex_lock(&joblock);

    jp->prev = jobtail;

    if (jobtail == NULL) {
        jobhead = jp;
    } else {
        jobtail->next = jp;
    }

    jobtail = jp;

    pthread_mutex_unlock(&joblock);
    pthread_cond_signal(&jobwait);
}

int32_t get_newjobno(void) {
    int32_t jobid;

    pthread_mutex_lock(&joblock);

    jobid = nextjob++;
    if (nextjob <= 0) {
        nextjob = 1;
    }

    pthread_mutex_unlock(&joblock);

    return (jobid);
}

void *client_thread(void *arg) {
    int n, fd, sockfd, nr, nw, first;
    int32_t jobid;
    pthread_t tid;
    struct printreq req;
    struct printresp res;
    char name[FILENMSZ];
    char buf[IOBUFSZ];

    tid = pthread_self();
    pthread_cleanup_push(client_cleanup, (void *)((long)tid));
    sockfd = (long)arg;
    add_worker(tid, sockfd);

    if ((n = treadn(sockfd, &req, sizeof(struct printreq), 10)) != sizeof(struct printreq)) {
        res.jobid = 0;
        if (n < 0) {
            res.retcode = htonl(errno);
        } else {
            res.retcode = htonl(EIO);
        }

        strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
        writen(sockfd, &res, sizeof(struct printresp));

        pthread_exit((void *)1);
    }
    req.size = ntohl(req.size);
    req.flags = ntohl(req.flags);

    jobid = get_newjobno();

    sprintf(name, "%s/%s/%d", SPOOLDIR, DATADIR, jobid);

    fd = creat(name, FILEPERM);
    if (fd < 0) {
        res.jobid = 0;
        res.retcode = htonl(errno);

        strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
        writen(sockfd, &res, sizeof(struct printresp));
        pthread_exit((void *)1);
    }

    first = 1;
    while ((nr = tread(sockfd, buf, IOBUFSZ, 20)) > 0) {
        if (first) {
            first = 0;
            if (strncmp(buf, "%!PS", 4) != 0)
                req.flags |= PR_TEXT;
        }
        nw = write(fd, buf, nr);
        if (nw != nr) {
            res.jobid = 0;
            if (nw < 0) {
                res.retcode = htonl(errno);
            } else {
                res.retcode = htonl(EIO);
            }
            close(fd);
            strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
            writen(sockfd, &res, sizeof(struct printresp));
            unlink(name);
            pthread_exit((void *)1);
        }
    }
    close(fd);

    sprintf(name, "%s/%s/%d", SPOOLDIR, REQDIR, jobid);
    fd = creat(name, FILEPERM);
    if (fd < 0) {
        res.jobid = 0;
        res.retcode = htonl(errno);

        strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
        writen(sockfd, &res, sizeof(struct printresp));
        sprintf(name, "%s/%s/%d", SPOOLDIR, DATADIR, jobid);
        unlink(name);

        pthread_exit((void *)1);
    }

    nw = write(fd, &req, sizeof(struct printreq));
    if (nw != sizeof(struct printreq)) {
        res.jobid = 0;
        if (nw < 0) {
            res.retcode = htonl(errno);
        } else {
            res.retcode = htonl(EIO);
        }
        close(fd);

        strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
        writen(sockfd, &res, sizeof(struct printresp));
        unlink(name);
        sprintf(name, "%s/%s/%d", SPOOLDIR, DATADIR, jobid);
        unlink(name);

        pthread_exit((void *)1);
    }
    close(fd);

    res.retcode = 0;
    res.jobid = htonl(jobid);
    sprintf(res.msg, "request ID %d", jobid);
    writen(sockfd, &res, sizeof(struct printresp));

    add_job(&req, jobid);
    pthread_cleanup_pop(1);
    return ((void *)0);
}

void update_jobno(void) {
    char buf[32];

    if (lseek(jobfd, 0, SEEK_SET) == -1) {
        exit(1);
    }
    sprintf(buf, "%d", nextjob);
    if (write(jobfd, buf, strlen(buf)) < 0) {
        exit(1);
    }
}

void remove_job(struct job *target) {
    if (target->next != NULL) {
        target->next->prev = target->prev;
    } else {
        jobtail = target->prev;
    }
    if (target->prev != NULL) {
        target->prev->next = target->next;
    } else {
        jobhead = target->next;
    }
}

char *add_option(char *cp, int tag, char *optname, char *optval) {
    int n;
    union {
        int16_t s;
        char c[2];
    } u;

    *cp++ = tag;
    n = strlen(optname);
    u.s = htons(n);
    *cp++ = u.c[0];
    *cp++ = u.c[1];
    strcpy(cp, optname);
    cp += n;
    n = strlen(optval);
    u.s = htons(n);
    *cp++ = u.c[0];
    *cp++ = u.c[1];
    strcpy(cp, optval);
    return (cp + n);
}

void replace_job(struct job *jp) {
    pthread_mutex_lock(&joblock);

    jp->prev = NULL;
    jp->next = jobhead;
    if (jobhead == NULL) {
        jobtail = jp;
    } else {
        jobhead->prev = jp;
    }
    jobhead = jp;

    pthread_mutex_unlock(&joblock);
}

ssize_t readmore(int sockfd, char **bpp, int off, int *bszp) {
    ssize_t nr;
    char *bp = *bpp;
    int bsz = *bszp;

    if (off >= bsz) {
        bsz += IOBUFSZ;
        if ((bp = realloc(*bpp, bsz)) == NULL) {
            exit(1);
        }
        *bszp = bsz;
        *bpp = bp;
    }
    if ((nr = tread(sockfd, &bp[off], bsz - off, 1)) > 0)
        return (off + nr);
    else
        return (-1);
}

int printer_status(int sfd, struct job *jp) {
    int i, success, code, len, found, bufsz, datsz;
    int32_t jobid;
    ssize_t nr;
    char *bp, *cp, *statcode, *contentlen;
    struct ipp_hdr *hp;

    /*
	 * Read the HTTP header followed by the IPP response header.
	 * They can be returned in multiple read attempts.  Use the
	 * Content-Length specifier to determine how much to read.
	 */
    success = 0;
    bufsz = IOBUFSZ;
    if ((bp = malloc(IOBUFSZ)) == NULL) {
    }

    while ((nr = tread(sfd, bp, bufsz, 5)) > 0) {
        cp = bp + 8;
        datsz = nr;
        while (isspace((int)*cp)) {
            cp++;
        }
        statcode = cp;
        while (isdigit((int)*cp)) {
            cp++;
        }
        if (cp == statcode) {
        } else {
            *cp++ = '\0';
            while (*cp != '\r' && *cp != '\n') {
                cp++;
            }
            *cp = '\0';
            code = atoi(statcode);
            if (HTTP_INFO(code))
                continue;
            if (!HTTP_SUCCESS(code)) {
                bp[datsz] = '\0';
                break;
            }

            i = cp - bp;
            for (;;) {
                while (*cp != 'C' && *cp != 'c' && i < datsz) {
                    cp++;
                    i++;
                }
                if (i >= datsz) {
                    if ((nr = readmore(sfd, &bp, i, &bufsz)) < 0) {
                        goto out;
                    } else {
                        cp = &bp[i];
                        datsz += nr;
                    }
                }

                if (strncasecmp(cp, "Content-Length:", 15) == 0) {
                    cp += 15;
                    while (isspace((int)*cp)) {
                        cp++;
                    }
                    contentlen = cp;
                    while (isdigit((int)*cp)) {
                        cp++;
                    }
                    *cp++ = '\0';
                    i = cp - bp;
                    len = atoi(contentlen);
                    break;
                } else {
                    cp++;
                    i++;
                }
            }
            if (i >= datsz) {
                if ((nr = readmore(sfd, &bp, i, &bufsz)) < 0) {
                    goto out;
                } else {
                    cp = &bp[i];
                    datsz += nr;
                }
            }

            found = 0;
            while (!found) {
                while (i < datsz - 2) {
                    if (*cp == '\n' && *(cp + 1) == '\r' &&
                        *(cp + 2) == '\n') {
                        found = 1;
                        cp += 3;
                        i += 3;
                        break;
                    }
                    cp++;
                    i++;
                }
                if (i >= datsz) {
                    if ((nr = readmore(sfd, &bp, i, &bufsz)) < 0) {
                        goto out;
                    } else {
                        cp = &bp[i];
                        datsz += nr;
                    }
                }
            }

            if (datsz - i < len) {
                if ((nr = readmore(sfd, &bp, i, &bufsz)) < 0) {
                    goto out;
                } else {
                    cp = &bp[i];
                    datsz += nr;
                }
            }

            hp = (struct ipp_hdr *)cp;
            i = ntohs(hp->status);
            jobid = ntohl(hp->request_id);

            if (jobid != jp->jobid) {
                break;
            }

            if (STATCLASS_OK(i))
                success = 1;
            break;
        }
    }

out:
    free(bp);
    if (nr < 0) {
    }
    return (success);
}

void *printer_thread(void *arg) {
    struct job *jp;
    int hlen, ilen, sockfd, fd, nr, nw, extra;
    char *icp, *hcp, *p;
    struct ipp_hdr *hp;
    struct stat sbuf;
    struct iovec iov[2];
    char name[FILENMSZ];
    char hbuf[HBUFSZ];
    char ibuf[IBUFSZ];
    char buf[IOBUFSZ];
    char str[64];
    struct timespec ts = {60, 0}; /* 1 minute */

    for (;;) {
        pthread_mutex_lock(&joblock);

        while (jobhead == NULL) {
            pthread_cond_wait(&jobwait, &joblock);
        }
        remove_job(jp = jobhead);

        pthread_mutex_unlock(&joblock);

        update_jobno();

        pthread_mutex_lock(&configlock);
        if (reread) {
            freeaddrinfo(printer);
            printer = NULL;
            printer_name = NULL;
            reread = 0;
            pthread_mutex_unlock(&configlock);
            init_printer();
        } else {
            pthread_mutex_unlock(&configlock);
        }

        /*
		 * Send job to printer.
		 */
        sprintf(name, "%s/%s/%d", SPOOLDIR, DATADIR, jp->jobid);
        if ((fd = open(name, O_RDONLY)) < 0) {
            free(jp);
            continue;
        }
        if (fstat(fd, &sbuf) < 0) {
            free(jp);
            close(fd);
            continue;
        }
        if ((sockfd = connect_retry(AF_INET, SOCK_STREAM, 0, printer->ai_addr, printer->ai_addrlen)) < 0) {
            goto defer;
        }

        /*
		 * Set up the IPP header.
		 */
        icp = ibuf;
        hp = (struct ipp_hdr *)icp;
        hp->major_version = 1;
        hp->minor_version = 1;
        hp->operation = htons(OP_PRINT_JOB);
        hp->request_id = htonl(jp->jobid);

        icp += offsetof(struct ipp_hdr, attr_group);
        *icp++ = TAG_OPERATION_ATTR;

        icp = add_option(icp, TAG_CHARSET, "attributes-charset", "utf-8");
        icp = add_option(icp, TAG_NATULANG, "attributes-natural-language", "en-us");

        sprintf(str, "http://%s/ipp", printer_name);

        icp = add_option(icp, TAG_URI, "printer-uri", str);
        icp = add_option(icp, TAG_NAMEWOLANG, "requesting-user-name", jp->req.usernm);
        icp = add_option(icp, TAG_NAMEWOLANG, "job-name", jp->req.jobnm);

        if (jp->req.flags & PR_TEXT) {
            p = "text/plain";
            extra = 1;
        } else {
            p = "application/postscript";
            extra = 0;
        }

        icp = add_option(icp, TAG_MIMETYPE, "document-format", p);
        *icp++ = TAG_END_OF_ATTR;

        ilen = icp - ibuf;

        /*
		 * Set up the HTTP header.
		 */
        hcp = hbuf;
        sprintf(hcp, "POST /ipp HTTP/1.1\r\n");
        hcp += strlen(hcp);
        sprintf(hcp, "Content-Length: %ld\r\n", (long)sbuf.st_size + ilen + extra);
        hcp += strlen(hcp);
        strcpy(hcp, "Content-Type: application/ipp\r\n");
        hcp += strlen(hcp);
        sprintf(hcp, "Host: %s:%d\r\n", printer_name, IPP_PORT);
        hcp += strlen(hcp);
        *hcp++ = '\r';
        *hcp++ = '\n';
        hlen = hcp - hbuf;

        /*
		 * Write the headers first.  Then send the file.
		 */
        iov[0].iov_base = hbuf;
        iov[0].iov_len = hlen;
        iov[1].iov_base = ibuf;
        iov[1].iov_len = ilen;
        if (write(sockfd, iov, 2) != hlen + ilen) {
            goto defer;
        }

        if (jp->req.flags & PR_TEXT) {
            if (write(sockfd, "\b", 1) != 1) {
                goto defer;
            }
        }

        while ((nr = read(fd, buf, IOBUFSZ)) > 0) {
            if ((nw = writen(sockfd, buf, nr)) != nr) {
                if (nw < 0) {
                } else {
                }
                goto defer;
            }
        }
        if (nr < 0) {
            goto defer;
        }

        /*
		 * Read the response from the printer.
		 */
        if (printer_status(sockfd, jp)) {
            unlink(name);
            sprintf(name, "%s/%s/%d", SPOOLDIR, REQDIR, jp->jobid);
            unlink(name);
            free(jp);
            jp = NULL;
        }
    defer:
        close(fd);
        if (sockfd >= 0)
            close(sockfd);
        if (jp != NULL) {
            replace_job(jp);
            nanosleep(&ts, NULL);
        }
    }
}

void build_qonstart(void) {
    int fd, nr;
    int32_t jobid;
    DIR *dirp;
    struct dirent *entp;
    struct printreq req;
    char dname[FILENMSZ], fname[FILENMSZ];

    sprintf(dname, "%s/%s", SPOOLDIR, REQDIR);
    if ((dirp = opendir(dname)) == NULL) {
        return;
    }
    while ((entp = readdir(dirp)) != NULL) {
        if (strcmp(entp->d_name, ".") == 0 || strcmp(entp->d_name, "..") == 0) {
            continue;
        }

        sprintf(fname, "%s/%s/%s", SPOOLDIR, REQDIR, entp->d_name);
        if ((fd = open(fname, O_RDONLY)) < 0) {
            continue;
        }

        nr = read(fd, &req, sizeof(struct printreq));
        if (nr != sizeof(struct printreq)) {
            close(fd);

            unlink(fname);

            sprintf(fname, "%s/%s/%s", SPOOLDIR, DATADIR, entp->d_name);

            unlink(fname);

            continue;
        }

        jobid = atol(entp->d_name);
        add_job(&req, jobid);
    }
    closedir(dirp);
}

int main(int argc, char *argv[]) {
    pthread_t tid;
    struct addrinfo *ailist, *aip;
    int sockfd, err, i, n, maxfd;
    char *host;
    fd_set rendezvous, rset;
    struct sigaction sa;
    struct passwd *pwdp;

    if (argc != 1) {
        printf("Use: printd\n");
    }

    daemonize("printd");

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) < 0) {
        exit(1);
    }
    sigemptyset(&mask);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGTERM);

    if ((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0) {
        exit(1);
    }

    n = sysconf(_SC_HOST_NAME_MAX);
    if (n < 0) {
        n = HOST_NAME_MAX;
    }
    if ((host = malloc(n)) == NULL) {
        exit(1);
    }
    if (gethostname(host, n) < 0) {
        exit(1);
    }
    if ((err = getaddrlist(host, "print", &ailist)) != 0) {
        exit(1);
    }

    FD_ZERO(&rendezvous);
    maxfd = -1;

    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        if ((sockfd = initserver(SOCK_STREAM, aip->ai_addr, aip->ai_addrlen, QLEN)) >= 0) {
            FD_SET(sockfd, &rendezvous);
            if (sockfd > maxfd) {
                maxfd = sockfd;
            }
        }
    }

    if (maxfd == -1) {
        exit(1);
    }

    pwdp = getpwnam(LPNAME);
    if (pwdp == NULL) {
        exit(1);
    }
    if (pwdp->pw_uid == 0) {
        exit(1);
    }
    if (setgid(pwdp->pw_gid) < 0 || setuid(pwdp->pw_uid) < 0) {
        exit(1);
    }

    init_request();
    init_printer();

    err = pthread_create(&tid, NULL, printer_thread, NULL);
    if (err == 0) {
        err = pthread_create(&tid, NULL, signal_thread, NULL);
    }
    if (err != 0) {
        exit(1);
    }

    build_qonstart();

    for (;;) {
        rset = rendezvous;
        if (select(maxfd + 1, &rset, NULL, NULL, NULL) < 0) {
            exit(1);
        }
        for (i = 0; i <= maxfd; i++) {
            if (FD_ISSET(i, &rset)) {
                if ((sockfd = accept(i, NULL, NULL)) < 0) {
                }
                pthread_create(&tid, NULL, client_thread, (void *)((long)sockfd));
            }
        }
    }
    exit(1);

    return 0;
}