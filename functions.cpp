#include <arpa/inet.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <map>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

const std::string RESET = "\033[0m";
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string CYAN = "\033[36m";

using namespace std;
vector<int> activeClients;
map<string, int> userSockets;
map<string, vector<int>> groupRooms;
mutex routerMutex;

struct AcceptedClients
{
  int acceptedSocketsFD;
  struct sockaddr_in address;
  int error;
  bool acceptedSuccessfully;
};

int createTCPSocket()
{
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd < 0)
  {
    cerr << "[-] Socket creation failed\n";
    return -1;
  }
  cout << "[+] Socket created successfully\n";
  return fd;
}

struct sockaddr_in createIPv4Address(const char *ip, int port)
{
  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr;

  if (strlen(ip) == 0)
  {
    address.sin_addr.s_addr = INADDR_ANY;
    return address;
  }
  else
  {
    inet_pton(AF_INET, ip, &address.sin_addr.s_addr);
    return address;
  }
}

struct AcceptedClients *acceptNewConnection(int serverSocketFD)
{
  struct AcceptedClients *client = new AcceptedClients;

  socklen_t clientAddressSize = sizeof(client->address);
  client->acceptedSocketsFD = accept(
      serverSocketFD, (struct sockaddr *)&client->address, &clientAddressSize);

  if (client->acceptedSocketsFD < 0)
  {
    cout << "[-] Failed to accept client connection\n";
    client->error = client->acceptedSocketsFD;
    client->acceptedSuccessfully = false;
  }
  else
  {
    cout << "[+] Client connected successfully!\n";
    client->error = 0;
    client->acceptedSuccessfully = true;
  }
  return client;
}

