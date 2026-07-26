#include <arpa/inet.h> 
#include <cstring>      
#include <iostream>     
#include <netinet/in.h> 
#include <iostream>
#include <sys/socket.h> 
#include <unistd.h> 

int createTCPSocket() {
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0) {
        std::cerr << "[-] Socket creation failed\n";
        return -1; 
    }
    std::cout << "[+] Socket created successfully\n";
    return fd; 
  }

struct sockaddr_in createIPv4Address(const char* ip, int port) {
    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr;
    
    if(strlen(ip) == 0){
        address.sin_addr.s_addr = INADDR_ANY;
        return address;
    } else  {
    inet_pton(AF_INET, ip, &address.sin_addr.s_addr);
    return address; 
    }
  }