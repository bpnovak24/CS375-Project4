# CS375 Computer Networking
### Project 4 - Sliding Window Protocol
#### Brandon Novak and Ollie Strasburg


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

First, we check if the Sender object has received a message. This if statement will actually only read when it is trying to a receive an ACK a second time from the Receiver. We for loop through our window size which is between the `LAR` and `LFS` which are indexed within the `buffer` with `LAR_index` and `LFS_index`. We check if there is a packet being stored. If so, we replace the `LAR`to that sequence number, and update `LAR_index` accordingly.

```
for (i = LAR_index; i <= LFS_index; i++){
  if (ntohl((*(buffer[i])).seqnum) > LAR){
    LAR = ntohl((*(buffer[i])).seqnum);
    LAR_index = (LAR % window)-1;
  }
 }
```

If the Sender wants to send a message, we use the `fgets` function in order to receive from the terminal. We use a `temp` packet to receive the information. Unless the message is "EXIT", then the following elements are always: 
```
1. fgets(temp.data, 1024, stdin);
2 temp.seqnum = htonl(sequence);
3 temp.ACK = 0;
4 temp.control = 0;
5 temp.length = htons(strnlen(temp.data,1024));
```

We add an issue where the `sizeof` operand was not working correctly in the `sendto` function so we had to calculate our size of message manually, and then we send the packet to the Receiver.

```
packet_size = sizeof temp.seqnum + sizeof temp.ACK + sizeof temp.control + sizeof temp.length + sizeof temp.data;
```

After we send the packet, we update our `LFS` by one, and the `LFS_index` in the following manner. 

```
LFS++;
LFS_index = (LFS % window)-1;
```

We allow 5 seconds to pass until we will resend a message. Typically Round Trip Time (RTT) is used as the standard timeout, but we decided for 5 because it was easier for testing purposes. 

If we receive an ACK before 5 seconds then a similar process occurs where we check the window to ensure we send the biggest LAR possible so as to clear the window as quickly as possible. 

## `Receiver` class

Class that will server as an object in test files. 

3 attributes:
1. `int sockfd` = socket descriptor
2. `sockaddr sender_addr` = address of the sender object
3. `socklen_t addr_len` = length of sender address

Methods:
1. `int setup_connection(char* hostname)` = Receive initial message from sender and send acknowledgement that the two hosts are connected.

2. `void conversation()` = Receive messages from sender and send ACKs back. Teardown connection when conversation ends. 


#### void setup_connection()

Works essentially the  same as Sender::setup_connection(). It sends a packet back to the sender after it receives the packet from Sender::setup_connection().

#### void conversation()

We initialize the Next Frame Expected and the Last Frame Acceptable as `NFE` and `LFA`. The window size is initialized as 5. We initialize a pointer to a `Packet` array of size `window` + 1. 

```
long int NFE;//next frame expected
long int LFA;//last frame acceptable
int window = 5;
int size = window + 1;
Packet* buffer[size];
```

Then we continue unto a while loop that will only break if the `packet_send`'s control equals 2 because a control of 2 indicates that the conversation is over. As of right now, the only way to activate a control of 2 is for the Sender to send the message "EXIT". Inside the while loop, we dynamically allocate memory for the `packet_recv`. We will use this to store the Packet that we receive from the sender. recvfrom() is then used to get  the  incoming message from the sender, which is stored in a temporary packet `temp`. We print the received packet to confirm its correctness, then use memcpy() to place the packet into `packet_recv`. We then convert the sequence number and length from the packet back to host byte order.

```
Packet* packet_recv = new Packet;
  int numbytes = recvfrom(sockfd, &temp, MAXBUFLEN-1, 0, (struct sockaddr *)&sender_addr, &addr_len);
  if (numbytes == -1){
     perror("recvfrom");
     exit(1);
    }
  printf("Temp Received: \%d, \%d, \%d, \%d, \%s\n", ntohl(temp.seqnum), temp.ACK, temp.control, ntohs(temp.length), temp.data);
  memcpy(packet_recv, &temp, sizeof temp);

  packet_recv->seqnum = ntohl(packet_recv->seqnum);
  packet_recv->length = ntohs(packet_recv->length);
```
 
 Next, we check if `packet_recv` has already been received.  If it has, we resend the ACK for that packet. 

```
if (packet_recv->seqnum < NFE){
            packet_send.seqnum = htonl(packet_recv->seqnum);
            packet_send.ACK = 1;
            packet_send.control = packet_recv->control;
            packet_send.length = htons(0);
            printf("Resending: \%d, \%d, \%d, \%d, \%s\n", ntohl(packet_send.seqnum), packet_send.ACK, packet_send.control, packet_send.length, packet_send.data);
            bytes_sent = sendto(sockfd, &packet_send, sizeof packet_send, 0, (struct sockaddr *)&sender_addr, addr_len);
   }
 ```
If this is a new packet, we check if the sequence number is within our window. If it  is, we calculate the index in  the buffer that corresponds to the  packet. Then, we place the packet into the buffer. 

```
buffer_index = (packet_recv->seqnum \% window)-1;//find index in buffer to store packet in
            buffer[buffer_index] = packet_recv;//store packet

```
If `packet_recv` has a sequence number equal to the NFE, then  we update the NFE, and set the sequence number of the ACK packet (`packet_send`) to that sequence number. Then, we check if the subsequent packets in  the buffer have sequence numbers directly after `packet_send.seqnum`. We continue updating the NFE and `packet_send.seqnum` until we find a packet with a smaller sequence number. After the  loop, we update the LFA.

```
if (packet_recv->seqnum == NFE){//if packet is the next one expected
  packet_send.seqnum = NFE;
  NFE++;
  for (i = buffer_index+1; i < LFA; i++){
    if ((*(buffer[i\%size])).seqnum == NFE){//check if you need to ACK for more than one packet
      packet_send.seqnum = NFE;
      NFE++;
    }
    else{//if done incrementing NFE and going through buffer (out of later consecutive ACKS)
      break;
    }
  }
}
LFA = NFE + window;//update last frame acceptable

```

We convert the `packet_send.seqnum` to network byte order, and prepare the  rest of the acknowledgement packet to send to the sender. If `packet_recv.control` is 2, we set the control to 2. This indicates that the connection will close and the while loops in both the `Sender` and `Receiver` classes will stop. Then, `packet_send` is sent back to the sender.


```
packet_send.seqnum = htonl(packet_send.seqnum);
packet_send.ACK = 1;
packet_send.control = packet_recv->control;
packet_send.length = htons(0);

if (packet_recv->control == 2){//if sender says to quit
  packet_send.control = 2;
}


printf("Sending: \%d, \%d, \%d, \%d\n", ntohl(packet_send.seqnum), packet_send.ACK, packet_send.control, packet_send.length);

bytes_sent = sendto(sockfd, &packet_send, sizeof packet_send, 0, (struct sockaddr *)&sender_addr, addr_len);

if (bytes_sent == -1){
  perror("send");
}
```


