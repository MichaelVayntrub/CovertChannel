# CovertChannel
 A channel that cannot be uncovert

This project consists of two parts:
  - A C program that create a covert channel by manipulating two TCP channel's
    packets sending order - "The covert sender". (runs on a virtual machine)
    [VMfiles\CovertChannel]
  - A Python server that decipher a message that delivered by the covert channel
    according the received packets order - "The covert receiver". (runs on the host)
    [PythonServer]

# Environment - virtual machine:
The virtual machine has Ubuntu OS and running on VirtualBox platform.

# Bash Terminal commands:
sudo apt update                               → refreshes the local package manager
sudo apt install build-essential              → Install includes gcc compiler.
sudo apt-get install libnetfilter-queue-dev   → Install netfilter-queue library.
sudo apt-get install iptables                 → Install iptables.
# Visual Studio code Extentions:
C/C++
C/C++ Extension Pack

# Environment - python server:
