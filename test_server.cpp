#include "ARQ.h"


int main(int argc, char **argv){
  Receiver receiver;//initialize receiver
  receiver.setup_connection();
  receiver.conversation();//talk to the sender
  return 0;
}
