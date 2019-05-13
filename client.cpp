#include <iostream>     //std::cout | std::cerr | std::endl | std::cin | std::string()
#include <stdlib.h>     //int atoi()
#include <sys/socket.h> //int socket() | int connect() | ssize_t recv() | ssize_t send() 
#include <sys/types.h>  //int socket() | int connect() | ssize_t recv() | ssize_t send()
#include <unistd.h>     //int close() 
#include <netinet/in.h> //struct sockaddr_in 
#include <arpa/inet.h>  //uni16_t htons() | int inet_pton()
#include <string.h>     //void *memset()
#include <stdio.h>      //ssize_t getline()


int main (int argc, char *argv[]) {

    //checking if appropriete number of arguments was passed to the program
    if (argc != 3) {

        std::cerr << "Something went wrong with the arguments passed to the program!\n";
        std::cerr << "Remember to pass the IP addres and port number for a socket!" << std::endl;
        return -1;
    }

    //creating socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {

        std::cerr << "Socket wasn't created!" << std::endl;
        return -2;
    }

    sockaddr_in sock_properties;
    sock_properties.sin_family = AF_INET;
    sock_properties.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &sock_properties.sin_port);

    //connecting to the server
    int connectResult = connect(sock, reinterpret_cast<sockaddr *>(&sock_properties), sizeof(sock_properties));
    if(connectResult < 0) {
        std::cerr << "Couldn't connect to server!" << std::endl;
        return -3;
    }

    //communication with server
    char buffer[4096];
    std::string userInput;

    while(true) {

        std::cout << "> ";
        getline(std::cin, userInput);

        int sendResult = send(sock, userInput.c_str(), userInput.size() + 1, 0);
        if(sendResult < 0) {
            std::cerr << "Couldn't send! Try again..." << std::endl;
            continue;
        }

        memset(buffer, 0, sizeof(buffer));
        int bytesRecv = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesRecv < 0 ) {
            std::cerr << "There was an error recieving a response from the server!" << std::endl;
        }
        else {
            std::cout << "Server> " << std::string(buffer, bytesRecv) << std::endl;
        }
    }

    //closing socket
    close(sock);
    
    return 0;
}