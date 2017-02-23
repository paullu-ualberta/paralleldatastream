#!/usr/bin/python

#/* PDS and Mininet, Version 0.9
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

import sys
import os
import signal
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.util import dumpNodeConnections
from mininet.log import lg
from mininet.cli import CLI
from mininet.node import Node
from mininet.util import waitListening

#def pause():
#    programPause = raw_input("Press the <ENTER> key to continue...")

def signal_handler(signal_number, frame):
    print "Proceed ..."

class SingleSwitchTopo(Topo):
    "Single switch connected to n hosts."
    def build(self, n=2):
        ###print "in build"
        switch = self.addSwitch('s1')
        # Python's range(N) generates 0..N-1
        ###print "switch added"
        for h in range(n):
            host = self.addHost('h%s' % (h + 1))
            ###print "host added"
            self.addLink(host, switch)
            ###print "host linked"

def connectToRootNS( network, switch, ip, routes ):
    """Connect hosts to root namespace via switch. Starts network.
      network: Mininet() network object
      switch: switch to connect to root namespace
      ip: IP address for root namespace node
      routes: host networks to route to"""
    # Create a node in root namespace and link to switch 0
    root = Node( 'root', inNamespace=False )
    intf = network.addLink( root, switch ).intf1
    root.setIP( ip, intf=intf )
    # Start network that now includes link to root namespace
    network.start()
    # Add routes from root ns to hosts
    for route in routes:
        root.cmd( 'route add -net ' + route + ' dev ' + str( intf ) )

def sshd( network, cmd='/usr/sbin/sshd', opts='-D',
          ip='10.123.123.1/32', routes=None, switch=None ):
    """Start a network, connect it to root ns, and run sshd on all hosts.
       ip: root-eth0 IP address in root namespace (10.123.123.1/32)
       routes: Mininet host networks to route to (10.0/24)
       switch: Mininet switch to connect to root namespace (s1)"""
    if not switch:
        switch = network[ 's1' ]  # switch to use
    if not routes:
        routes = [ '10.0.0.0/24' ]
    connectToRootNS( network, switch, ip, routes )
    for host in network.hosts:
        host.cmd( cmd + ' ' + opts + '&' )
    print "*** Waiting for ssh daemons to start"
    for server in network.hosts:
        waitListening( server=server, port=22, timeout=5 )

    print
    print "*** Hosts are running sshd at the following addresses:"
    print
    for host in network.hosts:
        print host.name, host.IP()
    print
    #print "*** Type 'exit' or control-D to shut down network"
    #CLI( network )



def run_test(network, command):
    "Create network and run simple PDS correctness test" 
    h1, h2 = network.get('h1', 'h2')
    ###print "before cmd"
    h1.waitOutput(verbose=True)
    print "command: " + command.replace('HOST',h2.IP()) 
    print h1.cmd(command.replace('HOST',h2.IP()));


def initialize_settings():
    # Tell mininet to print useful information
    lg.setLogLevel('info')
    # cleanup mininet settings ramained uncleaned from previous runs
    os.system('sudo mn -c > /dev/null 2>&1')
    # cleanup transferred files from previous runs
    os.system('sudo rm -f /home/mininet/*.rcv > /dev/null 2>&1')
    #todo: extract constant values form script and define variables: home folder path, file to transfer, ...(?)

def start_testbed():
    topo = SingleSwitchTopo(n=2)
    net = Mininet(topo=topo)    
    # get sshd args from the command line or use default args
    # useDNS=no -u0 to avoid reverse DNS lookup timeout
    sshOpts = '-D' #'-D -o UseDNS=no -u0'
    sshd( net, opts=sshOpts )
    return net

def stop_testbed( network ):
    cmd = "/usr/sbin/sshd"
    for host in network.hosts:
        host.cmd( 'kill %' + cmd )
    network.stop()

if __name__ == '__main__':
    
    initialize_settings()
    network = start_testbed()
    
    if len(sys.argv)>1 and sys.argv[1] == 'cli':
        print "*** Type 'exit' or control-D to shut down network"
        CLI( network )
    else:
	signal.signal(signal.SIGINT, signal_handler)
	signal.pause()  
	#pause()  
        #command = " ".join(sys.argv[1:])
        #run_test( network=network, command=command )

    stop_testbed( network )
    
    
    
