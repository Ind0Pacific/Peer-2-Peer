#include "functions.cpp"
#include <cstring>
#include <sys/socket.h>

int main() {

  std::string targetIP;
  int targetPort;
  std::string message_send;
  std::cout << "Enter the IP and port (e.g., 127.0.0.1 2000): ";
  std::cin >> targetIP >> targetPort;

  int clientFD = createTCPSocket();
  struct sockaddr_in address = createIPv4Address(targetIP.c_str(), targetPort);

  int result = connect(clientFD, (struct sockaddr *)&address, sizeof address);

  if (result < 0) {
    std::cout << ("Connection failed");
    return -1;
  }

  while (true) {
    std::cout << "Enter message: ";
    std::getline(std::cin >> std::ws, message_send);

    if (message_send == "exit") {
      std::cout << "[-] Closing connection...\n";
      break;
    } else
      send(clientFD, message_send.c_str(), message_send.length(), 0);
  }

  char buffer[4098];
  std::memset(buffer, 0, sizeof(buffer));

  recv(clientFD, buffer, 4098, 0);
  std::cout << "Response is " << buffer << std::endl;

  close(clientFD);
  shutdown(clientFD, SHUT_RDWR);
  return 0;
}