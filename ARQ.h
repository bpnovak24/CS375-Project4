#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <chrono>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>


#define SERVER_PORT "8080"	// the port users will be connecting to
#define MYPORT 8080
#define MAXBUFLEN 1034 // max buffer length, measured in bytes



struct Packet{
  long int seqnum;
  char ACK;
  char control;
  short length;
  char data[1024];
};

class Sender{
  public:

    int sockfd;
    sockaddr recv_addr;
    socklen_t addr_len;


    //Methods
    int setup_connection(char* hostname){
      struct addrinfo hints, *server_info, *ptr;

      memset(&hints, 0, sizeof hints);
      hints.ai_family = AF_INET; // IPv4
      hints.ai_socktype = SOCK_DGRAM;
      hints.ai_protocol = 0;
      hints.ai_flags = AI_PASSIVE; //my IP address

      int status;
      status = getaddrinfo(hostname, SERVER_PORT, &hints, &server_info);
      if (status != 0)
      {
        printf("status: %d\n", status);
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
      }

      struct sockaddr_in my_addr;
      my_addr.sin_family = AF_INET;
      my_addr.sin_port = htons(MYPORT);
      my_addr.sin_addr.s_addr = INADDR_ANY;
      memset(my_addr.sin_zero, 0, sizeof my_addr.sin_zero);

      for(ptr = server_info; ptr != NULL; ptr = ptr->ai_next) {
    		if ((sockfd = socket(ptr->ai_family, ptr->ai_socktype,
    				ptr->ai_protocol)) == -1) {
          printf("sockfd: %d\n", sockfd);
    			perror("talker: socket");
    			continue;
    		}
    		break;
    	}

      //bind
      if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr) == -1) {
        close(sockfd);
        perror("listener: bind");
        }

      // Make sure it binded
      if (ptr == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
      }

      /* ===================================================================================*/
      //Setting up connection

      //initial packet
      Packet packet_send;
      packet_send.seqnum = 0;
      packet_send.ACK = 0;
      packet_send.control = 1;
      packet_send.length = 0;

      //convert to network order
      packet_send.seqnum = htonl(packet_send.seqnum);
      packet_send.length = htons(packet_send.length);


      int bytes_sent = sendto(sockfd, &packet_send, sizeof packet_send, 0, ptr->ai_addr,
                              ptr->ai_addrlen);
      if (bytes_sent == -1){
        perror("send");
      }

      //receive initial packet
      Packet packet_recv;
      int numbytes = recvfrom(sockfd, &packet_recv, sizeof packet_send, 0,
                              ptr->ai_addr, &ptr->ai_addrlen);
      if (numbytes == -1){
         perror("recvfrom");
         exit(1);
        }

       //convert to host order
       packet_recv.seqnum = ntohl(packet_recv.seqnum);
       packet_recv.length = ntohs(packet_recv.length);

       //save sender and receiver addresses
       recv_addr = *ptr->ai_addr;
       addr_len = ptr->ai_addrlen;

       //check if packet received is valid
       if ((packet_recv.seqnum == 0) & (packet_recv.ACK == 1) &
            (packet_recv.control == 1) & (packet_recv.length == 0)){
          printf("Client: connection setup successful \n");
          return 1;
      }
       else{
          return 0;
        }
      /* ===================================================================================*/
    }
    void conversation(){  //talk to the Receiver
      struct pollfd pfds[2];
      pfds[0].fd = 0; //stdin
      pfds[0].events = POLLIN; // Report ready to read on incoming connection
      pfds[1].fd = sockfd;
      pfds[1].events = POLLIN;

      long int sequence = 0; //sequence number
      long int LAR;//last ack Recieved
      long int LFS;//last frame sent
      LAR = 0; //Last Ack Receieved
      LFS = 0; //Last Frame Sent
      int window = 5; //window size for sliding window
      int size = window + 1; //size for buffer
      Packet* buffer[size]; //buffer to store sent packets
      int buffer_index = 0; //current buffer index to store sent packets
      int i;
      int LAR_index = 0; //index in buffer for LAR
      int LFS_index = 0; //index in buffer for LFS
      Packet packet_recv; //packet to receive data in
      packet_recv.control = 1; //set control for while loop
      Packet temp; //temporary packet to memcpy from upon receive
      int packet_size; //sizeof isn't working for packet: variable used instead

      /*------------------------------------------------------------------ */
      while(packet_recv.control != 2){
            poll(pfds, 2, -1);
            Packet* packet_send = new Packet;
            if (pfds[1].revents & POLLIN){//IF STATEMENT ACCOUNTS FOR RECEIVING AN ACK
                                          // WHEN IT FAILED THE FIRST TIME
              int numbytes = recvfrom(sockfd, &packet_recv, sizeof packet_recv, 0,
                                      (struct sockaddr *)&recv_addr, &addr_len);
              if (numbytes == -1){
                perror("recvfrom");
                exit(1);
              }

              //convert to host order
              packet_recv.seqnum = ntohl(packet_recv.seqnum);
              packet_recv.length = ntohs(packet_recv.length);

              printf("Packet Recieved: %ld, %d, %d, %d\n", packet_recv.seqnum, packet_recv.ACK,
              packet_recv.control, packet_recv.length);

              //check if later ACKs were already received
              for (i = LAR_index; i <= LFS_index; i++){
                if (ntohl((*(buffer[i])).seqnum) > LAR){
                  LAR = ntohl((*(buffer[i])).seqnum);
                  LAR_index = (LAR % window)-1;
                }
             }
           }

           //Receive an ACK
           if (pfds[0].revents & POLLIN){
              fgets(temp.data, 1024, stdin);
              sequence++;
              temp.seqnum = htonl(sequence);
              temp.ACK = 0;
              temp.control = 0;

              //Check if trying to quit program
              if (strncmp(temp.data, "EXIT\n", 6) == 0){
                temp.control = 2;
              }

              temp.length = htons(strnlen(temp.data,1024));
              memcpy(packet_send, &temp, sizeof temp);
              buffer_index = (ntohl(packet_send->seqnum) % window)-1;//convert sequence number to index in buffer
              buffer[buffer_index] = packet_send;//store packet before sending
              printf("Sending: %d, %d, %d, %d, %s\n", ntohl(packet_send->seqnum), packet_send->ACK,
                        packet_send->control, ntohs(packet_send->length), packet_send->data);
              packet_size = sizeof temp.seqnum + sizeof temp.ACK + sizeof temp.control + sizeof temp.length + sizeof temp.data;//sizeof not working on packet_send
              int bytes_sent = sendto(sockfd, packet_send, packet_size, 0,
                                  (struct sockaddr *)&recv_addr, addr_len);
              if (bytes_sent == -1) {
                  perror("sendto");
                exit(1);
               }

              //update last frame sent
              LFS++;
              LFS_index = (LFS % window)-1;
              int wait = poll(pfds, 2, 5000); //Poll for 5 seconds
              if (wait == 0){
                if ((bytes_sent = sendto(sockfd, packet_send, sizeof packet_send, 0,
                    (struct sockaddr *)&recv_addr, addr_len)) == -1) {
                      perror("sendto");
                   exit(1);
                 }
              }
              else if (pfds[1].revents & POLLIN){
                int numbytes = recvfrom(sockfd, &packet_recv, sizeof packet_recv, 0,
                                        (struct sockaddr *)&recv_addr, &addr_len);
                if (numbytes == -1){
                  perror("recvfrom");
                  exit(1);
                }

                //convert to host order
                packet_recv.seqnum = ntohl(packet_recv.seqnum);
                packet_recv.length = ntohs(packet_recv.length);
                printf("Packet Received: %ld, %d, %d, %d\n", packet_recv.seqnum, packet_recv.ACK,
                          packet_recv.control,packet_recv.length);

                //check if later ACKs were already received
                for (i = LAR_index; i <= LFS_index; i++){
                  if (ntohl((*(buffer[i])).seqnum) > LAR){
                    LAR = ntohl((*(buffer[i])).seqnum);
                    LAR_index = (LAR % window)-1;
                  }
                }
            }
          }
        //}

      }
      printf("Connection closed\n");
    }
};


  class Receiver{
      public:
        int sockfd;
        sockaddr_in sender_addr;
        socklen_t addr_len;

      //Methods
        void setup_connection(){
          struct addrinfo hints, *server_info, *ptr;

          memset(&hints, 0, sizeof hints);
        	hints.ai_family = AF_INET; // IPv4
        	hints.ai_socktype = SOCK_DGRAM;
        	hints.ai_protocol = 0;
        	hints.ai_flags = AI_PASSIVE; //my IP adddress

          int status = getaddrinfo(NULL, SERVER_PORT, &hints, &server_info);
          if (status != 0)
          {
            fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
            exit(1);
          }

          for(ptr = server_info; ptr != NULL; ptr = ptr->ai_next) {
        		if ((sockfd = socket(ptr->ai_family, ptr->ai_socktype,
        						ptr->ai_protocol)) == -1) {
        			perror("listener: socket");
        			continue;
        		 }

        		if (bind(sockfd, ptr->ai_addr, ptr->ai_addrlen) == -1) {
        			close(sockfd);
        			perror("listener: bind");
        			continue;
        		 }

        		break;
        	}

          // Make sure it binded
          if (ptr == NULL)  {
            fprintf(stderr, "server: failed to bind\n");
            exit(1);
          }

          freeaddrinfo(server_info);

          printf("Chat Server: waiting for connections...\n");
          /* ===================================================================================*/
          // Setting up connection

          //initialize packet
          Packet packet_recv;

          int numbytes = recvfrom(sockfd, &packet_recv, MAXBUFLEN-1, 0,
               (struct sockaddr *)&sender_addr, &addr_len);
          if (numbytes == -1){
             perror("recvfrom");
             exit(1);
            }

          //convert to host order
          packet_recv.seqnum = ntohl(packet_recv.seqnum);
          packet_recv.length = ntohs(packet_recv.length);

          //check if packet is valid
          if ((packet_recv.seqnum == 0) & (packet_recv.ACK == 0) &
              (packet_recv.control == 1) & (packet_recv.length == 0)){
            printf("Server: connection setup successful \n");
          }

          //send confirmation packet back
          int bytes_sent;
          Packet packet_send;
          packet_send.seqnum = 0;
          packet_send.ACK = 1;
          packet_send.control = 1;
          packet_send.length = 0;

          packet_send.seqnum = htonl(packet_send.seqnum);
          packet_send.length = htons(packet_send.length);

          bytes_sent = sendto(sockfd, &packet_send, sizeof packet_send, 0,
                                    (struct sockaddr *)&sender_addr, addr_len);
          if (bytes_sent == -1){
            perror("send");
          }
            // End setup process
          /* ===================================================================================*/
        }
      void conversation(){
        /* ======================================================*/
        // Receive message and send ACK

        int bytes_sent;
        int window = 5; //window size
        int size = window + 1; //buffer size
        Packet* buffer[size]; //buffer to store recieved messages in
        long int NFE = 1;//next frame expected
        long int LFA; //last frame acceptable
        LFA = (NFE + window);
        int buffer_index;//current index for circular buffer
        int i;
        Packet packet_send;//packet to store sending values
        Packet temp;//temporary packet to store received packet
        packet_send.control = 1;//initialize for while loop

        while(packet_send.control != 2){
          /* ======================================================*/
          //Receive message
          Packet* packet_recv = new Packet;
          int numbytes = recvfrom(sockfd, &temp, MAXBUFLEN-1, 0,
               (struct sockaddr *)&sender_addr, &addr_len);
          if (numbytes == -1){
             perror("recvfrom");
             exit(1);
            }
          printf("Temp Received: %d, %d, %d, %d, %s\n", ntohl(temp.seqnum), temp.ACK,
                    temp.control, ntohs(temp.length), temp.data);
          memcpy(packet_recv, &temp, sizeof temp);

          //convert to host order
          packet_recv->seqnum = ntohl(packet_recv->seqnum);
          packet_recv->length = ntohs(packet_recv->length);

          printf("Received: %ld, %d, %d, %d, %s\n", packet_recv->seqnum, packet_recv->ACK,
                    packet_recv->control, packet_recv->length, packet_recv->data);

          //if packet ACK needs to be resent
          if (packet_recv->seqnum < NFE){
            packet_send.seqnum = htonl(packet_recv->seqnum);
            packet_send.ACK = 1;
            packet_send.control = packet_recv->control;
            packet_send.length = htons(0);
            printf("Resending: %d, %d, %d, %d, %s\n", ntohl(packet_send.seqnum), packet_send.ACK,
                      packet_send.control, packet_send.length, packet_send.data);
            bytes_sent = sendto(sockfd, &packet_send, sizeof packet_send, 0,
                                (struct sockaddr *)&sender_addr, addr_len);
          }
          else if ((packet_recv->seqnum >= NFE) & (packet_recv->seqnum <= LFA)){//if packet is within window
        /* ===========================================================*/
        // send Ack to client
            buffer_index = (packet_recv->seqnum % window)-1;//find index in buffer to store packet in
            buffer[buffer_index] = packet_recv;//store packet
            if (packet_recv->seqnum == NFE){//if packet is the next one expected
              packet_send.seqnum = NFE;
              NFE++;
              for (i = buffer_index+1; i < LFA; i++){
                if ((*(buffer[i%size])).seqnum == NFE){//check if you need to ACK for more than one packet
                  packet_send.seqnum = NFE;
                  NFE++;
                }
                else{//if done incrementing NFE and going through buffer (out of later consecutive ACKS)
                  break;
                }
              }
            }
            LFA = NFE + window;//update last frame acceptable

            //prepare packet to send back
            packet_send.seqnum = htonl(packet_send.seqnum);
            packet_send.ACK = 1;
            packet_send.control = packet_recv->control;
            packet_send.length = htons(0);

            if (packet_recv->control == 2){//if sender says to quit
              packet_send.control = 2;
            }


            printf("Sending: %d, %d, %d, %d\n", ntohl(packet_send.seqnum), packet_send.ACK,
                      packet_send.control, packet_send.length);

            bytes_sent = sendto(sockfd, &packet_send, sizeof packet_send, 0,
                                      (struct sockaddr *)&sender_addr, addr_len);

            if (bytes_sent == -1){
              perror("send");
            }
          }
        }
        printf("Connection closed\n");
        }
    };
