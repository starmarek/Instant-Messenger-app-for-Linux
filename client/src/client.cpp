#include "../bin/client_secondary_functions.hpp"

#include <netinet/in.h>
#include <arpa/inet.h>


//perform simple connecting operation
int connectToServer(char *portNumber, char *IpAddress) {

    sockaddr_in sock_properties;

    //socket creation
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {

        std::cerr << "Socket wasn't created!" << std::endl;
        return -2;
    }

    //setting up socket properties
    sock_properties.sin_family = AF_INET;
    sock_properties.sin_port = htons(atoi(portNumber));
    sock_properties.sin_addr.s_addr = inet_addr(IpAddress);

    //try to connect
    int connectResult = connect(sock, reinterpret_cast<sockaddr *>(&sock_properties), sizeof(sock_properties));
    if(connectResult < 0) {

        std::cerr << "Couldn't connect to server!" << std::endl;
        return -3;
    } 

    //if everything went fine, return socket
    return sock;
}


int main (int argc, char *argv[]) {
    
    //checking if appropriete number of arguments was passed
    if (argc != 3) {

        std::cerr << "Usage: IPadress  portNumber" << std::endl;
        return -1;
    }

    //check if connection was successfull
    int servSocket = connectToServer(argv[2], argv[1]);
    if(servSocket < 0) return servSocket;

    char buffer[4096];
    char userInput[4096];
    std::vector<char*> filesQueue;
    fd_set master;  //master set for select()

    //runs till EXIT keyword or forced termination
    while(true) {

        //setting up set for select()
        FD_ZERO(&master);
        FD_SET(0, &master);
        FD_SET(servSocket, &master);

        //detecting status changing on the server socket or std:in
        int fromSelect = select(servSocket + 1, &master, nullptr, nullptr, nullptr);

        if(FD_ISSET(0, &master)) {  //if std::in changed status
            
            //read the input
            memset(userInput, 0, sizeof(userInput));
            read(0, userInput, 4096);
            
            if(strncmp("queue-clear", userInput, 10) == 0)  //QUEUE-CLEAR keyword
                killQueue(filesQueue);

            else if(strncmp("keywordhelp", userInput, 11) == 0)  //QUEUE-SHOW keyword
                showHelp();

            else if(strncmp("queue-show", userInput, 10) == 0)  //QUEUE-SHOW keyword
                showQueue(filesQueue);
            
            else if(strncmp("queue-add", userInput, 9) == 0)  //QUEUE-ADD keyword
                queueFile(filesQueue, userInput);
            
            else if(strncmp("queue-remove", userInput, 12) == 0) //QUEUE-REMOVE keyword
                removeFileQueue(filesQueue, userInput);

            else if(strncmp("sendfiles", userInput, 9) == 0)  //SENDFILES keyword
                sendFiles(servSocket, filesQueue, userInput);

            else if(strncmp("exit", userInput, 4) == 0) //EXIT keyword
                break;

            else { //ran out of keywords so it must be a message to the other user

                send(servSocket, userInput, sizeof(userInput), 0);
                std::cout << std::endl;
            }

            if(fromSelect - 1 == 0) //if only std::input was set
                continue;
        }
        
        //the point where the information came from the server

        //save information in buffer
        memset(buffer, 0, sizeof(buffer));
        int bytesRecv = recv(servSocket, buffer, sizeof(buffer), 0);

        if(bytesRecv == 0) { //if recieved 0, it means server has closed

            std::cout << "Server went down!\nClosing app..." << std::endl;         
            break;  
        }
        else if(strncmp("sendfiles", buffer, 9) == 0) //if server want to send files to us, its time to go into recieving mode
            recvFiles(servSocket);

        else    //else it must be a server/user message
            std::cout << std::string(buffer, strlen(buffer)) << std::flush;
    }

    //clean up before termination
    close(servSocket);
    killQueue(filesQueue);
    
    return 0;
}