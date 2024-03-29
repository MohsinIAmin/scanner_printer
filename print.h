#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>

#define CONFIG_FILE "/etc/printer.conf"
#define SPOOLDIR "/var/spool/printer"
#define JOBFILE "jobno"
#define DATADIR "data"
#define REQDIR "reqs"

#define LPNAME "lp"

#define FILENMSZ 512
#define FILEPERM (S_IRUSR | S_IWUSR)

#define USERNM_MAX 64
#define JOBNM_MAX 256
#define MSGLEN_MAX 512

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif

#define IPP_PORT 631
#define QLEN 10

#define IBUFSZ 512   /* IPP header buffer size */
#define HBUFSZ 512   /* HTTP header buffer size */
#define IOBUFSZ 8192 /* data buffer size */

extern int getaddrlist(const char *, const char *, struct addrinfo **);
extern char *get_printserver(void);
extern struct addrinfo *get_printaddr(void);
extern ssize_t tread(int, void *, size_t, unsigned int);
extern ssize_t treadn(int, void *, size_t, unsigned int);
extern int connect_retry(int, int, int, const struct sockaddr *, socklen_t);
extern int initserver(int, const struct sockaddr *, socklen_t, int);

extern ssize_t readn(int, void *, size_t);
extern ssize_t writen(int, const void *, size_t);

/*
 * Structure describing a print request.
 */
struct printreq {
    uint32_t size;           /* size in bytes */
    uint32_t flags;          /* see below */
    char usernm[USERNM_MAX]; /* user's name */
    char jobnm[JOBNM_MAX];   /* job's name */
};

/*
 * Request flags.
 */
#define PR_TEXT 0x01 /* treat file as plain text */

/*
 * The response from the spooling daemon to the print command.
 */
struct printresp {
    uint32_t retcode;     /* 0=success, !0=error code */
    uint32_t jobid;       /* job ID */
    char msg[MSGLEN_MAX]; /* error message */
};
