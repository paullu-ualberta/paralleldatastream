/* PDS_server.c, Version 0.9
 *
 * This copyright header adapted from the Linux 4.1 kernel.
 *
 * Copyright 2017 Nooshin Eghbal, Hamidreza Anvari, Paul Lu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

// Global variables

uint64_t max_out_of_orders = 50000000;
uint64_t num_of_out_of_orders = 0;
uint64_t seqnum_send = 1;
uint64_t seqnum_rec = 1;
int32_t num_of_streams;
uint32_t block_size;
uint32_t port_forwarding = 0;

char **receive_command_args(FILE *fp_log){

    uint32_t num_args, arg_size, i, j;
    int32_t rd;

    if((rd = readn(STDIN_FILENO, &num_args, 4)) < 4){
        fwrite("less than 4 \n", 1, strlen("less than 4 \n"), fp_log);
        fflush(fp_log);
        exit(0);
    }

    num_args = ntohl(num_args);
    char **command_args = (char**)malloc((num_args+1) * sizeof(char *));
    char *arg;
    for(i = 0; i<num_args; i++){
        arg_size = 0;
        if((rd = readn(STDIN_FILENO, &arg_size, 4)) < 4){
            fwrite("less than 4 \n", 1, strlen("less than 4 \n"), fp_log);
            fflush(fp_log);
            exit(0);
        }
        arg_size = ntohl(arg_size);
        arg = (char*)malloc((arg_size+1) * sizeof(char));
        
        if((rd = readn(STDIN_FILENO, arg, arg_size)) < arg_size){
            fwrite("less than size \n", 1, strlen("less than size \n"), fp_log);
            fflush(fp_log);
            exit(0);
        }
        
        arg[arg_size] = '\0';
        command_args[i] = arg;
    }
    command_args[num_args] = (char *)0;

    return command_args;
}

int32_t create_TCP_server(FILE *fp_log){

    socklen_t optlen = sizeof(uint32_t);
    uint32_t port = 1234;
    struct sockaddr_in server;
    bzero(&server, sizeof(server));
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    int32_t socket_desc;

    if((socket_desc = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fwrite("socket create failed! \n", 1, strlen("socket create failed! \n"), fp_log);
        fflush(fp_log);
        exit(0);
    }

    int32_t yes = 1;
    if ( setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ){
        fwrite("setsockopt \n", 1, strlen("setsockopt \n"), fp_log);
        fflush(fp_log);
    }

    if (bind(socket_desc, (struct sockaddr*)(&server), sizeof(server)) < 0){
        fwrite("Bind failed! \n", 1, strlen("Bind failed! \n"), fp_log);
        fflush(fp_log);
        exit(0);
    }

    if (listen(socket_desc, 5) == -1){
        fwrite("Listen failed! \n", 1, strlen("Listen failed! \n"), fp_log);
        fflush(fp_log);
        exit(0);
    }

    return socket_desc;
}

int32_t create_SSH_server(FILE *fp_log){

    struct sockaddr_un server;
    memset(&server, 0, sizeof(struct sockaddr));
    server.sun_family = AF_UNIX;
    strncpy (server.sun_path, SRV_SOCK_PATH, sizeof (server.sun_path) - 1);
    if (access(server.sun_path, F_OK) == 0)
        unlink (server.sun_path);
    int32_t socket_desc;

    if((socket_desc = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
        fwrite("socket create failed! \n", 1, strlen("socket create failed! \n"), fp_log);
        fflush(fp_log);
        exit(0);
    }

    if (bind(socket_desc, (struct sockaddr*)(&server), sizeof(server)) < 0){
        fwrite("Bind UNIX failed! \n", 1, strlen("Bind UNIX failed! \n"), fp_log);
        fflush(fp_log);
        exit(0);
    }

    if (listen(socket_desc, 5) == -1){
        fwrite("Listen failed! \n", 1, strlen("Listen failed! \n"), fp_log);
        fflush(fp_log);
        exit(0);
    }

    return socket_desc;
}

void run_server(char *command_path, char **command_args, int32_t *std_in_out_fds, FILE *fp_log){

    pid_t pid;
    int32_t read_fd[2];
    int32_t write_fd[2];
    if (pipe(read_fd)){
        fwrite("Pipe failed! \n", 1, strlen("Pipe failed! \n"), fp_log);
        fflush(fp_log);
        exit(0);
    }
    if (pipe(write_fd)){
        fwrite("Pipe failed! \n", 1, strlen("Pipe failed! \n"), fp_log);
        fflush(fp_log);
        exit(0);
    }

    pid = fork();
    if(pid == 0){     // child code
        dup2(read_fd[1], STDOUT_FILENO); // Redirect stdout into writing end of pipe
        dup2(write_fd[0], STDIN_FILENO); // Redirect reading end of pipe into stdin

        close(read_fd[0]);
        close(read_fd[1]);
        close(write_fd[0]);
        close(write_fd[1]);

        execv(command_path, command_args);
        exit(1);
    }

    close(read_fd[1]); // Don't need writing end of the stdout pipe in parent.
    close(write_fd[0]); // Don't need the reading end of the stdin pipe in the parent

    std_in_out_fds[0] = read_fd[0];
    std_in_out_fds[1] = write_fd[1];

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;

    /*if( setsockopt(std_in_out_fds[0], SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval)) == -1){
            fwrite("setsockopt \n", 1, strlen("setsockopt \n"), fp_log);
            fflush(fp_log);
    }*/

    set_fl(std_in_out_fds[1], O_NONBLOCK);
    set_fl(std_in_out_fds[0], O_NONBLOCK);
}

