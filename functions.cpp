#include <arpa/inet.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

struct AcceptedClients {
  int acceptedSocketsFD;
  struct sockaddr_in address;
  int error;
  bool acceptedSuccessfully;
};

int createTCPSocket() {
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd < 0) {
    cerr << "[-] Socket creation failed\n";
    return -1;
  }
  cout << "[+] Socket created successfully\n";
  return fd;
}

struct sockaddr_in createIPv4Address(const char *ip, int port) {
  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
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

struct AcceptedClients *acceptNewConnection(int serverSocketFD) {

  struct AcceptedClients *client = new AcceptedClients;

  socklen_t clientAddressSize = sizeof(client->address);
  client->acceptedSocketsFD = accept(
      serverSocketFD, (struct sockaddr *)&client->address, &clientAddressSize);

  if (client->acceptedSocketsFD < 0) {
    cout << "[-] Failed to accept client connection\n";
    client->error = client->acceptedSocketsFD;
    client->acceptedSuccessfully = false;
  } else {
    cout << "[+] Client connected successfully!\n";
    client->error = 0;
    client->acceptedSuccessfully = true;
  }
  return client;
}

void loadChatHistory(const string &userName, const string &ip) {
  mkdir("bin", 0777);
  mkdir("bin/chat_history", 0777);
  string filename = "bin/chat_history/" + userName + "-" + ip + ".json";
  ifstream file(filename);
  string line;

  if (file.is_open()) {
    cout << "\n--- Loading Chat History for " << userName << " ---\n";
    while (getline(file, line)) {
      size_t senderStart = line.find("\"sender\": \"");
      size_t msgStart = line.find("\"message\": \"");

      if (senderStart != string::npos && msgStart != string::npos) {
        senderStart += 11;
        size_t senderEnd = line.find("\"", senderStart);
        string sender = line.substr(senderStart, senderEnd - senderStart);

        msgStart += 12;
        size_t msgEnd = line.find("\"}", msgStart);
        string message = line.substr(msgStart, msgEnd - msgStart);
        cout << sender << ": " << message << "\n";
      }
    }
  }
  cout << "--- End of History ---\n\n";
  file.close();
}

void saveMessagesToHistory(const string &userName, const string &ip,
                           const string &sender, const string &message) {
  mkdir("bin", 0777);
  mkdir("bin/chat_history", 0777);

  string filename = "bin/chat_history/" + userName + "-" + ip + ".json";
  ofstream file(filename, ios::app);

  if (file.is_open()) {
    file << "{\"sender\": \"" << sender << "\", \"message\": \"" << message
         << "\"}\n";
    file.close();
  }
}

void receiveAndPrintIncomingData(struct AcceptedClients *clientNode) {
  char buffer[4096];
  string clientIP = inet_ntoa(clientNode->address.sin_addr);

  memset(buffer, 0, sizeof(buffer));
  recv(clientNode->acceptedSocketsFD, buffer, 4096, 0);
  string userName = buffer;
  cout << "[+] " << userName << " has joined the server from IP: " << clientIP
       << ".\n";
  loadChatHistory(userName, clientIP);

  while (true) {
    memset(buffer, 0, sizeof(buffer));
    ssize_t data_Recived = recv(clientNode->acceptedSocketsFD, buffer, 4096, 0);

    if (data_Recived > 0) {
      string message = buffer;
      cout << "[" << userName << "]: " << buffer << endl;
      saveMessagesToHistory(userName, clientIP, userName, message);
    } else {
      cout << "[!] " << userName << " (FD: " << clientNode->acceptedSocketsFD
           << ") has exited the chat.\n";
      break;
    }
  }
  close(clientNode->acceptedSocketsFD);
  delete clientNode;
}

void recevieMessages(int clientFD) {
  char buffer[4096];
  while (true) {
    memset(buffer, 0, sizeof(buffer));
    ssize_t byteRecived = recv(clientFD, buffer, 4096, 0);
    if (byteRecived > 0) {
      cout << "\n" << buffer << "\nEnter message: " << flush;
    } else {
      cout << "\n[!] Server disconnected.\n";
      exit(0);
    }
  }
}