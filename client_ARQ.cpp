#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>


#define SERVER_PORT "8080"	// the port users will be connecting to
#define MYPORT 8080
#define MAXBUFLEN 1034


struct Packet{
  long int seqnum;
  char ACK;
  char control;
  short length;
  char data[1024];//help
};

int main(int argc, char **argv)
{
  if (argc < 2)
    {
      printf("%d",argc);
      fprintf(stderr, "client: You must specify the server hostname on the command line.");
      exit(1);
    }
    // 3 because we need the function (./client), hostname, and client's username
    if (argc != 3)
  {
  fprintf(stderr, "usage: talker hostname message\n"); // file printf -> print to file
  exit(1);
  }

    struct addrinfo hints, *server_info, *ptr;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE; //my IP adddress

    int status;
    status = getaddrinfo(argv[1], SERVER_PORT, &hints, &server_info);
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

    // Make sure it binded
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

    Packet packet_recv;

    int numbytes = recvfrom(sockfd, &packet_recv, sizeof packet_send, 0,
                            ptr->ai_addr, &ptr->ai_addrlen);
    if (numbytes == -1){
       perror("recvfrom");
       exit(1);
      }
     packet_recv.seqnum = ntohs(packet_recv.seqnum);
     packet_recv.length = ntohs(packet_recv.length);

     if ((packet_recv.seqnum == 0) & (packet_recv.ACK == 1) &
          (packet_recv.control == 1) & (packet_recv.length == 0)){
        printf("Client: connection setup successful \n");
    }
    /* ===================================================================================*/


    struct pollfd pfds[2];
    pfds[0].fd = 0; //stdin
    pfds[0].events = POLLIN; // Report ready to read on incoming connection
    pfds[1].fd = sockfd;
    pfds[1].events = POLLIN;

    int sequence = 0;

    while(1){
      poll(pfds, 2, -1); // wait indefinitely
      if (pfds[0].revents & POLLIN){
        //while((packet_recv.seqnum != sequence) & (packet_recv.ACK != 1)){
        while(1){
          fgets(packet_send.data, 1024, stdin);
          sequence++;
          packet_send.seqnum = htonl(sequence);
          packet_send.ACK = 0;
          packet_send.control = 0;
          packet_send.length = htons(strlen(packet_send.data));
          if ((bytes_sent = sendto(sockfd, &packet_send, sizeof packet_send, 0,
            ptr->ai_addr,ptr->ai_addrlen)) == -1) {
              perror("sendto");
            exit(1);
          }
          int rtt = poll(pfds, 2, 5000); //Wait 5 seconds
          if (rtt == 0){
            printf("Did not receive ACK --> Resend msg \n");
            if ((bytes_sent = sendto(sockfd, &packet_send, sizeof packet_send, 0,
              ptr->ai_addr,ptr->ai_addrlen)) == -1) {
                perror("sendto");
              exit(1);
            }
          }
          else if (pfds[1].revents & POLLIN){
             numbytes = recvfrom(sockfd, &packet_recv, sizeof packet_send, 0,
                                    ptr->ai_addr, &ptr->ai_addrlen);
            if (numbytes == -1){
               perror("recvfrom");
               exit(1);
              }
             packet_recv.seqnum = ntohl(packet_recv.seqnum);
             packet_recv.length = ntohs(packet_recv.length);
             printf("%ld, %d, %d, %d\n", packet_recv.seqnum, packet_recv.ACK,
                      packet_recv.control,packet_recv.length);
             printf("ACK received!\n");
          }
      }
     }
   }
    return 0;
  }
