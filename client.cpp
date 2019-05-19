#include <iostream>     //std::cout | std::cerr | std::endl | std::cin | std::string()
#include <stdlib.h>     //int atoi()
#include <sys/socket.h> //int socket() | int connect() | ssize_t recv() | ssize_t send() 
#include <sys/types.h>  //int socket() | int connect() | ssize_t recv() | ssize_t send()
#include <unistd.h>     //int close() 
#include <netinet/in.h> //struct sockaddr_in 
#include <arpa/inet.h>  //uni16_t htons() | int inet_pton()
#include <string.h>     //void *memset()
#include <stdio.h>      //ssize_t getline()
#include <sys/select.h> //int select()
#include <sys/sendfile.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <vector>


void killQueue(std::vector<char*> &tab) {

    for(auto it = tab.begin(); it != tab.end(); ++it) {
        delete[] *it;
    }
    tab.clear();
    std::cout <<  "\nSuccessfull!\n" << std::endl;
}


void showQueue(std::vector<char*> &tab) {

    if(tab.size() == 0) {
        std::cout << "\nEMPTY!\n" << std::endl;
        return;
    }
    int counter = 0;
    for(auto it = tab.begin(); it != tab.end(); ++it) {
        std::cout << std::endl << ++counter << ". " << *it << std::endl;
    }
    std::cout << std::endl;
}


void removeFileQueue(std::vector<char *> &tab, char *input) {

    char fileName[100];
    memset(fileName, 0, sizeof(fileName));
    memcpy(fileName, &input[13], 90);

    if (strlen(fileName) == 0){
        std::cout << "\nNo file specified, try again!\n" << std::endl; 
        return;
    }

    for(auto it = tab.begin(); it != tab.end(); ++it) {
    
        if(strncmp(fileName, *it, strlen(fileName) - 1) == 0) {
           
            delete[] *it;
            tab.erase(it); 
            std::cout <<  "\nSuccessfull!\n" << std::endl;
            return;
        }
    }

    std::cout <<  "\nThere is no such file, try again!\n" << std::endl;    
}


void queueFile(std::vector<char *> &tab, char *input) {

    char fileName[100];
    memset(fileName, 0, sizeof(fileName));
    memcpy(fileName, &input[10], 90);

    if (strlen(fileName) == 0){
        std::cout << "\nNo file specified, try again!\n" << std::endl; 
        return;
    }

    char *newFile = new char[100];
    memcpy(newFile, fileName, strlen(fileName) - 1);
    tab.push_back(newFile);
    std::cout <<  "\nSuccessfull!\n" << std::endl;
}


void sendFiles(int socket, std::vector<char*> &tab, char *userInput) {

    fd_set stopSendSet;
    char fooBuff[4096];
    timeval tv;
    struct stat stat_buf{};

    if(tab.size() == 0) {
        std::cout << "\nNo files in the queue!\nSending aborted!\n" << std::endl;
        return;
    }

    send(socket, userInput, 9, 0);  
    usleep(100000);
    memset(fooBuff, 0, sizeof(fooBuff));
    sprintf(fooBuff, "%d", (int)tab.size());
    send(socket, fooBuff, strlen(fooBuff), 0);
    usleep(100000);

    for (auto it = tab.begin(); it != tab.end(); ++it) {
        
        std::cout << "\nSending "<< *it << " file now...\n" << std::endl;
        int file = open(*it, O_RDONLY);
        if(file == -1) {

            send(socket, "skip", 4, 0);
            usleep(100000);
            std::cerr << "\nThere was problem opening file!" << std::endl;
            continue;
        }

        FD_ZERO(&stopSendSet);
        FD_SET(0, &stopSendSet);
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        select(socket + 1, &stopSendSet, nullptr, nullptr, &tv);

        if(FD_ISSET(0, &stopSendSet)) {
            memset(fooBuff, 0, sizeof(fooBuff));
            read(0, fooBuff, 4096);
            if(strncmp("stop", fooBuff, 4) == 0) {
                send(socket, "stop", 4, 0);
                std::cout << "\nSending aborted!\n" << std::endl;
                return;
            }
        }

        send(socket, *it, strlen(*it), 0);
        usleep(100000);

        fstat(file, &stat_buf);       
        int bytes = stat_buf.st_size;
        memset(fooBuff, 0, sizeof(bytes));
        sprintf(fooBuff, "%d", bytes);
        send(socket, fooBuff, strlen(fooBuff), 0);
        usleep(100000);

        if(bytes == 0) {
            continue;
        }

        sendfile64(socket, file, nullptr, stat_buf.st_size);
    }
    std::cout << "Sending complete!\n" << std::endl;
}


int connectToServer(char *portNumber, char *IpAddress) {

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {

        std::cerr << "Socket wasn't created!" << std::endl;
        return -2;
    }

    sockaddr_in sock_properties;
    sock_properties.sin_family = AF_INET;
    sock_properties.sin_port = htons(atoi(portNumber));
    sock_properties.sin_addr.s_addr = inet_addr(IpAddress);

    int connectResult = connect(sock, reinterpret_cast<sockaddr *>(&sock_properties), sizeof(sock_properties));
    if(connectResult < 0) {

        std::cerr << "Couldn't connect to server!" << std::endl;
        return -3;
    }

    return sock;
}


int main (int argc, char *argv[]) {
    
    if (argc != 3) {

        std::cerr << "Something went wrong with the arguments passed to the program!\n";
        std::cerr << "Remember to pass the IP addres and port number for a socket!" << std::endl;
        return -1;
    }
    
    int servSocket = connectToServer(argv[2], argv[1]);
    if(servSocket < 0) return servSocket;

    char buffer[4096];
    char userInput[4096];
    std::vector<char*> filesQueue;
    fd_set master;  

    while(true) {

        FD_ZERO(&master);
        FD_SET(0, &master);
        FD_SET(servSocket, &master);

        select(servSocket + 1, &master, nullptr, nullptr, nullptr);

        if(FD_ISSET(0, &master)) {

            memset(userInput, 0, sizeof(userInput));
            read(0, userInput, 4096);
            
            if(strncmp("queue-clear", userInput, 10) == 0) {

                killQueue(filesQueue);
            }
            else if(strncmp("queue-show", userInput, 10) == 0) {
                
                showQueue(filesQueue);
            }
            else if(strncmp("queue-add", userInput, 9) == 0) {

                queueFile(filesQueue, userInput);
            }
            else if(strncmp("queue-remove", userInput, 12) == 0) {

                removeFileQueue(filesQueue, userInput);
            }
            else if(strncmp("sendfiles", userInput, 9) == 0) {

                sendFiles(servSocket, filesQueue, userInput);
            }
            else if(strncmp("exit", userInput, 4) == 0) {

                break;
            }
            else {

                send(servSocket, userInput, sizeof(userInput), 0);
                std::cout << std::endl;
            }
        }
        else {

            memset(buffer, 0, sizeof(buffer));
            int bytesRecv = recv(servSocket, buffer, sizeof(buffer), 0);
            if (bytesRecv < 0 ) {

                std::cerr << "There was an error recieving a response from the server!" << std::endl;
            }
            else if(bytesRecv == 0) {

                std::cout << "Server went down!\nClosing app..." << std::endl;         
                break;  
            }
            else {

                std::cout << std::string(buffer, bytesRecv);
            }
        }
    }

    close(servSocket);
    killQueue(filesQueue);
    
    return 0;
}