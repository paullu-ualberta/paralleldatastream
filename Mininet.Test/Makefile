#/* PDS testing using Mininet, Version 0.9
# *
# * This copyright header adapted from the Linux 4.1 kernel.
# *
# * Copyright 2017 Nooshin Eghbal, Hamidreza Anvari, Paul Lu
# *
# * This program is free software; you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as published by
# * the Free Software Foundation; either version 3 of the License, or
# * (at your option) any later version.
# *
# * This program is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU General Public License for more details.
# *      
# * You should have received a copy of the GNU General Public License
# * along with this program; if not, write to the Free Software
# * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
# */

SHELL          := /bin/bash
TEST_CODE      := testPDS.py

TEST_COMMAND := ssh -A h1 'rsync --progress --rsh="/home/ubuntu/bin/PDS_client.out 4 10000 TCP" /home/ubuntu/100m.blob h2:/home/ubuntu/100m.rcv'
 
test_old:
	sudo  $(CURDIR)/$(TEST_CODE) $(TEST_COMMAND) 

test:
	sudo $(CURDIR)/$(TEST_CODE) > /dev/null 2>&1 & 
	sleep 3 #to make sure mininet is up & running
	$(TEST_COMMAND)
	#ASSERTION here
	#sudo pkill -2 $(TEST_CODE)


#configures the mininet testbed and brings up mininet CLI for interactive testing
testcli:
	sudo  $(CURDIR)/$(TEST_CODE) cli

#configures mininet nodes with ssh service for running tests and returns
start:
	sudo $(CURDIR)/$(TEST_CODE) > /dev/null 2>&1 &

#sends stop signal to the already set up mininet configuration (which is waiting for signal to terminate)
stop:
	sudo pkill -2 $(TEST_CODE)
	pkill PDS_client.out

.PHONY: test testcli start stop