int32_t write_to_sockets(int32_t *socket_desc_client, int32_t stdin_fd, fd_set soc_tempfds, FILE *fp_log){

    uint32_t i, temp_seq, max_diff = 0;
    int32_t rd, wrt, stream_index;
    unsigned char buffer[block_size];
    socklen_t optlen = sizeof(uint32_t);

    stream_index = rand() % num_of_streams;
    for(i=0; i<num_of_streams; i++){
        if (FD_ISSET(socket_desc_client[stream_index], &soc_tempfds)) {
            break;
        }
        else{
            stream_index += 1;
            if (stream_index == num_of_streams)
                stream_index = 0;
        }
    }

    if((rd = read(stdin_fd, buffer+sizeof(uint64_t)+sizeof(uint32_t), block_size-(sizeof(uint64_t)+sizeof(uint32_t)))) < 0){
        fwrite("Read from stdin failed! \n", 1, strlen("Read from stdin failed! \n"), fp_log);
        fflush(fp_log);
        exit(0);
    }

    if (rd > 0){ 
        temp_seq = seqnum_send / 4294967296; // = 2^32 
        temp_seq = htonl(temp_seq);
        memcpy(buffer, &temp_seq, sizeof(uint32_t));
        temp_seq = seqnum_send % 4294967296;
        temp_seq = htonl(temp_seq);
        memcpy(buffer+sizeof(uint32_t), &temp_seq, sizeof(uint32_t));
        rd =  htonl(rd);
        memcpy(buffer+sizeof(uint64_t), &rd, sizeof(uint32_t));
        rd =  ntohl(rd);
        if((wrt = writen(socket_desc_client[stream_index], buffer, rd+sizeof(uint64_t)+sizeof(uint32_t))) < rd+sizeof(uint64_t)+sizeof(uint32_t)){
            fwrite("Send failed! \n", 1, strlen("Send failed! \n"), fp_log);
            fflush(fp_log);
            return -1;
        }
        else{
            seqnum_send += 1;
        }
    }

    else if (num_of_out_of_orders == 0 && rd == 0){
        return 0;
    }

    return 1;
}

int32_t write_to_buffer(int32_t *socket_desc_client, fd_set soc_tempfds, char **buffer_rec, 
                     uint32_t *byte_nums, uint32_t *offset, FILE *fp_log){ 
 
    uint32_t rec_num_bytes, i, rec_buff = 0;
    int32_t rd, max_rec = 0;
    uint32_t temp_seq;
    uint64_t seqnum_temp;
    int32_t stream_index = -1;
    unsigned char buffer[block_size];

    for(i=0; i<num_of_streams; i++){
        if (FD_ISSET(socket_desc_client[i], &soc_tempfds)) {
            ioctl(socket_desc_client[i], FIONREAD, &rec_buff);
            if (rec_buff >= max_rec && rec_buff > sizeof(uint64_t)+sizeof(uint32_t)) {
                stream_index = i;
                max_rec = rec_buff;
            }
        }
    }

    if (stream_index == -1)
        return 0;

    if((rd = readn(socket_desc_client[stream_index], buffer, sizeof(uint64_t)+sizeof(uint32_t))) == sizeof(uint64_t)+sizeof(uint32_t)){

        memcpy(&temp_seq, buffer, sizeof(uint32_t));
        temp_seq = ntohl(temp_seq);
        seqnum_temp = temp_seq * 4294967296;
        memcpy(&temp_seq, buffer+sizeof(uint32_t), sizeof(uint32_t)); 
        temp_seq = ntohl(temp_seq);
        seqnum_temp += temp_seq;
        memcpy(&rec_num_bytes, buffer+sizeof(uint64_t), sizeof(uint32_t));
        rec_num_bytes = ntohl(rec_num_bytes);
 
        if (seqnum_temp >= max_out_of_orders){
            fwrite("Large seq. num! \n", 1, strlen("Large seq. num! \n"), fp_log);
            fflush(fp_log);
            exit(0);
        }
        char *buff_temp = (char*)malloc((rec_num_bytes) * sizeof(unsigned char));

        if ((rd = readn(socket_desc_client[stream_index], buff_temp, rec_num_bytes)) == rec_num_bytes){
            buffer_rec[seqnum_temp%max_out_of_orders] = buff_temp;
            byte_nums[seqnum_temp%max_out_of_orders] = rec_num_bytes;
            offset[seqnum_temp%max_out_of_orders] = 0;
            num_of_out_of_orders += 1;
        }
        else{
            fwrite("Read failed! \n", 1, strlen("Read failed! \n"), fp_log);
            fflush(fp_log);
            exit(0);
        }
    }
    else{
        fwrite("Read header failed! \n", 1, strlen("Read header failed! \n"), fp_log);
        fflush(fp_log);
        exit(0);
    }

    return 1;
}

