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


#define SERVER_PORT "8080"	// the port users will be connecting to
#define MYPORT 8080
#define MAXBUFLEN 100

struct sockaddr sender_addr;
struct addrinfo hints, *server_info, *ptr;
int sockfd;

void cleanExit(int x)
{
  // char *msg = "EXIT\n";
  // int bytes_sent;
  // if ((bytes_sent = sendto(sockfd, msg, MAXBUFLEN-1 , 0,
  //   ptr->ai_addr,ptr->ai_addrlen)) == -1) {
  //   perror("sendto");
  //   exit(1);
  // }
  printf("\nShutting down...\n");

  exit(0);
}

int controlC_quit()
{
	  struct sigaction sa;
	  sa.sa_handler = cleanExit;        // set the handler function
    sigemptyset(&sa.sa_mask);         // no signals will be blocked in Handler
    sa.sa_flags = SA_RESTART;         // restart a system call on signal

    if (sigaction(SIGINT, &sa, NULL) == -1)    // set the action for SIGINT
    {
        perror("sigaction");
        exit(1);
    }

    return 0;
}

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
    if ((status == getaddrinfo(argv[1], SERVER_PORT, &hints, &server_info)) != 0)
    {
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

    int bytes_sent = sendto(sockfd, argv[2], strlen(argv[2]), 0, ptr->ai_addr,
                          ptr->ai_addrlen);
    if (bytes_sent == -1){
      perror("send");
  }

}
