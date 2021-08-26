#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

void daemonize(const char *cmd) {
    int fd0, fd1, fd2;
    unsigned long int i;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;

    umask(0);

    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
        exit(1);
    }

    if ((pid = fork()) < 0) {
        exit(1);
    } else if (pid != 0) {
        exit(0);
    }
    setsid();

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0) {
        exit(1);
    }
    if ((pid = fork()) < 0) {
        exit(1);
    } else if (pid != 0) {
        exit(0);
    }

    if (chdir("/") < 0) {
        exit(1);
    }

    if (rl.rlim_max == RLIM_INFINITY) {
        rl.rlim_max = 1024;
    }
    for (i = 0; i < rl.rlim_max; i++) {
        close(i);
    }

    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);
    openlog(cmd, LOG_CONS, LOG_DAEMON);
    if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
        exit(1);
    }
}
