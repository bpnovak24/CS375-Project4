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
  int seqnum;
  char ACK;
  char control;
  short length;
  char data[1024];
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

    struct sockaddr sender_addr;
    socklen_t addr_size;
    struct addrinfo hints, *server_info, *ptr;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE; //my IP adddress

    int status;
    status == getaddrinfo(argv[1], SERVER_PORT, &hints, &server_info);
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
   printf("Finished \n");
    /* ===================================================================================*/
    //Setting up connection
    Packet setup_packet_send;
    setup_packet_send.seqnum = 0;
    setup_packet_send.ACK = 0;
    setup_packet_send.control = 1;
    setup_packet_send.length = 0;

    setup_packet_send.seqnum = htons(setup_packet_send.seqnum);
    setup_packet_send.ACK = htons(setup_packet_send.ACK);
    setup_packet_send.control = htons(setup_packet_send.control);
    setup_packet_send.length = htons(setup_packet_send.length);

    int bytes_sent = sendto(sockfd, &setup_packet_send, 0, 0, ptr->ai_addr,
                            ptr->ai_addrlen);
    if (bytes_sent == -1){
      perror("send");
    }

    Packet setup_packet_recv;

    int numbytes = recvfrom(sockfd, &setup_packet_recv, MAXBUFLEN-1, 0,
                            ptr->ai_addr, &ptr->ai_addrlen);
    if (numbytes == -1){
       perror("recvfrom");
       exit(1);
      }
     setup_packet_recv.seqnum = ntohs(setup_packet_recv.seqnum);
     setup_packet_recv.ACK = ntohs(setup_packet_recv.ACK);
     setup_packet_recv.control = ntohs(setup_packet_recv.control);
     setup_packet_recv.length = ntohs(setup_packet_recv.length);

     if ((setup_packet_recv.seqnum == 0) & (setup_packet_recv.ACK == 0) &
          (setup_packet_recv.control == 1) & (setup_packet_recv.length == 0)){
        printf("Client: connection setup successful \n");
      }

    /* ===================================================================================*/
    // Setting up connection
    return 0;
  }
