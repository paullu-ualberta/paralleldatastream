/* unpcode.c, Version 0.5
 *
 * The following functions are obtained from  http://www.unpbook.com
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>          //inet_addr
#include <netdb.h>
#include <fcntl.h>
#include <sys/time.h>           //FD_SET, FD_ISSET, FD_ZERO macros
#include <time.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

/* The two following functions are obtained from  http://www.unpbook.com */

ssize_t                         /* Read "n" bytes from a descriptor. */
readn(int fd, void *vptr, size_t n)
{
        size_t nleft;
        ssize_t nread;
        char *ptr;

        ptr = vptr;
        nleft = n;
        while (nleft > 0) {
                if ((nread = read(fd, ptr, nleft)) < 0) {
                        if (errno == EINTR)
                                nread = 0;      /* and call read() again */
                        else
                                return (-1);
                } else if (nread == 0)
                        break;  /* EOF */

                nleft -= nread;
                ptr += nread;
        }
        return (n - nleft);     /* return >= 0 */
}

ssize_t                         /* Write "n" bytes to a descriptor. */
writen(int fd, void *vptr, size_t n)
{
        size_t nleft; 
        ssize_t nwritten;
        const char *ptr;

        ptr = vptr;             /* can't do pointer arithmetic on void* */
        nleft = n;
        while (nleft > 0) {
                if ((nwritten = write(fd, ptr, nleft)) <= 0)
                        return (nwritten);      /* error */

                nleft -= nwritten;
                ptr += nwritten;
        }
        return (n);
}

void set_fl(int fd, int flags)
{                               /* flags are file status flags to turn on */
        int val;

        if ((val = fcntl(fd, F_GETFL, 0)) < 0)
                printf("fcntl F_GETFL error \n");

        val |= flags;           /* turn on flags */

        if (fcntl(fd, F_SETFL, val) < 0)
                printf("fcntl F_SETFL error \n");
}

void clr_fl(int fd, int flags)
{                               /* flags are file status flags to turn off */
        int val;

        if ((val = fcntl(fd, F_GETFL, 0)) < 0)
                printf("fcntl F_GETFL error \n");

        val &= ~flags;          /* turn flags off */

        if (fcntl(fd, F_SETFL, val) < 0)
                printf("fcntl F_SETFL error \n");
}

char *find_command_path(char *command, char *command_path)
{

        strcpy(command_path, "");
        char *which_command = (char *)malloc((30) * sizeof(char));
        strcpy(which_command, "which ");
        strcat(which_command, command);
        FILE *fp = popen(which_command, "r");
        char tmp[10];
        while (fgets(tmp, sizeof(tmp), fp) != 0)
                strcat(command_path, tmp);
        pclose(fp);
        command_path[strlen(command_path) - 1] = '\0';
        return (0);
}