void write_to_stdout(int32_t stdout_fd, char **buffer_rec, uint32_t *byte_nums, uint32_t *offset, FILE *fp_log){

    int32_t wrt;
    while(num_of_out_of_orders > 0){
        wrt = 0;
        if (byte_nums[seqnum_rec%max_out_of_orders] != 0){
            if((wrt = write(stdout_fd, buffer_rec[seqnum_rec%max_out_of_orders]+offset[seqnum_rec%max_out_of_orders],
                byte_nums[seqnum_rec%max_out_of_orders])) <= 0){
                ;
            }
            else if(wrt < byte_nums[seqnum_rec%max_out_of_orders]){
                offset[seqnum_rec%max_out_of_orders] += wrt;
                byte_nums[seqnum_rec%max_out_of_orders] -= wrt;
            }
            else{
                free(buffer_rec[seqnum_rec%max_out_of_orders]);
                byte_nums[seqnum_rec%max_out_of_orders] = 0;
                offset[seqnum_rec%max_out_of_orders] = 0;
                seqnum_rec += 1;
                num_of_out_of_orders -= 1;
            }
        }
        else if(wrt <= 0)
            break;
    }
}

void send_the_end(int32_t *socket_desc_client, fd_set soc_tempfds, FILE *fp_log){
    
    int32_t rd, wrt, i =0;
    unsigned char buffer[12];
    socklen_t optlen = sizeof(uint32_t);

    while(1){
        if (FD_ISSET(socket_desc_client[i], &soc_tempfds)) {
            seqnum_send = 0;
            memcpy(buffer, &seqnum_send, 8);
            rd = 0;
            memcpy(buffer+8, &rd, 4);
            if((wrt = writen(socket_desc_client[i], buffer, 12)) < 12){
                fwrite("Send failed! \n", 1, strlen("Send failed! \n"), fp_log);
                fflush(fp_log);
                exit(0);
            }
            else
                break;
        }
        i += 1;
        if (i == num_of_streams)
            i = 0;
    }
}

