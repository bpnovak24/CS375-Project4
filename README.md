# CS375 Computer Networking
### Project 4 - Sliding Window Protocol
#### Brandon Novak and Ollie Strasburgh


For project 4, we implemented the Sliding Window Protocol on UDP sockets. All relevant code pertaining to our protocol is found in `ARQ.h`, and `test_client.cpp` and `test_client.cpp` are the test files for the client/sender and server/receiver, respectively.

In `ARQ.h`, we create a struct for our packet structure, a class for the Sender, and a class for the Receiver. We will discuss each structure's design decisions and provide explanations in depth.

## `Packet` Struct

`Packet` is the struct that will store all the information that we will sending within our sliding window protocol. The struct contains 5 elements within it. 

1. `long int seqnum` = 4 byte field that contains the sequence number of the message.
2. `char ACK` = 1 bytes field that indicates if the message is an Acknowledgement message. `0x01` = ACK, `0x00` = Not ACK.
3. `char control` = 1 byte indicator if the message is for setup, teardown or a regular message. `0x00` = regular data, `0x01` = connection setup, `0x02` = connection teardown. 
4. `short length` = length of the `data` field in bytes
5. `char data[1024]` = character array of size 1024 bytes. Stores message of packet. 

## `Sender` class

Class that will server as an object in test files. 

3 attributes:
1. `int sockfd` = socket descriptor
2. `sockaddr recv_addr` = address of the receiver object
3. `socklen_t addr_len` = length of receiver address

Methods:
1. `int setup_connection(char* hostname)` = Send initial message to receiver and receive acknowledgement that the two hosts are connected.

2. `void conversation()` = Send messages to receiver and receive ACKs from receiver. Teardown connection when conversation ends. 

#### int setup_connection(char* hostname)

Bind IP address to a socket using the techniques from the last Project 1 and Project 2. 




