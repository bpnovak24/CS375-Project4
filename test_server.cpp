#include "ARQ.h"


int main(int argc, char **argv){
  Receiver receiver;
  receiver.setup_connection();
  receiver.conversation();
  return 0;
}
