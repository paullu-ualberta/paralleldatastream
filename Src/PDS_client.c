/* PDS_client.c, Version 0.9
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


#define SERVER_SCRIPT_PATH "/home/ubuntu/pds/PDS_server.out"
#define TRANSCEIVER_PATH "/home/ubuntu/pds/transceiver.out"



// Global variables

uint64_t seqnum_send = 1;
uint64_t seqnum_rec = 1;
uint64_t num_of_out_of_orders = 0;
uint64_t max_out_of_orders = 50000000;
int32_t num_of_streams;
uint32_t block_size;
uint32_t port_forwarding = 0;

/*

argv[0] is PDS_client.out; argv[1] is the number of streams; 
argv[2] is the block size; argv[3] is either TCP or SSH;
argv[4] is the IP addr of remote node or -1 if the next two args are username and IP;
argv[5/7] can be PF (Port Forwarding) when the next two args are local and remote port #s.

*/

// The following function is obtained fromhttp://www.binarytides.com/hostname-to-ip-address-c-sockets-linux/
int hostname_to_ip(char* hostname , char* ip){
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
         
    if ( (he = gethostbyname( hostname ) ) == NULL) 
        // get the host info
        return 1;
 
    addr_list = (struct in_addr **) he->h_addr_list;
     
    for(i = 0; addr_list[i] != NULL; i++){
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
     
    return 1;
}

void send_args(int32_t argc, char** argv, int32_t socket_desc, FILE *fp_log){ 

    unsigned char buffer[100];
    uint32_t i, temp, num_of_args;
    int32_t wrt;

    /* Sending the number of parameters */
   if (argv[4][0] == '-')
       num_of_args = argc - 7;
   else
       num_of_args = argc - 5;

    num_of_args = htonl(num_of_args);
    memcpy(buffer, &num_of_args, sizeof(uint32_t));
    if((wrt = write(socket_desc, buffer, sizeof(uint32_t))) < sizeof(uint32_t)){
        fwrite("Send failed \n", 1, strlen("Send failed \n"), fp_log);
        fflush(fp_log);
        exit(0);
    }

    num_of_args = ntohl(num_of_args);
    for(i = argc-num_of_args; i<argc; i++){

        temp = strlen(argv[i]);
        temp = htonl(temp);
        memcpy(buffer, &temp, sizeof(uint32_t));
        if((wrt = writen(socket_desc, buffer, sizeof(uint32_t))) < sizeof(uint32_t)){
            fwrite("Send failed \n", 1, strlen("Send failed \n"), fp_log);
            fflush(fp_log);
            exit(0);
        }

        if((wrt = writen(socket_desc, argv[i], strlen(argv[i]))) < strlen(argv[i])){
            fwrite("Send failed \n", 1, strlen("Send failed \n"), fp_log);
            fflush(fp_log);
            exit(0);
        }
    }
}

char * run_PDS_server(int32_t *read_fd, int32_t *write_fd, int32_t argc, char **argv, FILE *fp_log){

    int32_t num_args;
    char *IP = (char *)malloc(100 * sizeof(char));

    if (port_forwarding == 1)
            num_args = 8;
        else 
            num_args = 6;

    char **command_args = (char **)malloc((num_args+1) * sizeof(char *));

    if (argv[4][0] == '-'){

        hostname_to_ip(argv[6] , IP);
        int32_t addr_len = strlen(argv[5])+strlen(IP)+strlen("@");
        char *addr = (char *)malloc((addr_len+1)*sizeof(char));
        memcpy(addr, argv[5], strlen(argv[5]));
        memcpy(addr+strlen(argv[5]), "@", strlen("@"));
        memcpy(addr+strlen(argv[5])+strlen("@"), IP, strlen(IP));
        addr[addr_len] = '\0';
        command_args[0] = addr;
        command_args[1] = addr;
        if (port_forwarding == 1){
            command_args[6] = "PF";
            command_args[7] = argv[9];
        }
    }
    else{ 

        hostname_to_ip(argv[4] , IP);
        command_args[0] = IP;
        command_args[1] = IP;
        if (port_forwarding == 1){
            command_args[6] = "PF";
            command_args[7] = argv[7];
        }
    }
    command_args[2] = SERVER_SCRIPT_PATH ;
    command_args[3] = argv[1];
    command_args[4] = argv[2];
    command_args[5] = argv[3];
    command_args[num_args] = (char *)0;

    char *command_path = (char*)malloc((100) * sizeof(char));
    find_command_path("ssh", command_path);

    pid_t pid;
    if (pipe(read_fd)){
        exit(0);
    }
    if (pipe(write_fd)){
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

    set_fl(write_fd[1], O_NONBLOCK);
    set_fl(read_fd[0], O_NONBLOCK);

    return IP;
}

void run_transceiver(int32_t *read_fd, int32_t *write_fd, char **argv, char *ID, FILE *fp_log){

    int32_t num_args = 5;
    char **command_args = (char**)malloc((num_args+1) * sizeof(char *));
    char *IP = (char *)malloc(100 * sizeof(char));

    if (argv[4][0] == '-'){
        hostname_to_ip(argv[6] , IP);
        int32_t addr_len = strlen(argv[5])+strlen(IP)+strlen("@");
        char *addr = (char *)malloc((addr_len+1)*sizeof(char));
        memcpy(addr, argv[5], strlen(argv[5]));
        memcpy(addr+strlen(argv[5]), "@", strlen("@"));
        memcpy(addr+strlen(argv[5])+strlen("@"), IP, strlen(IP));
        addr[addr_len] = '\0';
        command_args[0] = addr;
        command_args[1] = addr;

    }
    else{
        hostname_to_ip(argv[4] , IP);
        command_args[0] = IP;
        command_args[1] = IP;
    }

    command_args[2] = TRANSCEIVER_PATH ;
    command_args[3] = argv[2];
    command_args[4] = ID;
    command_args[num_args] = (char *)0;

    char *command_path = (char*)malloc((100) * sizeof(char));
    find_command_path("ssh", command_path);

    pid_t pid;
    if (pipe(read_fd)){
        exit(0);
    }
    if (pipe(write_fd)){
        exit(0);
    }

    pid = fork();
    if(pid == 0){     // child code
        dup2(read_fd[1], STDOUT_FILENO); // Redirect stdout into writing end of pipe
        dup2(write_fd[0], STDIN_FILENO); // Redirect reading end of pipe into stdin

        int saved_flags = fcntl(STDOUT_FILENO, F_GETFL);
        fcntl(STDOUT_FILENO, F_SETFL, saved_flags & ~O_NONBLOCK);
        saved_flags = fcntl(STDIN_FILENO, F_GETFL);
        fcntl(STDOUT_FILENO, F_SETFL, saved_flags & ~O_NONBLOCK);

        close(read_fd[0]);
        close(read_fd[1]);
        close(write_fd[0]);
        close(write_fd[1]);

        execv(command_path, command_args);
        exit(1);
    }

    int saved_flags = fcntl(read_fd[0], F_GETFL);
    fcntl(read_fd[0], F_SETFL, saved_flags & ~O_NONBLOCK);
    saved_flags = fcntl(write_fd[1], F_GETFL);
    fcntl(write_fd[1], F_SETFL, saved_flags & ~O_NONBLOCK);  

    close(read_fd[1]); // Don't need writing end of the stdout pipe in parent.
    close(write_fd[0]); // Don't need the reading end of the stdin pipe in the parent

}

int32_t create_TCP_server(FILE *fp_log, uint32_t port){

    socklen_t optlen = sizeof(uint32_t);
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

int32_t *build_TCP_connections(char *IP, FILE *fp_log){

    socklen_t optlen = sizeof(uint32_t);
    uint32_t port = 1234;
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    int32_t *socket_desc = (int32_t *)malloc(num_of_streams * sizeof(int));
    // create sockets and connect them to server 
    int32_t i;

    for(i = 0; i<num_of_streams; i++){
        if((socket_desc[i] = socket(AF_INET , SOCK_STREAM , 0)) < 0){
            fwrite("Could not create socket \n", 1, strlen("Could not create socket \n"), fp_log);
            fflush(fp_log);
            exit(0);
        }
        
        while (connect(socket_desc[i], (struct sockaddr *)&server, sizeof(server)) < 0)
            ;
    }

    return socket_desc;
}

int32_t **build_SSH_connections(char **argv, FILE *fp_log){

    int32_t i;
    int32_t transceiver_read_fd[num_of_streams][2];
    int32_t transceiver_write_fd[num_of_streams][2];

    int32_t **transceiver_fds = (int32_t **)malloc(2 * sizeof(int32_t *));
    transceiver_fds[0] = (int32_t *)malloc(num_of_streams * sizeof(int32_t));
    transceiver_fds[1] = (int32_t *)malloc(num_of_streams * sizeof(int32_t));

    for(i = 0; i<num_of_streams; i++){
        char ID[10];
        sprintf(ID, "%d", i);
        run_transceiver(transceiver_read_fd[i], transceiver_write_fd[i], argv, ID, fp_log);
        transceiver_fds[0][i] = transceiver_read_fd[i][0];
        transceiver_fds[1][i] = transceiver_write_fd[i][1]; 
    }

    return transceiver_fds;
}

void write_to_SSH_stream(int32_t *transceiver_write_fds, fd_set write_tempfds, int32_t read_fd, FILE *fp_log){

    uint32_t temp_seq, maxDiff = 0;
    int32_t stream_index, wrt, rd;
    unsigned char buffer[block_size];
    
    if((rd = read(read_fd, buffer+sizeof(uint64_t)+sizeof(uint32_t), block_size-(sizeof(uint64_t)+sizeof(uint32_t)))) < 0){
        fwrite("Receive from child fail \n", 1, strlen("Receive from child fail \n"), fp_log);
        fflush(fp_log);
        exit(0);
    }
    else if (rd > 0){
        temp_seq = seqnum_send / 4294967296; // = 2^32 
        temp_seq = htonl(temp_seq);
        memcpy(buffer, &temp_seq, sizeof(uint32_t));
        temp_seq = seqnum_send % 4294967296;
        temp_seq = htonl(temp_seq);
        memcpy(buffer+sizeof(uint32_t), &temp_seq, sizeof(uint32_t));
        rd =  htonl(rd);
        memcpy(buffer+sizeof(uint64_t), &rd, sizeof(uint32_t));
        stream_index = rand() % num_of_streams;
        while(!FD_ISSET(transceiver_write_fds[stream_index], &write_tempfds)){
            stream_index += 1;
            if (stream_index == num_of_streams)
                stream_index = 0;
        }
        rd =  ntohl(rd);
        if((wrt = writen(transceiver_write_fds[stream_index], buffer, rd+sizeof(uint64_t)+sizeof(uint32_t))) < rd+sizeof(uint64_t)+sizeof(uint32_t)){
            fwrite("Send fail \n", 1, strlen("Send fail \n"), fp_log);
            fflush(fp_log);
            exit(0);
        }
        else{
            seqnum_send += 1;
        }
    }
}

void write_to_TCP_stream(int32_t *socket_desc, fd_set write_tempfds, int32_t read_fd, FILE *fp_log){

    socklen_t optlen = sizeof(uint32_t);
    uint32_t temp_seq, i, maxDiff = 0;
    int32_t wrt, rd, stream_index = -1;
    unsigned char buffer[block_size];

    stream_index = rand() % num_of_streams;
    while(!FD_ISSET(socket_desc[stream_index], &write_tempfds)){
        stream_index += 1;
        if (stream_index == num_of_streams)
            stream_index = 0;
    }

    if((rd = read(read_fd, buffer+sizeof(uint64_t)+sizeof(uint32_t), block_size-(sizeof(uint64_t)+sizeof(uint32_t)))) < 0){
        fwrite("Receive from child fail \n", 1, strlen("Receive from child fail \n"), fp_log);
        fflush(fp_log);
        exit(0);
    }
    else if (rd > 0){ 
        temp_seq = seqnum_send / 4294967296; // = 2^32 
        temp_seq = htonl(temp_seq);
        memcpy(buffer, &temp_seq, sizeof(uint32_t));
        temp_seq = seqnum_send % 4294967296;
        temp_seq = htonl(temp_seq);
        memcpy(buffer+sizeof(uint32_t), &temp_seq, sizeof(uint32_t));
        rd =  htonl(rd);
        memcpy(buffer+sizeof(uint64_t), &rd, sizeof(uint32_t));
        rd =  ntohl(rd);
        if((wrt = writen(socket_desc[stream_index], buffer, rd+sizeof(uint64_t)+sizeof(uint32_t))) < rd+sizeof(uint64_t)+sizeof(uint32_t)){
            fwrite("Send fail \n", 1, strlen("Send fail \n"), fp_log);
            fflush(fp_log);
            exit(0);
        }
        else{
            seqnum_send += 1;
        }
    }
}

int32_t write_to_buffer(int32_t *read_desc, fd_set read_tempfds, FILE *fp_log, char **buffer_rec, 
                    uint32_t *byte_nums, uint32_t *offset){

    unsigned char buffer[block_size];
    uint32_t rec_num_bytes, offset_temp;
    uint32_t temp_seq;
    uint64_t seqnum_temp;
    int32_t maxDiff, i, rd, wrt;
    int32_t recBuff = 0;
    int32_t maxRec = 0;
    int32_t stream_index = -1;
    while (stream_index == -1){
        for(i=0; i<num_of_streams; i++){
            if (FD_ISSET(read_desc[i], &read_tempfds)) {
                ioctl(read_desc[i], FIONREAD, &recBuff);
                if (recBuff >= maxRec && recBuff>=sizeof(uint64_t)+sizeof(uint32_t)) {
                    stream_index = i;
                    maxRec = recBuff;
                }
            }
        }
    }

    if((rd = readn(read_desc[stream_index], buffer, sizeof(uint64_t)+sizeof(uint32_t))) == sizeof(uint64_t)+sizeof(uint32_t)){
        memcpy(&temp_seq, buffer, sizeof(uint32_t));
        temp_seq = ntohl(temp_seq);
        seqnum_temp = temp_seq * 4294967296;
        memcpy(&temp_seq, buffer+sizeof(uint32_t), sizeof(uint32_t));
        temp_seq = ntohl(temp_seq);
        seqnum_temp += temp_seq;
        if (seqnum_temp == 0){
            fwrite("The End! \n", 1, strlen("The End! \n"), fp_log);
            fflush(fp_log);
            return 0;
        }
        memcpy(&rec_num_bytes, buffer+sizeof(uint64_t), sizeof(uint32_t));
        rec_num_bytes = ntohl(rec_num_bytes);
    
        char *buff_temp = (char*)malloc((rec_num_bytes) * sizeof(unsigned char));
        if ((rd = readn(read_desc[stream_index], buff_temp, rec_num_bytes)) == rec_num_bytes){
            buffer_rec[seqnum_temp%max_out_of_orders] = buff_temp;
            byte_nums[seqnum_temp%max_out_of_orders] = rec_num_bytes;
            offset[seqnum_temp%max_out_of_orders] = 0;
            num_of_out_of_orders += 1;
        }
        else{
            fwrite("read fail* \n", 1, strlen("read fail* \n"), fp_log);
            fflush(fp_log);
            exit(0);
        }
    }
    else{
        fwrite("read header fail \n", 1, strlen("read header fail \n"), fp_log);
        fflush(fp_log);
        exit(0);
    }

    return 1;
}

void write_to_output(FILE *fp_log, char **buffer_rec, uint32_t *byte_nums, uint32_t *offset, int32_t write_fd){

    int32_t wrt;
    while(num_of_out_of_orders > 0){
        wrt = 0;
        if (byte_nums[seqnum_rec%max_out_of_orders] != 0){
            if((wrt = write(write_fd, buffer_rec[seqnum_rec%max_out_of_orders]+offset[seqnum_rec%max_out_of_orders], byte_nums[seqnum_rec%max_out_of_orders])) <= 0){
                ;
            }
            else if(wrt < byte_nums[seqnum_rec%max_out_of_orders]){
                buffer_rec[seqnum_rec%max_out_of_orders] += wrt;
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

void initialize_fd_sets(fd_set *activefds, int32_t *socket_desc, int32_t **transceiver_fds, 
                        int32_t input_fd, int32_t output_fd, int32_t *maxfds){

    // 0: TCP sockets, 1: read desc, 2: write desc, 3: stdin, 4: stdout
    int32_t i = 0;

    for (i = 0; i<5; i++){
        maxfds[i] = 0;
        FD_ZERO (&activefds[i]);
    }

    for (i = 0; i<num_of_streams; i++){

        if (transceiver_fds != NULL){
            FD_SET(transceiver_fds[0][i], &activefds[1]);
            if (transceiver_fds[0][i] > maxfds[1])
                maxfds[1] = transceiver_fds[0][i];
            FD_SET(transceiver_fds[1][i], &activefds[2]);
            if (transceiver_fds[1][i] > maxfds[2])
                maxfds[2] = transceiver_fds[1][i];
        }
        else if (socket_desc != NULL){
            FD_SET(socket_desc[i], &activefds[0]);       
            if(socket_desc[i] > maxfds[0])
                maxfds[0] = socket_desc[i];
        }
    }

    FD_SET(input_fd, &activefds[3]);
    maxfds[3] = input_fd;
    FD_SET(output_fd, &activefds[4]);
    maxfds[4] = output_fd;

    set_fl(input_fd, O_NONBLOCK);
    set_fl(output_fd, O_NONBLOCK);
}

void close_fds (int32_t *socket_desc, int32_t **transceiver_fds){
    int32_t i;

    if (socket_desc != NULL)
        for(i = 0; i<num_of_streams; i++)
            close(socket_desc[i]);

    else if (transceiver_fds != NULL)
        for(i = 0; i<num_of_streams; i++){
            close(transceiver_fds[0][i]);
            close(transceiver_fds[1][i]);
        }
}

int32_t main(int32_t argc, char *argv[]){

    num_of_streams = atoi(argv[1]);
    block_size = atoi(argv[2]);
    int32_t select_out, select_out_std;
    uint32_t local_port;

    if ((argc > 5 && argv[5][0] == 'P' && argv[5][1] == 'F') || (argc > 7 && argv[7][0] == 'P' && argv[7][1] == 'F')){
        port_forwarding = 1;
        if (argc > 5 && argv[5][0] == 'P' && argv[5][1] == 'F')
            local_port = atoi(argv[6]);
        else
            local_port = atoi(argv[8]);
    }

    int32_t server_socket, client_socket;
    int32_t *read_desc;
    int32_t *socket_desc;
    int32_t **transceiver_fds;
    char **buffer_rec = malloc(max_out_of_orders * sizeof(char *));
    uint32_t *byte_nums = malloc(max_out_of_orders * sizeof(uint32_t));
    uint32_t *offset = malloc(max_out_of_orders * sizeof(uint32_t));
    
    fd_set activefds[5];
    fd_set read_tempfds, write_tempfds, stdin_tempfd, stdout_tempfd; 
    int32_t maxfds[5];
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FILE *fp_log = fopen("~/log_PDS_client.txt", "w"); 
 
    if (port_forwarding == 1){
        server_socket = create_TCP_server(fp_log, local_port);
        if ((client_socket = accept(server_socket, NULL, NULL)) <0){
            fwrite("Accept failed! \n", 1, strlen("Accept failed! \n"), fp_log);
            fflush(fp_log);
            exit(0);
        }
        /*socklen_t optlen = sizeof(uint32_t);
        struct sockaddr_in server;
        server.sin_addr.s_addr = inet_addr("127.0.0.1");
        server.sin_family = AF_INET;
        server.sin_port = htons(local_port);

        if((client_socket = socket(AF_INET , SOCK_STREAM , 0)) < 0){
            fwrite("Could not create socket \n", 1, strlen("Could not create socket \n"), fp_log);
            fflush(fp_log);
            exit(0);
        }

        while (connect(client_socket, (struct sockaddr *)&server, sizeof(server)) < 0)
            ;*/
    }

    /* run PDS server on the other side via SSH */

    int32_t read_fd[2];
    int32_t write_fd[2];

    char *IP = run_PDS_server(read_fd, write_fd, argc, argv, fp_log);

    /* Sending setup parameters */

    if (port_forwarding == 0)
        send_args(argc, argv, write_fd[1], fp_log);

    if (strcmp(argv[3], "TCP") == 0){
        socket_desc = build_TCP_connections(IP, fp_log);
        if (port_forwarding == 1)
            initialize_fd_sets(activefds, socket_desc, NULL, client_socket, client_socket, maxfds);
        else
            initialize_fd_sets(activefds, socket_desc, NULL, STDIN_FILENO, STDOUT_FILENO, maxfds);
    } 

    else if (strcmp(argv[3], "SSH") == 0){
        transceiver_fds = build_SSH_connections(argv, fp_log);
        if (port_forwarding == 1)
            initialize_fd_sets(activefds, NULL, transceiver_fds, client_socket, client_socket, maxfds);
        else
            initialize_fd_sets(activefds, NULL, transceiver_fds, STDIN_FILENO, STDOUT_FILENO, maxfds);
    } 

    while (1) {

        // read from stdin and write to sockets! 
        stdin_tempfd = activefds[3];
        select_out_std = select(maxfds[3]+1, &stdin_tempfd, NULL, NULL, &tv);

        if (select_out_std > 0 && strcmp(argv[3], "SSH") == 0){
            write_tempfds = activefds[2];
            select_out = select(maxfds[2]+1, NULL, &write_tempfds, NULL, &tv);
            if (select_out > 0){
                if (port_forwarding == 1)
                    write_to_SSH_stream(transceiver_fds[1], write_tempfds, client_socket, fp_log);
                else
                    write_to_SSH_stream(transceiver_fds[1], write_tempfds, STDIN_FILENO, fp_log);
            }
        }

        else if (select_out_std > 0 && strcmp(argv[3], "TCP") == 0){
            write_tempfds = activefds[0];
            select_out = select(maxfds[0]+1, NULL, &write_tempfds, NULL, &tv);
            if (select_out > 0){
                if (port_forwarding == 1)
                    write_to_TCP_stream(socket_desc, write_tempfds, client_socket, fp_log);  
                else
                    write_to_TCP_stream(socket_desc, write_tempfds, STDIN_FILENO, fp_log);
            }
        }
  
        // read from sockets and write to stdout!
 
        if (strcmp(argv[3], "SSH") == 0){
            read_tempfds = activefds[1];
            select_out = select(maxfds[1]+1, &read_tempfds, NULL, NULL, &tv);
            read_desc = transceiver_fds[0];
        }

        else if (strcmp(argv[3], "TCP") == 0){
            read_tempfds = activefds[0];
            select_out = select(maxfds[0]+1, &read_tempfds, NULL, NULL, &tv);
            read_desc = socket_desc;
        }

        // there is some data ready on sockets and there is free space on stdout
        if (select_out > 0)
            if (write_to_buffer(read_desc, read_tempfds, fp_log, buffer_rec, byte_nums, offset) == 0)
                break;     

        if (port_forwarding == 1)
            write_to_output(fp_log, buffer_rec, byte_nums, offset, client_socket);
        else
            write_to_output(fp_log, buffer_rec, byte_nums, offset, STDOUT_FILENO);

    }//while 

    free(buffer_rec);
    free(byte_nums);
    free(offset);

    if (strcmp(argv[3], "TCP") == 0)
        close_fds (socket_desc, NULL);
    else if(strcmp(argv[3], "SSH") == 0)
        close_fds (NULL, transceiver_fds);

    return 0;
}