int32_t main(int32_t argc, char **argv){

    FILE *fp_log = fopen("~/log_PDS_server.txt", "w");

    num_of_streams = atoi(argv[1]);
    block_size = atoi(argv[2]);

    uint32_t remote_port;

    if (argc > 4 && argv[4][0] == 'P' && argv[4][1] == 'F'){
        port_forwarding = 1;
        remote_port = atoi(argv[5]);
    }

    int32_t i, write_out, select_out_socket_1, select_out_socket_2, select_out_std;
    socklen_t optlen = sizeof(uint32_t);
    int32_t socket_desc, server_socket;
    int32_t socket_desc_client[num_of_streams];

    char **buffer_rec = malloc(max_out_of_orders * sizeof(char *));
    uint32_t *byte_nums = malloc(max_out_of_orders * sizeof(uint32_t));
    uint32_t *offset = malloc(max_out_of_orders * sizeof(uint32_t)); 

    int32_t std_in_out_fds[2];

    fd_set soc_tempfds, std_tempfd, stdin_activefd, stdout_activefd; 
    fd_set soc_activefds;
    int32_t soc_maxfd = 0;
    FD_ZERO(&soc_activefds);
    FD_ZERO(&stdin_activefd);
    FD_ZERO(&stdout_activefd);  
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    if (strcmp(argv[3], "TCP") == 0)
        socket_desc = create_TCP_server(fp_log);

    else if (strcmp(argv[3], "SSH") == 0)
        socket_desc = create_SSH_server(fp_log);

    // accept connections 

    for(i = 0; i<num_of_streams; i++){
        if ((socket_desc_client[i] = accept(socket_desc, NULL, NULL)) <0){
            fwrite("Accept failed! \n", 1, strlen("Accept failed! \n"), fp_log);
            fflush(fp_log);
            exit(0);
        }
        FD_SET(socket_desc_client[i], &soc_activefds);
        if(socket_desc_client[i] > soc_maxfd)
            soc_maxfd = socket_desc_client[i];

    }

    if (port_forwarding == 0){

        /* Setting up the server */

        char **command_args = receive_command_args(fp_log);
        char *command_path = (char*)malloc((100) * sizeof(char));
        find_command_path(command_args[0], command_path);

        run_server(command_path, command_args, std_in_out_fds, fp_log);

        FD_SET(std_in_out_fds[0], &stdin_activefd);
        FD_SET(std_in_out_fds[1], &stdout_activefd);
    }
    else{
        socklen_t optlen = sizeof(uint32_t);
        struct sockaddr_in server;
        server.sin_addr.s_addr = inet_addr("127.0.0.1");
        server.sin_family = AF_INET;
        server.sin_port = htons(remote_port);

        if((server_socket = socket(AF_INET , SOCK_STREAM , 0)) < 0){
            fwrite("Could not create socket \n", 1, strlen("Could not create socket \n"), fp_log);
            fflush(fp_log);
            exit(0);
        }

        while (connect(server_socket, (struct sockaddr *)&server, sizeof(server)) < 0)
            ;

        set_fl(server_socket, O_NONBLOCK);
        FD_SET(server_socket, &stdin_activefd);
        FD_SET(server_socket, &stdout_activefd);
    }

    while (1) {      

        // read from stdin and write to sockets!
        soc_tempfds = soc_activefds;
        std_tempfd = stdin_activefd;
        if (port_forwarding == 0)
            select_out_std = select(std_in_out_fds[0]+1, &std_tempfd, NULL, NULL, &tv);
        else
            select_out_std = select(server_socket+1, &std_tempfd, NULL, NULL, &tv);
        select_out_socket_1 = select(soc_maxfd+1, NULL, &soc_tempfds, NULL, &tv);

        // there is some data ready on stdin and there is free space on sockets
        if (select_out_socket_1 > 0 && select_out_std > 0){

            if (port_forwarding == 0){
                write_out = write_to_sockets(socket_desc_client, std_in_out_fds[0], soc_tempfds, fp_log);
                if (write_out == 0){
                // Send "The End!" to the client

                    while (1){
                        soc_tempfds = soc_activefds;
                        select_out_socket_1 = select(soc_maxfd+1, NULL, &soc_tempfds, NULL, &tv);
                        if (select_out_socket_1 > 0)
                            break;
                    }

                    send_the_end(socket_desc_client, soc_tempfds, fp_log);
                    break;
                }
                else if (write_out == -1)
                    break;
            }
            else
                write_out = write_to_sockets(socket_desc_client, server_socket, soc_tempfds, fp_log);
        }    

        // read from sockets and write to stdout!
        soc_tempfds = soc_activefds;
        select_out_socket_2 = select(soc_maxfd+1, &soc_tempfds, NULL, NULL, &tv);
 
        // there is some data ready on sockets
        if (select_out_socket_2 > 0){
            if (write_to_buffer(socket_desc_client, soc_tempfds, buffer_rec, byte_nums, offset, fp_log) == 0)
               ;  
        }
        
        if (port_forwarding == 0)
            write_to_stdout(std_in_out_fds[1], buffer_rec, byte_nums, offset, fp_log);
        else
            write_to_stdout(server_socket, buffer_rec, byte_nums, offset, fp_log);

    }//while

    fwrite("\n The End! \n", 1, strlen("\n The End! \n"), fp_log);
    fflush(fp_log);
 
    sleep (3);


    if (port_forwarding == 0){
        // Clear stdout/stdin nonblocking 
        clr_fl(std_in_out_fds[1], O_NONBLOCK);
        clr_fl(std_in_out_fds[0], O_NONBLOCK);
        close(std_in_out_fds[0]); 
        close(std_in_out_fds[1]);
    } 
    else{
        clr_fl(server_socket, O_NONBLOCK);
        close(server_socket);
    }

    free(buffer_rec);
    free(byte_nums);
    free(offset);

    /* close sockets */ 
    for(i = 0; i<num_of_streams; i++){
        if (i == 0)
            close(socket_desc);
        close(socket_desc_client[i]);
    } 
  
    return 0;
}
