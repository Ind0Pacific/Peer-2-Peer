#include "functions.cpp"
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

int main() {

  int serverSocketFD = createTCPSocket();
  struct sockaddr_in serveraddress = createIPv4Address("", 2000);
  int result = bind(serverSocketFD, (struct sockaddr *)&serveraddress, sizeof(serveraddress));
  if (result == 0)
    std::cout << "[+] Server is bound to port 2000\n";
  else {
    std::cout << "[-] Bind failed\n";
    return -1;
  }
  listen(serverSocketFD, 10);
  std::cout << "[*] Waiting for connections...\n";
  while (true) {
    struct AcceptedClients *clientNode = acceptNewConnection(serverSocketFD);
    if (!clientNode->acceptedSuccessfully) {
      delete clientNode;
      continue;
    }
    std::thread clientThread(receiveAndPrintIncomingData, clientNode);
    clientThread.detach();
  }
  shutdown(serverSocketFD, SHUT_RDWR);
  return 0;
}