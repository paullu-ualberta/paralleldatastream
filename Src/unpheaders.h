#ifndef UNPHEADERS_H
#define UNPHEADERS_H

/* unpheaders.h, Version 0.5
 *
 * The two following functions are obtained from  http://www.unpbook.com
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> //inet_addr
#include <netdb.h>
#include <fcntl.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <time.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/un.h>
#include <stdint.h>

extern ssize_t						/* Read "n" bytes from a descriptor. */
readn(int fd, void *vptr, size_t n);


extern ssize_t						/* Write "n" bytes to a descriptor. */
writen(int fd, void *vptr, size_t n);

extern void set_fl(int fd, int flags);
extern void clr_fl(int fd, int flags);
extern char *find_command_path(char *command, char *command_path);

#endif /* UNPHEADERS_H */
