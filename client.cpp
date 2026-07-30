#include "functions.cpp"
#include <cstring>
#include <iostream>
#include <istream>
#include <string>
#include <sys/socket.h>

using namespace std;
int main() {

  string clientName;
  string targetIP;
  int targetPort;
  string message_send;
  cout << "Enter the IP and port (e.g., 127.0.0.1 2000): ";
  cin >> targetIP >> targetPort;

  int clientFD = createTCPSocket();
  struct sockaddr_in address = createIPv4Address(targetIP.c_str(), targetPort);
  int result = connect(clientFD, (struct sockaddr *)&address, sizeof address);

  if (result < 0) {
    cout << ("Connection failed");
    return -1;
  }
  cout << "Enter your name: ";
  getline(cin >> ws, clientName);
  send(clientFD, clientName.c_str(), clientName.length(), 0);
  cout << "[+] Connected! Type 'exit' to quit.\n";
  loadChatHistory(clientName, targetIP);

  while (true) {
    cout << "Enter message: ";
    getline(cin >> ws, message_send);
    if (message_send == "exit") {
      cout << "[-] Closing connection...\n";
      break;
    } else{
      send(clientFD, message_send.c_str(), message_send.length(), 0);
    }  
  }
  char buffer[4096];
  memset(buffer, 0, sizeof(buffer));
  recv(clientFD, buffer, 4096, 0);
  cout << "Response is " << buffer << endl;

  close(clientFD);
  shutdown(clientFD, SHUT_RDWR);
  return 0;
}