void loadChatHistory(const string &userName, const string &ip)
{
  mkdir("bin", 0777);
  mkdir("bin/chat_history", 0777);
  string filename = "bin/chat_history/" + userName + "-" + ip + ".json";
  ifstream file(filename);
  string line;

  if (file.is_open())
  {
    cout << "\n--- Loading Chat History for " << userName << " ---\n";
    while (getline(file, line))
    {
      size_t senderStart = line.find("\"sender\": \"");
      size_t msgStart = line.find("\"message\": \"");

      if (senderStart != string::npos && msgStart != string::npos)
      {
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
                           const string &sender, const string &message)
{
  mkdir("bin", 0777);
  mkdir("bin/chat_history", 0777);

  string filename = "bin/chat_history/" + userName + "-" + ip + ".json";
  ofstream file(filename, ios::app);

  if (file.is_open())
  {
    file << "{\"sender\": \"" << sender << "\", \"message\": \"" << message
         << "\"}\n";
    file.close();
  }
}

void broadcastMessage(const string &senderName, const string &message,
                      int senderSocketFD)
{
  lock_guard<mutex> lock(routerMutex);
  string formattedMessage = "[" + senderName + "]: " + message;

  for (int clientFD : activeClients)
  {
    if (clientFD != senderSocketFD)
    {
      send(clientFD, formattedMessage.c_str(), formattedMessage.length(), 0);
    }
  }
}

void receiveAndPrintIncomingData(struct AcceptedClients *clientNode)
{
  char buffer[4096];
  string clientIP = inet_ntoa(clientNode->address.sin_addr);

  memset(buffer, 0, sizeof(buffer));
  recv(clientNode->acceptedSocketsFD, buffer, 4096, 0); // handshake

  string requestedName = buffer;
  string finalName = requestedName;
  {
    lock_guard<mutex> lock(routerMutex);
    if (userSockets.find(requestedName) != userSockets.end())
    {
      int count = 1;
      while (userSockets.find(requestedName + "#" + to_string(count)) !=
             userSockets.end())
      {
        count++;
      }
      finalName = requestedName + "#" + to_string(count);
    }
    userSockets[finalName] = clientNode->acceptedSocketsFD;
    activeClients.push_back(clientNode->acceptedSocketsFD);
  }

  string handShakeMsg = "[UID_ASSIGNED] " + finalName;
  send(clientNode->acceptedSocketsFD, handShakeMsg.c_str(),
       handShakeMsg.length(), 0);

  cout << "[+] " << finalName << " has joined the server from IP: " << clientIP
       << ".\n";

  while (true)
  {
    memset(buffer, 0, sizeof(buffer));
    ssize_t data_Recived = recv(clientNode->acceptedSocketsFD, buffer, 4096, 0);

    if (data_Recived > 0)
    {
      string message = buffer;
      cout << "[" << finalName << "]: " << buffer << endl;

      //   /dm uuid system

      if (message.length() >= 4 && message.substr(0, 4) == "/dm ")
      {
        size_t spacePos = message.find(' ', 4);

        if (spacePos != string::npos)
        {
          string targetUser = message.substr(4, spacePos - 4);
          string actualMessage = message.substr(spacePos + 1);

          saveMessagesToHistory(targetUser, "DM", finalName, actualMessage);
          saveMessagesToHistory(finalName, "DM", finalName, actualMessage);

          lock_guard<mutex> lock(routerMutex);
          if (userSockets.find(targetUser) != userSockets.end())
          {
            int targetFD = userSockets[targetUser];
            string formattedMessage =
                "[DM from " + finalName + "]: " + actualMessage;
            send(targetFD, formattedMessage.c_str(), formattedMessage.length(),
                 0);
          }
          else
          {
            string errorMessage =
                "[Server]: User " + targetUser + " is offline.\n";
            send(clientNode->acceptedSocketsFD, errorMessage.c_str(),
                 errorMessage.length(), 0);
          }
        }
      }

      // /join groupName system

      else if (message.length() >= 6 && message.substr(0, 6) == "/join ")
      {
        string groupName = message.substr(6);
        lock_guard<mutex> lock(routerMutex);
        groupRooms[groupName].push_back(clientNode->acceptedSocketsFD);
        string successMsg = "[Server]: You joined group '" + groupName + "'.\n";
        send(clientNode->acceptedSocketsFD, successMsg.c_str(),
             successMsg.length(), 0);
      }

      // /group groupName Message system

      else if (message.length() >= 7 && message.substr(0, 7) == "/group ")
      {
        size_t spacePos = message.find(' ', 7);
        if (spacePos != string::npos)
        {
          string groupName = message.substr(7, spacePos - 7);
          string actualMsg = message.substr(spacePos + 1);

          saveMessagesToHistory(groupName, "Group", finalName, actualMsg);

          lock_guard<mutex> lock(routerMutex);
          if (groupRooms.find(groupName) != groupRooms.end())
          {
            string formattedMsg =
                "[" + groupName + "] " + finalName + ": " + actualMsg;
            for (int fd : groupRooms[groupName])
            {
              if (fd != clientNode->acceptedSocketsFD)
              {
                send(fd, formattedMsg.c_str(), formattedMsg.length(), 0);
              }
            }
          }
          else
          {
            string errorMsg =
                "[Server]: Group " + groupName + " does not exist.\n";
            send(clientNode->acceptedSocketsFD, errorMsg.c_str(),
                 errorMsg.length(), 0);
          }
        }
      }
      else
      {
        saveMessagesToHistory("Global", "Room", finalName, message);
        broadcastMessage(finalName, message, clientNode->acceptedSocketsFD);
      }
    }
    else
    {
      cout << RED << "[!] " << finalName << " has exited." << RESET << "\n";

      lock_guard<mutex> lock(routerMutex);

      userSockets.erase(finalName); // Remove from DMs

      // Remove from global
      for (auto i = activeClients.begin(); i != activeClients.end(); ++i)
      {
        if (*i == clientNode->acceptedSocketsFD)
        {
          activeClients.erase(i);
          break;
        }
      }

      // Remove from groups preventing the deadlock and crash -> free(): double
      // free detected in tcache 2
      vector<string> emptyGroups;

      for (auto &room : groupRooms)
      {
        auto &fds = room.second;

        // Delete user socket from the group
        for (auto i = fds.begin(); i != fds.end();)
        {
          if (*i == clientNode->acceptedSocketsFD)
          {
            i = fds.erase(i);
          }
          else
          {
            ++i;
          }
        }

        // Add for deletion if group is empty
        if (fds.empty())
        {
          emptyGroups.push_back(room.first);
        }
      }

      // Delete empty groups after loop is completely
      for (const string &groupName : emptyGroups)
      {
        cout << YELLOW << "[Server]: Group '" << groupName
             << "' is empty and has been deleted." << RESET << "\n";
        groupRooms.erase(groupName);
      }

      break;
    }
  }
  close(clientNode->acceptedSocketsFD);
  delete clientNode;
}

void recevieMessages(int clientFD)
{
  char buffer[4096];
  while (true)
  {
    memset(buffer, 0, sizeof(buffer));
    ssize_t byteRecived = recv(clientFD, buffer, 4096, 0);
    if (byteRecived > 0)
    {
      cout << "\n"
           << GREEN << buffer << RESET << "\n"
           << CYAN << "Enter message: " << RESET << flush;
    }
    else
    {
      cout << "\n"
           << RED << "[!] Server disconnected." << RESET << "\n";
      exit(0);
    }
  }
}