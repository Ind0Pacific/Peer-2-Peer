/*
 * Peer-2-Peer (A TCP Chat Application) - Version 1.5.7 Release.
 * Author: Deepanshu Vashisht (GitHub -> https://github.com/Ind0Pacific).
 * Description: Multi-threaded C++ TCP chat system featuring room routing,
 *              DMs, history logging using sockets.
 */

#include "functions.cpp"
#include <arpa/inet.h>
#include <cstddef>
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
  char buffer[65536];
  memset(buffer, 0, sizeof(buffer));
  recv(clientFD, buffer, 65536, 0);

  string serverResponse = buffer;
  if (serverResponse.find("[UID_ASSIGNED] ") == 0)
  {
    string assignedName = serverResponse.substr(15);

    if (assignedName != clientName)
    {
      cout << RED
           << "[!] Name taken. Server assigned you the UID: " << assignedName
           << RESET << "\n";
      clientName = assignedName;
    }
    else
    {
      cout << YELLOW << "[+] Connected successfully as: " << clientName << RESET
           << "\n";
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
  cout << "[?] Commands: /dm User#1 msg | /join Group | /group Group msg | /send usename filename (should be one current directory adn less than 20KB) | "
          "/scan - Check who is online using chat history\n\n";

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
    else if (message_send.length() >= 6 && message_send.substr(0, 6) == "/send ")
    {
      size_t spacePos = message_send.find(' ', 6);
      if (spacePos != string::npos)
      {
        string targetUser = message_send.substr(6, spacePos - 6);
        string filepath = message_send.substr(spacePos + 1);

        // Open file in binary
        ifstream file(filepath, ios::binary);
        if (file)
        {
          ostringstream oss;
          oss << file.rdbuf();
          string fileData = oss.str();
          file.close();

          // TCP Packet Fragmentation Safety Check
          // buffer is of 60KB but we doing it of 20 as we need to add command target usr uuid etc this will increase the size so we need to prevent the buffer overflow
          if (fileData.length() > 20000)
          {
            cout << RED << "[-] File too large! Max size is 20KB for this server." << RESET << "\n";
          }
          else
          {
            cout << YELLOW << "[*] Encoding and sending file..." << RESET << "\n";
            string hexData = encodeHex(fileData);

            // Extract just the filename from directory path
            string filename = fs::path(filepath).filename().string();

            // Package it for the server router
            string networkPacket = "/file " + targetUser + " " + filename + " " + hexData;
            send(clientFD, networkPacket.c_str(), networkPacket.length(), 0);

            cout << GREEN << "[+] File '" << filename << "' sent successfully!" << RESET << "\n";
          }
        }
        else
        {
          cout << RED << "[-] File not found: " << filepath << RESET << "\n";
        }
      }
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