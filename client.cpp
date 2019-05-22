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


//erase all files from the queue
void killQueue(std::vector<char*> &tab) {

    for(auto it = tab.begin(); it != tab.end(); ++it) {
        delete[] *it;
    }
    tab.clear();
    std::cout <<  "\nSuccessfull!\n" << std::endl;
}


//show status of the queue to the user
void showQueue(std::vector<char*> &tab) {

    if(tab.size() == 0) {
        std::cout << "\nEMPTY!\n" << std::endl;
        return;
    }

    //enumerate the files
    int counter = 0;

    for(auto it = tab.begin(); it != tab.end(); ++it) {
        std::cout << std::endl << ++counter << ". " << *it << std::endl;
    }
    std::cout << std::endl;
}


//remove a single file from the queue
void removeFileQueue(std::vector<char *> &tab, char *input) {

    //copy the name of desired file
    char fileName[100];
    memset(fileName, 0, sizeof(fileName));
    memcpy(fileName, &input[13], 90);

    //if file was not specified
    if (strlen(fileName) == 0){
        std::cout << "\nNo file specified, try again!\n" << std::endl; 
        return;
    }

    for(auto it = tab.begin(); it != tab.end(); ++it) {
        
        //if the file was found
        if(strncmp(fileName, *it, strlen(fileName) - 1) == 0) {
           
            delete[] *it;
            tab.erase(it); 
            std::cout <<  "\nSuccessfull!\n" << std::endl;
            return;
        }
    }

    std::cout <<  "\nThere is no such file, try again!\n" << std::endl;    
}


//add a file to the queue
void queueFile(std::vector<char *> &tab, char *input) {

    //copy the file name and crete the file instance
    char fileName[100];
    memset(fileName, 0, sizeof(fileName));
    memcpy(fileName, &input[10], 90);
    char *newFile = new char[100];
    memcpy(newFile, fileName, strlen(fileName) - 1);

    //open this file
    int file = open(newFile, O_RDONLY);

    if (strlen(fileName) == 0) { //if file name was empty

        std::cout << "\nNo file specified, try again!\n" << std::endl; 
        return;
    }
    else if(file == -1) {  //if opening was unsuccessfull 
        
        std::cout << "\nThis file doesn't exist!\n" << std::endl;
        return;
    }

    //finally, add file to the vector
    tab.push_back(newFile);
    close(file);
    std::cout <<  "\nSuccessfull!\n" << std::endl;
}


//send all queued files
void sendFiles(int socket, std::vector<char*> &tab, char *userInput) {

    char fooBuff[4096];
    fd_set stopSendSet;      //for select() to observe the input descriptor
    timeval tv;              //timeout for select()
    struct stat stat_buf{};  //needed to get the information about the size of file

    //simple safety check
    if(tab.size() == 0) {
        std::cout << "\nNo files in the queue!\nSending aborted!\n" << std::endl;
        return;
    }

    //first inform the server that files sending operation will be performed
    send(socket, userInput, 9, 0);  
    usleep(100000);

    //second inform server about the amount of files
    memset(fooBuff, 0, sizeof(fooBuff));
    sprintf(fooBuff, "%d", (int)tab.size());
    send(socket, fooBuff, strlen(fooBuff), 0);
    usleep(100000);

    //for all files in the queue
    for (auto it = tab.begin(); it != tab.end(); ++it) {
        
        std::cout << "\nSending "<< *it << " file now...\n" << std::endl;

        //if file opening was unsuccessfull skip this file
        int file = open(*it, O_RDONLY);
        if(file == -1) {

            send(socket, "skip", 4, 0);
            usleep(100000);
            std::cerr << "\nThere was problem opening file!" << std::endl;
            continue;
        }
        
        //before actual send give user a 5 sec timeout so he can stop the whole process
        FD_ZERO(&stopSendSet);
        FD_SET(0, &stopSendSet);
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        select(socket + 1, &stopSendSet, nullptr, nullptr, &tv);

        //if made an input
        if(FD_ISSET(0, &stopSendSet)) {
            memset(fooBuff, 0, sizeof(fooBuff));
            read(0, fooBuff, 4096);

            //if the input means that user want to stop the process
            if(strncmp("stop", fooBuff, 4) == 0) {
                send(socket, "stop", 4, 0);
                std::cout << "\nSending aborted!\n" << std::endl;
                return;
            }
        }

        //send the name of file so on the other side appropriee file extension is used
        send(socket, *it, strlen(*it), 0);
        usleep(100000);
        
        //send size of file in bytes so appropriete memory for file is allcoated
        fstat(file, &stat_buf);       
        int bytes = stat_buf.st_size;
        memset(fooBuff, 0, sizeof(bytes));
        sprintf(fooBuff, "%d", bytes);
        send(socket, fooBuff, strlen(fooBuff), 0);
        usleep(100000);

        //if file is empty dont send it, there is no point
        if(bytes == 0) {
            continue;
        }

        //finally send file
        sendfile64(socket, file, nullptr, stat_buf.st_size);
    }
    std::cout << "Sending complete!\n" << std::endl;
}


//recieve files as a response to the other user who has started files sending process
void recvFiles(int socket) {

    char buffer[1000];
    
    //recv name of person who is sending files
    memset(buffer, 0, sizeof(buffer));
    recv(socket, buffer, sizeof(buffer), 0);
    std::cout << "Recieving files from " << buffer << " ...\n" << std::endl;

    //amount of files to be sent
    memset(buffer, 0, sizeof(buffer));
    recv(socket, buffer, sizeof(buffer), 0);

    int count = atoi(buffer);
    int counter = 0;           //for files enumration

    for(int i = 0; i < count; ++i) {
        
        //open file if file name was recieved, otherwise it is a skip(couldnt open file) or stop(sending aborted)
        memset(buffer, 0, sizeof(buffer));
        recv(socket, buffer, sizeof(buffer), 0);

        if(strcmp(buffer, "skip") == 0){
            continue;
        }
        else if (strcmp(buffer, "stop") == 0) {
            break;
        }

        //crete new file and open it in read/write permissions
        int filee = open(buffer, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

        std::cout << ++counter << ". " << buffer << std::endl << std::endl;

        //size of file, for proper fileBuffer alloc; if its empty (size==0) than just close file and continue
        memset(buffer, 0, sizeof(buffer));
        recv(socket, buffer, sizeof(buffer), 0);
        if(atoi(buffer) == 0) {
            close(filee);
            continue;
        }
        char *fileBuff = new char[atoi(buffer)-1];

        //actual file content. After reciev, writing it to file
        recv(socket, fileBuff, atoi(buffer), 0);
        write(filee, fileBuff, strlen(fileBuff));
        close(filee);
        delete[] fileBuff;
    }
    std::cout << "Recieving complete!\n" << std::endl;
}

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
            std::cout << buffer << std::flush;
    }

    //clean up before termination
    close(servSocket);
    killQueue(filesQueue);
    
    return 0;
}