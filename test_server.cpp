#include "ARQ.h"

bool error(int chance) // produce an error with probability 1/chance
{
return (rand() < (RAND_MAX / chance));
}

int main(int argc, char **argv){
  Receiver receiver;
  receiver.setup_connection();
  receiver.conversation();
  
  // Sender sender;
  // int status = sender.setup_connection(argv[1]);
  // if (status == 0){
  //   printf("Unable to successfully connect to host\n");
  //   exit(1);
  // }
return 0;

}
