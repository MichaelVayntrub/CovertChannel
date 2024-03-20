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
Using Visual Studio code as an editor (Extensions: C/C++, C/C++ Extension Pack).
- VirtualBox v7.0.14
- Ubuntu OS v22.04.3-desktop-amd64
- Visual Studio code v1.87.2
- gcc (Ubuntu 11.4.0-1ubuntu1~22.04) v11.4.0 - ['sudo apt install build-essential']
- iptables v1.8.7 (nf_tables) - ['sudo apt-get install iptables']
- libnetfilter_queue v1.0.5 - ['sudo apt-get install libnetfilter-queue-dev']

# Environment - python server:
The server create 2 listening TCP sockets on ports 54321, 54322 with the Python
socket library.
Using Pycharm as an editor.
- PyCharm Community Edition 2021.2.2
- Python v3.12.2

# How to use:
- Open the VMfiles\CovertChannel folder on the vm using the Visual code editor,
  and then press Ctrl+B and choose the Build & Run option.
- Fill yours sudo password (sudo user's permissions required to run the program)
- The covert channel program will load the modules, update the iptables
  and then will wait for the host's ip address.
- Open the PythonServer folder on Pycharm (or any other python editor) and
  then run the server by typing in the terminal "python program.py".
- Type the "-run" command in the server's input to establish the TCP sockets.
- Press the TAB key to send the vm the host's ip.
- After doing so the connection will be established and packets will be received
  one after another, and in case of existing covert message, it will be updated
  at the bottom after each character that have been deciphered.
- You can press ESC to end the connection and if the covert message has been updated
  you'll get a message to choose if to save or discard the message.
- Depending on your choice it will be saved in the database or discarded for eternity.
- After the covert session you'll be able to view the messages in the database by
  typing the command "-msg".
