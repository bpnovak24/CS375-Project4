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

We initialize a Packet called `packet_send` and we initialize with the following elements:
```
packet_send.seqnum = 0;
packet_send.ACK = 0;
packet_send.control = 1;
packet_send.length = 0;
packet_send.seqnum = htonl(packet_send.seqnum);
packet_send.length = htons(packet_send.length);
```

We give those particular values because those are the values which indicate the Sender would like to connect with the Receiver. We convert the `seqnum` and `length` into network order bytes. We send the packet to the Receiver, and wait for a response from the Receiver. Once the Sender receives a response from the Receiver, and the response is the same excepot for the ACK field (which should equal 1), then the Sender will print "Client: connection setup successful" and return 1. If the response did not contain the correct data in the elements, then it will return 0.

#### void conversation()

Initialize pollfd in order to identify a message to send and a message to receive.

```
struct pollfd pfds[2];
pfds[0].fd = 0;
pfds[0].events = POLLIN;
pfds[1].fd = sockfd;
pfds[1].events = POLLIN;
```

We initialize the Last ACK Received and the Last Frame Sent as `LAR` and `LFS`. The window size is initialized as 5. We initialize a pointer to a `Packet` array of size `window` + 1. 

```
long int LAR;//last ack Recieved
long int LFS;//last frame sent
int window = 5;
int size = window + 1;
Packet* buffer[size];
```

Then we continue unto a while loop that will only break if the `packet_recv`'s control equals 2 because a control of 2 indicates that the conversation is over. As of right now, the only way to activate a control of 2 is for the Sender to send the message "EXIT".

Inside the while loop, the function polls to see if it should sending or receiving, and we dynmically allocate memory for the `packet_send`. 

First, we check if the 









