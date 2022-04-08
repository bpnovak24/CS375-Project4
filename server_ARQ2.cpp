// server.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <chrono> // Delay Function for testing
#include <thread> // Delay Function for testing
#include <poll.h>
#include <signal.h>

#define MAXBUFLEN 1034 // max buffer length, measured in bytes
#define MYPORT "8080"	// the port users will be connecting to
#define SERVERPORT 8080
#define BACKLOG 10

struct Packet{
  long int seqnum;
  char ACK;
  char control;
  short length;
  char data[1024];
};

bool error(int chance) // produce an error with probability 1/chance
{
return (rand() < (RAND_MAX / chance));
}

int main(void)
{
  struct addrinfo hints, *server_info, *ptr;
  int sockfd;

  memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_PASSIVE; //my IP adddress

  int status = getaddrinfo(NULL, MYPORT, &hints, &server_info);
  if (status != 0)
  {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    return 1;
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
  sockaddr_in client_addr;
  socklen_t addr_len = sizeof client_addr;
  char client[INET_ADDRSTRLEN];
  Packet packet_recv;

  int numbytes = recvfrom(sockfd, &packet_recv, MAXBUFLEN-1, 0,
       (struct sockaddr *)&client_addr, &addr_len);
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

  Packet packet_send;
  packet_send.seqnum = 0;
  packet_send.ACK = 1;
  packet_send.control = 1;
  packet_send.length = 0;

  packet_send.seqnum = htonl(packet_send.seqnum);
  packet_send.length = htonl(packet_send.length);

  int bytes_sent = sendto(sockfd, &packet_send, sizeof packet_send, 0,
                            (struct sockaddr *)&client_addr, addr_len);
  if (bytes_sent == -1){
    perror("send");
  }
    // End setup process
  /* ===================================================================================*/


  /* ======================================================*/
  // Receive message and send ACK
  struct pollfd pfds[1];
  // Add the listener to set
  pfds[0].fd = sockfd;
  pfds[0].events = POLLIN; // Report ready to read on incoming connection
  int pollin_happened;
  long int last_seqnum = packet_recv.seqnum;

  while(1){
    /* ======================================================*/
    //Receive message
    numbytes = recvfrom(sockfd, &packet_recv, MAXBUFLEN-1, 0,
         (struct sockaddr *)&client_addr, &addr_len);
    if (numbytes == -1){
       perror("recvfrom");
       exit(1);
      }
    packet_recv.seqnum = ntohl(packet_recv.seqnum);
    packet_recv.length = ntohs(packet_recv.length);
    printf("Recieved: %ld, %d, %d, %d, %s\n", packet_recv.seqnum, packet_recv.ACK,
              packet_recv.control,packet_recv.length, packet_recv.data);

    if (packet_recv.seqnum <= last_seqnum){
      //discard message
      printf("SEQNUM %ld already recieved: Resending ACK\n", packet_recv.seqnum);
    }
  /* ===========================================================*/
  // send Ack to client
    packet_send.seqnum = htonl(packet_recv.seqnum);
    packet_send.ACK = 1;
    packet_send.control = 0;
    packet_send.length = htons(0);
    if (!error(4)){ // lose a packet 1/4 of the time

      printf("Sending: %d, %d, %d, %d\n", ntohl(packet_send.seqnum), packet_send.ACK,
                packet_send.control, packet_send.length);

      // std::chrono::seconds dura( 7); //test resending
      // std::this_thread::sleep_for( dura );

      bytes_sent = sendto(sockfd, &packet_send, sizeof packet_send, 0,
                                (struct sockaddr *)&client_addr, addr_len);
      if (bytes_sent == -1){
        perror("send");
      }
    }
    else{
      printf("lol packet lost that's awkward\n");
    }
  }
}
