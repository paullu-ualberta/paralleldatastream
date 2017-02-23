#/* PDS, Version 0.9
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

all: PDS_client.c PDS_server.c  transceiver.c unpcode.c unpheaders.h
	gcc -O3 -c unpcode.c
	gcc -O3 -c PDS_client.c
	gcc -O3 -c PDS_server.c
	gcc -O3 -c transceiver.c
	gcc -O3 PDS_client.o unpcode.o -o PDS_client.out
	gcc -O3 PDS_server.o unpcode.o -o PDS_server.out
	gcc -O3 transceiver.o unpcode.o -o transceiver.out

clean:
	-rm *.out
	-rm *.o
