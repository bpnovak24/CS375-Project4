#include "ARQ.h"

int main(int argc, char **argv){
  // Receiver receiver;
  // receiver.setup_connection();
  Sender sender;
  int status = sender.setup_connection(argv[1]);
  if (status == 0){
    printf("Unable to successfully connect to host\n");
    exit(1);
  }
return 0;

}
