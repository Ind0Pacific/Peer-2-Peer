/*
 * Peer-2-Peer (A TCP Chat Application) - Version 1.0 Release
 * Author: Deepanshu Vashisht (GitHub -> https://github.com/Ind0Pacific)
 * Description: Multi-threaded C++ TCP chat system featuring room routing,
 *              DMs, history logging.
 */

#include "functions.cpp"
#include <filesystem>
#include <iostream>
#include <istream>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <vector>

using namespace std;
namespace fs = std::filesystem;
int main()
{

  int choice = 0;
  int index = 1;
  int targetPort;
  int clientFD;

  string clientName;
  string targetIP;
  string message_send;
  string choiceStr;
  string line;
  string selectedFile;
  string sender;
  string message;
  string path = "bin/chat_history";

  vector<string> chatFiles;

  size_t senderStart;
  size_t msgStart;
  size_t senderEnd;
  size_t msgEnd;

  cout << "Enter the IP and port (e.g., 127.0.0.1 2000): ";
  cin >> targetIP >> targetPort;

  clientFD = createTCPSocket();
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
      cout << RED << "[!] Name taken. Server assigned you the UID: " << assignedName << RESET << "\n";
      clientName = assignedName;
    }
    else
    {
      cout << YELLOW << "[+] Connected successfully as: " << clientName << RESET << "\n";
    }
  }

  cout << YELLOW << "\n=======================================\n";
  cout << "           YOUR RECENT CHATS           \n";
  cout << "=======================================\n"
       << RESET;

  // chat file storage folder scan for .json files
  if (fs::exists(path) && fs::is_directory(path))
  {
    for (const auto &entry : fs::directory_iterator(path))
    {
      if (entry.path().extension() == ".json")
      {
        string filename = entry.path().filename().string();
        chatFiles.push_back(filename);
        string displayName = filename.substr(0, filename.length() - 5);
        cout << "[" << index << "] " << displayName << "\n";
        index++;
      }
    }
  }

  // ask usr what chat they want to view
  if (chatFiles.empty())
  {
    cout << "No chat history found. Starting fresh!\n";
  }
  else
  {
    cout << "[" << index << "] Skip (Just start chatting)\n";
    cout << "Select a chat to view history (1-" << index << "): ";
    getline(cin >> ws, choiceStr);

    try
    {
      choice = stoi(choiceStr);
    }
    catch (...)
    {
    }

    // Load the chats of choosen file

    if (choice >= 1 && choice <= chatFiles.size())
    {
      selectedFile = "bin/chat_history/" + chatFiles[choice - 1];
      ifstream file(selectedFile);

      cout << "\n--- Loading " << chatFiles[choice - 1] << " ---\n";
      while (getline(file, line))
      {
        senderStart = line.find("\"sender\": \"");
        msgStart = line.find("\"message\": \"");

        if (senderStart != string::npos && msgStart != string::npos)
        {
          senderStart += 11;
          senderEnd = line.find("\"", senderStart);
          sender = line.substr(senderStart, senderEnd - senderStart);

          msgStart += 12;
          msgEnd = line.find("\"}", msgStart);
          message = line.substr(msgStart, msgEnd - msgStart);
          cout << GREEN << sender << ": " << message << RESET << "\n";
          cout << sender << ": " << message << "\n";
        }
      }
      cout << "--- End of History ---\n\n";
    }
    else
    {
      cout << "\n--- Skipping History ---\n\n";
    }
  }

  cout << "=======================================\n";
  cout << "[+] Server is live! Type 'exit' to quit.\n";
  cout << "[?] Commands: /dm User#1 msg | /join Group | /group Group msg\n\n";

  // background listner

  thread listener(recevieMessages, clientFD);
  listener.detach();

  // type messages

  while (true)
  {
    cout << CYAN << "Enter message: " << RESET;
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