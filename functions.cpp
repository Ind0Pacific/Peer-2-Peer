#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

struct AcceptedClients{
  int acceptedSocketsFD;
  struct sockaddr_in address;
  int error;
  bool acceptedSuccessfully;
};

int createTCPSocket() {
   int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (fd < 0) {
    std::cerr << "[-] Socket creation failed\n";
    return -1;
  }
   std::cout << "[+] Socket created successfully\n";
   return fd;
}

struct sockaddr_in createIPv4Address(const char *ip, int port) {
   struct sockaddr_in address;
   std::memset(&address, 0, sizeof(address));
   address.sin_family = AF_INET;
   address.sin_port = htons(port);
   address.sin_addr.s_addr;

   if (strlen(ip) == 0) {
    address.sin_addr.s_addr = INADDR_ANY;
    return address;
 } else {
   inet_pton(AF_INET, ip, &address.sin_addr.s_addr);
   return address;
  }
}

struct AcceptedClients* acceptNewConnection(int serverSocketFD) {

   struct AcceptedClients* client = new AcceptedClients;
  
   socklen_t clientAddressSize = sizeof(client->address);
   client->acceptedSocketsFD = accept(serverSocketFD, (struct sockaddr *)&client->address, &clientAddressSize);
 
   if (client->acceptedSocketsFD < 0) {
    std::cout << "[-] Failed to accept client connection\n";
    client->error = client->acceptedSocketsFD;
    client->acceptedSuccessfully = false;
  } else {
    std::cout << "[+] Client connected successfully!\n";
    client->error = 0;
    client->acceptedSuccessfully = true;
  }
   return client;
}

void receiveAndPrintIncomingData(struct AcceptedClients *clientNode) {
  char buffer[4096];

  std::memset(buffer , 0 , sizeof(buffer));
  recv(clientNode ->acceptedSocketsFD, buffer, 4096, 0);
  std::string userName = buffer;
  std::cout << "[+] " << userName << " has joined the server.\n";

  while (true) {
    std::memset(buffer, 0, sizeof(buffer));
    ssize_t data_Recived = recv(clientNode->acceptedSocketsFD, buffer, 4096, 0);
    
    if (data_Recived > 0) {
      std::cout << "[" << userName << "]: " << buffer << std::endl;
    } else {
      std::cout << "[!] " << userName << " (FD: " << clientNode->acceptedSocketsFD << ") has exited the chat.\n";
      break; 
    }
  }
  close(clientNode->acceptedSocketsFD);
  delete clientNode;
}