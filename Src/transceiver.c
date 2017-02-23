/* transceiver.c, Version 0.9
 *
 * This copyright header adapted from the Linux 4.1 kernel.
 *
 * Copyright 2017 Nooshin Eghbal, Hamidreza Anvari, Paul Lu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *      
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "unpheaders.h"
#define SRV_SOCK_PATH   "/tmp/ux_socket"

int32_t main(int32_t argc, char **argv)
{

        time_t sec1;
        sec1 = time(NULL);

        struct sockaddr_un server;
        int32_t i, rd1, rd2, wrt, socket_desc, select_out_std, select_out_soc,
            maxfd = 0;
        fd_set tempfd, soc_activefd, stdin_tempfd, stdin_activefd,
            stdout_tempfd, stdout_activefd;;
        uint32_t block_size = atoi(argv[1]);
        uint32_t ID = atoi(argv[2]);
        unsigned char buffer[block_size];
        FILE *fp_log = fopen("./log_PDS_transceiver.txt", "w");

        FD_ZERO(&soc_activefd);
        FD_ZERO(&stdin_activefd);
        FD_SET(STDIN_FILENO, &stdin_activefd);
        FD_ZERO(&stdout_activefd);
        FD_SET(STDOUT_FILENO, &stdout_activefd);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        memset(&server, 0, sizeof(struct sockaddr_un));
        server.sun_family = AF_UNIX;
        strncpy(server.sun_path, SRV_SOCK_PATH, sizeof(server.sun_path) - 1);

        if ((socket_desc = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
                fwrite("Could not create socket \n", 1,
                       strlen("Could not create socket \n"), fp_log);
                fflush(fp_log);
                exit(0);
        }
        FD_SET(socket_desc, &soc_activefd);
        maxfd = socket_desc;

        while (connect(socket_desc, (struct sockaddr *)&server, sizeof(server))
               < 0) {
                ;
        }

        int saved_flags = fcntl(STDOUT_FILENO, F_GETFL);
        fcntl(STDOUT_FILENO, F_SETFL, saved_flags & ~O_NONBLOCK);
        saved_flags = fcntl(STDIN_FILENO, F_GETFL);
        fcntl(STDOUT_FILENO, F_SETFL, saved_flags & ~O_NONBLOCK);

        while (1) {

                // read from stdin and write to socket
                stdin_tempfd = stdin_activefd;
                select_out_std =
                    select(STDIN_FILENO + 1, &stdin_tempfd, NULL, NULL, &tv);
                tempfd = soc_activefd;
                select_out_soc = select(maxfd + 1, NULL, &tempfd, NULL, &tv);

                // there is some data ready on stdin and there is free space on sockets
                if (select_out_soc > 0 && select_out_std > 0) {

                        if ((rd1 = read(STDIN_FILENO, buffer, block_size)) < 0) {
                                fwrite("Receive from stdin \n", 1,
                                       strlen("Receive from stdin \n"), fp_log);
                                fflush(fp_log);
                                exit(0);
                        } else if (rd1 > 0) {

                                if ((wrt =
                                     writen(socket_desc, buffer, rd1)) < rd1) {
                                        fwrite("Send fail \n", 1,
                                               strlen("Send fail \n"), fp_log);
                                        fflush(fp_log);
                                        exit(0);
                                }
                        }
                }
                // read from socket and write to stdout
                stdout_tempfd = stdout_activefd;
                select_out_std =
                    select(STDOUT_FILENO + 1, NULL, &stdout_tempfd, NULL, &tv);
                tempfd = soc_activefd;
                select_out_soc = select(maxfd + 1, &tempfd, NULL, NULL, &tv);

                // there is some data ready on sockets and there is free space on stdout
                if (select_out_soc > 0 && select_out_std > 0) {

                        if ((rd2 = read(socket_desc, buffer, block_size)) < 0) {
                                fwrite("Read failed \n", 1,
                                       strlen("Read failed \n"), fp_log);
                                fflush(fp_log);
                                exit(0);
                        } else if (rd2 > 0) {
                                //sleep(1);
                                if ((wrt =
                                     writen(STDOUT_FILENO, buffer,
                                            rd2)) < rd2) {
                                        fwrite("Write failed \n", 1,
                                               strlen("Write failed \n"),
                                               fp_log);
                                        fflush(fp_log);
                                        exit(0);
                                }
                        }
                }

                if (rd1 == 0)
                        break;

        }                       //End of while(1)

        return 0;
}
