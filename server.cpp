#include "functions.cpp"

int main() {

  int serverSocketFD = createTCPSocket();
  struct sockaddr_in serveraddress = createIPv4Address("", 2000);
  int result = bind(serverSocketFD, (struct sockaddr *)&serveraddress, sizeof(serveraddress));
  if (result == 0) {
    std::cout << "Server is connected\n";
    return -1;
  }

  int listenResult = listen(serverSocketFD, 10); 

  struct sockaddr_in clientAddress;
  __socklen_t clientAddressSize = sizeof(clientAddress);
  int clientSocketFD = accept(serverSocketFD, (struct sockaddr *)&clientAddress, &clientAddressSize);

  char buffer[4098];
  while (true) {
    std::memset(buffer, 0, sizeof(buffer));
    ssize_t data_Recived = recv(clientSocketFD, buffer, 4096, 0);

    if (data_Recived > 0) std::cout << buffer << std::endl;
     else  break;
  }
  return 0;
}