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
// #define window 5

bool error(int chance){ // produce an error with probability 1/chance
return (rand() < (RAND_MAX / chance));
}


struct Packet{
  long int seqnum;
  char ACK;
  char control;
  short length;
  char data[1024];
};

class Sender{
  public:
    //Packet packet_send;
    Packet packet_recv;
    int sockfd;
    sockaddr recv_addr;
    socklen_t addr_len;

    long int LAR;//last ack Recieved
    long int LFS;//last frame sent
    Packet* buffer;
    double rtt;


    //Methods
    int setup_connection(char* hostname){
      struct addrinfo hints, *server_info, *ptr;
      //int sockfd;

      memset(&hints, 0, sizeof hints);
      hints.ai_family = AF_INET; // IPv4
      hints.ai_socktype = SOCK_DGRAM;
      hints.ai_protocol = 0;
      hints.ai_flags = AI_PASSIVE; //my IP adddress

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

      //ptr = server_info;

      for(ptr = server_info; ptr != NULL; ptr = ptr->ai_next) {
    		if ((sockfd = socket(ptr->ai_family, ptr->ai_socktype,
    				ptr->ai_protocol)) == -1) {
          printf("sockfd: %d\n", sockfd);
    			perror("talker: socket");
    			continue;
    		}
    		break;
    	}

      if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr) == -1) {
        close(sockfd);
        perror("listener: bind");
        }

      // Make sure it        int window = 5; binded
      if (ptr == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
      }
      /* ===================================================================================*/
      //Setting up connection
      Packet packet_send;
      packet_send.seqnum = 0;
      packet_send.ACK = 0;
      packet_send.control = 1;
      packet_send.length = 0;

      packet_send.seqnum = htonl(packet_send.seqnum);
      packet_send.length = htons(packet_send.length);

      int bytes_sent = sendto(sockfd, &packet_send, sizeof packet_send, 0, ptr->ai_addr,
                              ptr->ai_addrlen);
      if (bytes_sent == -1){
        perror("send");
      }

      int numbytes = recvfrom(sockfd, &packet_recv, sizeof packet_send, 0,
                              ptr->ai_addr, &ptr->ai_addrlen);
      if (numbytes == -1){
         perror("recvfrom");
         exit(1);
        }
       packet_recv.seqnum = ntohl(packet_recv.seqnum);
       packet_recv.length = ntohs(packet_recv.length);

       recv_addr = *ptr->ai_addr;
       addr_len = ptr->ai_addrlen;


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
    void conversation(){
      struct pollfd pfds[2];
      pfds[0].fd = 0; //stdin
      pfds[0].events = POLLIN; // Report ready to read on incoming connection
      pfds[1].fd = sockfd;
      pfds[1].events = POLLIN;

      long int sequence = 0;
      LAR = 0; //Last Ack Receieved
      LFS = 0; //Last Frame Sent
      int window = 5;
      Packet* buffer[window+1];
      int buffer_index=0;
      int i;
      int LAR_index = 0;
      int LFS_index = 0;

      /*------------------------------------------------------------------ */
      //trying sometihnh new
      while(packet_recv.control != 2){
        while (buffer[window+1] != NULL){ // This probably isnt right
            poll(pfds, 2, -1);
            Packet* packet_send = new Packet;
            if (pfds[1].revents & POLLIN){
              //THIS IF STATEMENT ACCOUNTS FOR RECEIVING AN ACK
              // WHEN IT FAILED THE FIRST TIME!
              int numbytes = recvfrom(sockfd, &packet_recv, sizeof packet_send, 0,
                                      (struct sockaddr *)&recv_addr, &addr_len);
              if (numbytes == -1){
                perror("recvfrom");
                exit(1);
              }
              packet_recv.seqnum = ntohl(packet_recv.seqnum);
              packet_recv.length = ntohs(packet_recv.length);
              printf("Packet Recieved: %ld, %d, %d, %d\n", packet_recv.seqnum, packet_recv.ACK,
              packet_recv.control, packet_recv.length);
              for (i = LAR_index; i <= LFS_index; i++){
                if (ntohl((*(buffer[i])).seqnum) == LAR+1){
                  LAR++;
                  LAR_index = (LAR % window)-1;
                }
                else{
                  break;
                }
             }
             printf("1-LAR: %ld\n", LAR);
             printf("1-LFS: %ld\n", LFS);
           }
           if (pfds[0].revents & POLLIN){
              fgets(packet_send->data, 1024, stdin);
              sequence++;
              packet_send->seqnum = htonl(sequence);
              packet_send->ACK = 0;
              packet_send->control = 0;
              packet_send->length = htons(strnlen(packet_send->data,1024));
              buffer_index = (ntohl(packet_send->seqnum) % window)-1;
              buffer[buffer_index] = packet_send;
              for (i=0; i < buffer_index+1; i++){
                printf("buffer[%d]: %d\n",i, ntohl((*(buffer[i])).seqnum));
              }
              int bytes_sent = sendto(sockfd, &packet_send, sizeof packet_send, 0,
                                  (struct sockaddr *)&recv_addr, addr_len);
              if (bytes_sent == -1) {
                  perror("sendto");
                exit(1);
               }
              LFS++;
              LFS_index = (LFS % window)-1;
              int wait = poll(pfds, 2, 5000); //Wait 5 seconds
              if (wait == 0){
                printf("Did not receive ACK --> Resend msg \n");
                if ((bytes_sent = sendto(sockfd, &packet_send, sizeof packet_send, 0,
                    (struct sockaddr *)&recv_addr, addr_len)) == -1) {
                      perror("sendto");
                   exit(1);
                 }
              }
              else if (pfds[1].revents & POLLIN){
                int numbytes = recvfrom(sockfd, &packet_recv, sizeof packet_send, 0,
                                        (struct sockaddr *)&recv_addr, &addr_len);
                if (numbytes == -1){
                  perror("recvfrom");
                  exit(1);
                }
                packet_recv.seqnum = ntohl(packet_recv.seqnum);
                packet_recv.length = ntohs(packet_recv.length);
                printf("Packet Received: %ld, %d, %d, %d\n", packet_recv.seqnum, packet_recv.ACK,
                          packet_recv.control,packet_recv.length);

                for (i = LAR_index; i <= LFS_index; i++){
                  printf("buffer[%d]: %d\n", i, ntohl((*(buffer[i])).seqnum));
                  if (ntohl((*(buffer[i])).seqnum) > LAR){
                    LAR = ntohl((*(buffer[i])).seqnum);
                    LAR_index = (LAR % window)-1;
                  }
                }
                printf("2-LAR: %ld\n", LAR);
                printf("2-LFS: %ld\n", LFS);
            }
          }
        }
      }
    }
};
  class Receiver{
      public:
        Packet packet_recv;
        int sockfd;
        sockaddr_in sender_addr;
        socklen_t addr_len;
        Packet packet_send;

        //Methods
        void setup_connection(){
          struct addrinfo hints, *server_info, *ptr;
          //int sockfd;

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
          //Packet packet_recv;

          int numbytes = recvfrom(sockfd, &packet_recv, MAXBUFLEN-1, 0,
               (struct sockaddr *)&sender_addr, &addr_len);
          if (numbytes == -1){
             perror("recvfrom");
             exit(1);
            }
          packet_recv.seqnum = ntohl(packet_recv.seqnum);
          packet_recv.length = ntohs(packet_recv.length);
        //   printf("%ld, %d, %d, %d\n", packet_recv.seqnum, packet_recv.ACK,
        // packet_recv.control,packet_recv.length);

          if ((packet_recv.seqnum == 0) & (packet_recv.ACK == 0) &
              (packet_recv.control == 1) & (packet_recv.length == 0)){
            printf("Server: connection setup successful \n");
          }

          int bytes_sent;
      //  Packet packet_send;
          packet_send.seqnum = 0;
          packet_send.ACK = 1;
          packet_send.control = 1;
          packet_send.length = 0;

          packet_send.seqnum = htonl(packet_send.seqnum);
          packet_send.length = htons(packet_send.length);

          //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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
        struct pollfd pfds[1];
        // Add the listener to set
        long int last_seqnum = packet_recv.seqnum;
        int bytes_sent;
        int window = 5;
        long int NFE = 1;//next frame expected
        long int LFA; //last frame acceptable
        Packet* buffer[window+1];
        LFA = (NFE + window)-1;
        int buffer_index;
        int i;

        while(packet_send.control != 2){
          /* ======================================================*/
          //Receive message
          int numbytes = recvfrom(sockfd, &packet_recv, MAXBUFLEN-1, 0,
               (struct sockaddr *)&sender_addr, &addr_len);
          if (numbytes == -1){
             perror("recvfrom");
             exit(1);
            }
          packet_recv.seqnum = ntohl(packet_recv.seqnum);
          packet_recv.length = ntohs(packet_recv.length);
          printf("Received: %ld, %d, %d, %d, %s", packet_recv.seqnum, packet_recv.ACK,
                    packet_recv.control,packet_recv.length, packet_recv.data);
          if (packet_recv.seqnum < NFE){
            packet_send.seqnum = htonl(packet_recv.seqnum);
            packet_send.ACK = 1;
            packet_send.control = 0;
            packet_send.length = htons(0);
            printf("Resending: %d, %d, %d, %d\n", ntohl(packet_send.seqnum), packet_send.ACK,
                      packet_send.control, packet_send.length);
            //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            bytes_sent = sendto(sockfd, &packet_send, sizeof packet_send, 0,
                                      (struct sockaddr *)&sender_addr, addr_len);
          }
          else if ((packet_recv.seqnum >= NFE) & (packet_recv.seqnum <= LFA)){
        /* ===========================================================*/
        // send Ack to client
            buffer_index = (packet_recv.seqnum % window)-1;
            buffer[buffer_index] = &packet_recv;
            if (packet_recv.seqnum == NFE){
              packet_send.seqnum = NFE;
              NFE++;
              for (i = buffer_index+1; i < buffer_index+window; i++){
                if ((*(buffer[i])).seqnum == NFE){
                  printf("SENDING MORE THAN 1 ACK");
                  packet_send.seqnum = NFE;
                  NFE++;
                }
                else{
                  break;
                }
              }
            }
            LFA = NFE + window;
            printf("packet_send seqnum: %ld\n", packet_send.seqnum);
            packet_send.seqnum = htonl(packet_send.seqnum);
            packet_send.ACK = 1;
            packet_send.control = 0;
            packet_send.length = htons(0);

            if (packet_recv.control == 2){
              packet_send.control = 2;
            }

            if (ntohl(packet_send.seqnum) % 2 != 0){ // lose a packet 1/4 of the time

              printf("Sending: %d, %d, %d, %d\n", ntohl(packet_send.seqnum), packet_send.ACK,
                        packet_send.control, packet_send.length);

              // std::chrono::seconds dura( 7); //test resending
              // std::this_thread::sleep_for( dura );
              //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
              bytes_sent = sendto(sockfd, &packet_send, sizeof packet_send, 0,
                                        (struct sockaddr *)&sender_addr, addr_len);
                                        //sockfd, &packet_send, sizeof packet_send, 0,
                                          //ptr->ai_addr,ptr->ai_addrlen
              if (bytes_sent == -1){
                perror("send");
              }
            }
            else{
              printf("lol packet %d lost that's awkward\n", ntohl(packet_send.seqnum));
            }
          }
          printf("NFE: %ld\n", NFE);
        }
        }
    };
