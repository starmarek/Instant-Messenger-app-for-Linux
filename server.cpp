#include <iostream>     //std::cout | std::cerr | std::endl | std::string()
#include <stdlib.h>     //int atoi()
#include <sys/types.h>  //int socket() | int accept() | int listen() | ssize_t recv() | ssize_t send()
#include <sys/socket.h> //int socket() | int accept() | int listen() | ssize_t recv() | ssize_t send() | int getnameinfo() 
#include <netdb.h>      //int getnameinfo()
#include <netinet/in.h> //struct sockaddr_in
#include <string.h>     //void *memset()
#include <arpa/inet.h>  //uni16_t htons() | int inet_pton()
#include <unistd.h>     //close()


int main (int argc, char *argv[]) {

    //checking if listening socket port was defined
    if(argc != 2) {

        std::cerr << "Something went wrong with the arguments passed to the program!\nRemember to pass the port number for listening socket!" << std::endl;
        return -1;
    }

    //creating listening socket
    int listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(listening_socket < 0) {

        std::cerr << "Listening socket couldn't be created!" << std::endl;
        return -2;
    }

    //setting up listening socket properties and binding them together
    sockaddr_in lstnr_addr;

    lstnr_addr.sin_family = AF_INET;
    lstnr_addr.sin_port = htons(atoi(argv[1]));
    inet_pton(AF_INET, "0.0.0.0", &lstnr_addr.sin_addr); //"0.0.0.0" means ANY

    if( bind(listening_socket, reinterpret_cast<sockaddr *>(&lstnr_addr), sizeof(lstnr_addr)) < 0) {
        std::cerr << "Couldn't bind listening socket!" << std::endl;
        return -3;
    }

    //turning on the listetning mode for the listening socket
    if( listen(listening_socket, SOMAXCONN) < 0 ) {
        
        std::cerr << "Couldn't listen!" << std::endl;
        return -4;
    }
    std::cout << "Listening for client..." << std::endl << std::endl;

    //accepting incoming connection and saving client's socket
    sockaddr_in client_addr;
    socklen_t client_sock_len = sizeof(client_addr);

    int client_socket = accept(listening_socket, reinterpret_cast<sockaddr *>(&client_addr), &client_sock_len);
    if(client_socket < 0) {

        std::cerr << "Client socket couldn't be created!" << std::endl;
        return -5;
    }

    //closing listening socket
    close(listening_socket);

    //printing to the console information about a client
    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];
    memset(host, 0, sizeof(host));
    memset(serv, 0, sizeof(serv));

    getnameinfo(reinterpret_cast<sockaddr *>(&client_addr), client_sock_len, host, sizeof(host), serv, sizeof(serv), 0);
    std::cout << host << " connected on " << serv << std::endl << std::endl;
 

    //communication with a client
    char buffer[4096];
    while (true) {

        memset(buffer, 0, sizeof(buffer));

        int bytesRecv = recv(client_socket, buffer, sizeof(buffer), 0);
        if(bytesRecv < 0) {

            std::cerr << "Connection issue!" << std::endl;
            return -6;
        }
        else if(bytesRecv == 0) {

            std::cout << "Connection closed!" << std::endl;
            break;
        }

        std::cout << "Recieved: " << std::string(buffer, bytesRecv) << std::endl;
        send(client_socket, buffer, sizeof(buffer), 0);
    }

    //closing client socket
    close(client_socket);

    return 0;
}