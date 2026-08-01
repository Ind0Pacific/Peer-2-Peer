#include "functions.cpp"
#include <iostream>
#include <istream>
#include <string>
#include <sys/socket.h>
#include <thread>

using namespace std;
int main()
{

  string clientName;
  string targetIP;
  int targetPort;
  string message_send;
  cout << "Enter the IP and port (e.g., 127.0.0.1 2000): ";
  cin >> targetIP >> targetPort;

  int clientFD = createTCPSocket();
  struct sockaddr_in address = createIPv4Address(targetIP.c_str(), targetPort);
  int result = connect(clientFD, (struct sockaddr *)&address, sizeof address);

  if (result < 0)
  {
    cout << ("Connection failed");
    return -1;
  }

  cout << "Enter your display name: ";
  getline(cin >> ws, clientName);
  send(clientFD, clientName.c_str(), clientName.length(), 0);

  cout << "[*] Negotiating UID with server...\n";
  char buffer[4096];
  memset(buffer, 0, sizeof(buffer));
  recv(clientFD, buffer, 4096, 0);

  string serverResponse = buffer;
  if (serverResponse.find("[UID_ASSIGNED] ") == 0)
  {
    string assignedName = serverResponse.substr(15);

    if (assignedName != clientName)
    {
      cout << "[!] Name taken. Server assigned you the UID: " << assignedName << "\n";
      clientName = assignedName;
    }
    else
    {
      cout << "[+] Connected successfully as: " << clientName << "\n";
    }
  }
  cout << "[+] Connected! Type 'exit' to quit.\n";
  loadChatHistory(clientName, targetIP);

  thread listener(recevieMessages, clientFD);
  listener.detach();

  while (true)
  {
    cout << "Enter message: ";
    getline(cin >> ws, message_send);
    if (message_send == "exit")
    {
      cout << "[-] Closing connection...\n";
      break;
    }
    else
    {
      send(clientFD, message_send.c_str(), message_send.length(), 0);
    }
  }
  close(clientFD);
  shutdown(clientFD, SHUT_RDWR);
  return 0;
}