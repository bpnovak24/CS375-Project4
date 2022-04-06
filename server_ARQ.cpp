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
#include <poll.h>
#include <signal.h>

#define MAXBUFLEN 100 // max buffer length, measured in bytes
#define MYPORT "8080"	// the port users will be connecting to
#define SERVERPORT 8080
#define BACKLOG 10

struct Packet{
  int seqnum;
  char ACK;
  char control;
  short length;
  char data[1024];
};

int main(void)
{
  struct addrinfo hints, *server_info, *ptr;
  int sockfd;

  memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6; // IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_PASSIVE; //my IP adddress

  int status;
  if ((status == getaddrinfo(NULL, MYPORT, &hints, &server_info)) != 0)
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

  // char hostname[1024];
  // hostname[1023] = '\0';
  // gethostname(hostname, 1023);
  // printf("Hostname: %s\n", hostname);

  printf("Chat Server: waiting for connections...\n");
  /* ===================================================================================*/
  // Setting up connection
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof client_addr;
  char client[INET_ADDRSTRLEN];
  Packet setup_packet_recv;

  int numbytes = recvfrom(sockfd, &setup_packet_recv, MAXBUFLEN-1, 0,
       (struct sockaddr *)&client_addr, &addr_len);
  if (numbytes == -1){
     perror("recvfrom");
     exit(1);
    }
  setup_packet_recv.seqnum = ntohl(setup_packet_recv.seqnum);
  setup_packet_recv.ACK = ntohs(setup_packet_recv.ACK);
  setup_packet_recv.control = ntohs(setup_packet_recv.control);
  setup_packet_recv.length = ntohs(setup_packet_recv.length);
  printf("%d, %d, %d, %d\n", setup_packet_recv.seqnum, setup_packet_recv.ACK,
setup_packet_recv.control, setup_packet_recv.length);

  if ((setup_packet_recv.seqnum == 0) & (setup_packet_recv.ACK == 0) &
      (setup_packet_recv.control == 1) & (setup_packet_recv.length == 0)){
    printf("Server: connection setup successful \n");
  }

  Packet setup_packet_send;
  setup_packet_send.seqnum = 0;
  setup_packet_send.ACK = 0;
  setup_packet_send.control = 1;
  setup_packet_send.length = 0;

  setup_packet_send.seqnum = htonl(setup_packet_send.seqnum);
  setup_packet_send.ACK = htonl(setup_packet_send.ACK);
  setup_packet_send.control = htonl(setup_packet_send.control);
  setup_packet_send.length = htonl(setup_packet_send.length);

  int bytes_sent = sendto(sockfd, &setup_packet_send, 0, 0,
                            (struct sockaddr *)&client_addr, addr_len);
  if (bytes_sent == -1){
    perror("send");
  }
    // End setup process
  /* ===================================================================================*/

}
