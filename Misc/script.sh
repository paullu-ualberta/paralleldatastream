#!/bin/bash

sudo tc qdisc change dev eth1 root netem limit 10000000000 delay 50ms loss 0.1% 0%

FILE=$1

for j in 1 2 3 4 5
do

ssh 192.168.154.34 rm -rf "/dev/shm/eghbal/14G.blob"
sleep 15

(time -p rsync  --rsh="PDS_client.out 8 10000 TCP"  /dev/shm/eghbal/14G.blob  192.168.154.34:/dev/shm/eghbal/14G.blob) 2>> $FILE

ssh 192.168.154.34 rm -rf "/dev/shm/eghbal/14G.blob"
sleep 15

(time -p rsync /dev/shm/eghbal/14G.blob  192.168.154.34:/dev/shm/eghbal/14G.blob) 2>> $FILE

ssh 192.168.154.34 rm -rf "/dev/shm/eghbal/14G.blob"
sleep 15

(ssh 192.168.154.34 'time -p rsync  --rsh="PDS_client.out 8 4000 SSH" 192.168.154.33:/dev/shm/eghbal/14G.blob /dev/shm/eghbal/14G.blob') 2>> $FILE

sleep 15

(time -p globus-url-copy -p 8 -fast -g2 file:///dev/shm/eghbal/14G.blob ftp://eghbal:123456@192.168.154.34:2811/dev/shm/eghbal/14G.blob) 2>> $FILE

done 
