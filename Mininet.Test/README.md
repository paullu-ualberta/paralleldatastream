# PDS
Parallel Data Streams Tool, testing using Mininet

## Overview
PDS is a tool on top of which other client-server programs like rsync can run.
It transfers data between client and server through parallel TCP streams like
the mechanism used by GridFTP for transferring files.

## License
**NOTE**
The original parts of this code are released under GPLv2.
However, some parts of the code are from the Unix Network Programming book.
Until that code is replaced, please do NOT distribute this code further.

## Mininet VM Setup Guide
1) Run ~/mininet/examples/sshd.py in your Mininet VM. This example creates a
tree network of 4 hosts and assigns an IP address to each node and also run
sshd on them so then you can ssh to each of them.
In the output of this example, you can see the IP address of each of these 4
hosts. For example, 10.0.0.1 is the IP of h1.

2) From a terminal on your host machine, first do ssh to Mininet VM by
```bash
ssh -Y mininet@192.168.56.101
```
3) Open the PDS\_client.c file and modify the path of PDS\_server.out in
run\_PDS\_server() to "/home/mininet/PDS\_server.out".
Run "make" to compile PDS code and have PDS\_cleint.out and PDS\_server.out in
/home/mininet/.

4) Do ssh to let's say h1 by
```bash
ssh 10.0.0.1
```
5) Run PDS command to transfer a file from h1 to h2 through the link between
them by
```bash
rsync  --progress --rsh="./PDS\_client.out 4 10000 TCP" /home/mininet/200M.blob 10.0.0.2:/home/mininet/200M\_rec.blob
```
