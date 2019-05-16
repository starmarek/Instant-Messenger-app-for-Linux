#include <iostream>     //std::cout | std::cerr | std::endl | std::string()
#include <stdlib.h>     //int atoi()
#include <sys/types.h>  //int socket() | int accept() | int listen() | ssize_t recv() | ssize_t send()
#include <sys/socket.h> //int socket() | int accept() | int listen() | ssize_t recv() | ssize_t send() | int getnameinfo() 
#include <netdb.h>      //int getnameinfo()
#include <netinet/in.h> //struct sockaddr_in
#include <string.h>     //void *memset()
#include <arpa/inet.h>  //uni16_t htons() | int inet_pton()
#include <unistd.h>     //close()
#include <sys/select.h> //int select()
#include <vector>       //std::vector
#include <tuple>        //std::tuple


int findSocketByIP(int *tab, int max, char *IP) {

    char host[4096];
    sockaddr_in addr;
    socklen_t len = sizeof(addr);

    for (int i = 0; i < max; ++i) {

        if(tab[i] > 0) {
            
            memset(host, 0, sizeof(host));
            getpeername(tab[i], reinterpret_cast<sockaddr *>(&addr), &len);
            const char *IP2 = inet_ntop(AF_INET, &addr.sin_addr, host, len);
            size_t boo = sizeof(IP2);
            std::cout << IP2 << std::endl;

            if(strncmp(IP, IP2, boo) == 0 ) {
                
                return tab[i];
            }
        }
    }
    return -1;
}


void addAllSockets(int max, int &maxSock, int *tab, fd_set &set) {
 
    for (int i = 0; i < max; ++i) {
        
        if(tab[i] > 0) { 

            FD_SET(tab[i], &set);
        }
        if(tab[i] > maxSock) {
            maxSock = tab[i];
        }
    }   
}


void welcomeSocket(int max, int &listening, int *tab, sockaddr_in &addr, socklen_t &len) {
    
    int newSocket = accept(listening, reinterpret_cast<sockaddr *>(&addr), &len);
    char welcomeMessage[] = "\n\t******* Hello friend! You have connected to the chat server *********\n";

    send(newSocket, welcomeMessage, sizeof(welcomeMessage), 0);

    for(int i = 0; i < max; ++i ) {

        if(tab[i] == 0) {
            tab[i] = newSocket;
            break;
        }
    }
}

void deleteChatRoom(std::vector<std::tuple<int ,int>> &chatRooms, int sock) {

    for(unsigned int i = 0; i < chatRooms.size(); ++i) {

        auto current = chatRooms[i];
        char leftMess[] = "Server Message: Your friend disconnected from the server!\n";

        auto first = std::get<0>(current);
        auto second = std::get<1>(current);

        if(first == sock) {

            send(second, leftMess, sizeof(leftMess), 0);           
            chatRooms.erase(chatRooms.begin()+i);
            break;
        }
        else if(second == sock) {
            
            send(first, leftMess, sizeof(leftMess), 0);           
            chatRooms.erase(chatRooms.begin()+i);
            break;
        }
    }
}

int returnChatBuddy(std::vector<std::tuple<int ,int>> &chatRooms, int sock) {

    for(unsigned int i = 0; i < chatRooms.size(); ++i) {

        auto current = chatRooms[i];

        auto first = std::get<0>(current);
        auto second = std::get<1>(current);

        if(first == sock){

            return second;
        }
        else if(second == sock) { 

            return first;
        }
    }
    return -1;
}

void checkSockReq(int *tab, int max, fd_set &set, sockaddr_in &addr, socklen_t &len) {

    char buffer[4096];
    char keyWord[8] = "connect";
    char ipAddr[50];
    static std::vector<std::tuple<int ,int >> chatRooms;
 
    for(int i = 0; i < max; ++i) {
        
        int sd = tab[i];
        memset(buffer, 0, sizeof(buffer));
        memset(ipAddr, 0, sizeof(ipAddr));
        if(FD_ISSET(sd, &set)) {
            
            int bytesRecv = recv(sd, buffer, sizeof(buffer), 0);
            if(bytesRecv < 0){

                std::cerr << "There was problem reciving!" << std::endl;
            }
            else if (bytesRecv == 0) {

                char service[4096];
                getpeername(sd, reinterpret_cast<sockaddr *>(&addr), &len);
                inet_ntop(AF_INET, &addr.sin_addr, service, len);
                std::cout << service << " on " << ntohs(addr.sin_port) << " disconnected!" << std::endl;
                tab[i] = 0; 
                close(sd);
                deleteChatRoom(chatRooms, sd);
            }
            else{

                if(strncmp(keyWord, buffer, 7) == 0) {

                    memcpy(ipAddr, &buffer[8], 40);
                    
                    int budy = findSocketByIP(tab, max, ipAddr);
                    if(budy < 0){

                        char noBuddyMess[] = "Server Message: There is no such user logged!.\n";
                        send(sd, noBuddyMess, sizeof(noBuddyMess), 0);
                    }
                    else {

                        auto pair = std::make_tuple(sd, budy);
                        chatRooms.push_back(pair);
                        char BuddyMess[] = "Server Message: Succesfully connected!.\n";
                        send(sd, BuddyMess, sizeof(BuddyMess), 0);
                        send(budy, BuddyMess, sizeof(BuddyMess), 0);
                    }
                }
                else {
                    
                    int buddyNum = returnChatBuddy(chatRooms, sd);
                    if(buddyNum < 0){

                        char noBuddyMess[] = "Server Message: You are not connected to any person!.\nUse 'connect' keyword, followed by an IP addres of your college\n";
                        send(sd, noBuddyMess, sizeof(noBuddyMess), 0);
                    }
                    else{
                        send(buddyNum, ">>", 2, 0);
                        send(buddyNum, buffer, sizeof(buffer), 0);
                    }
                }
            }
        }
    }
}


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

    //setting up listening socket properties and binding the socket 
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr); //"0.0.0.0" means ANY

    if( bind(listening_socket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        std::cerr << "Couldn't bind listening socket!" << std::endl;
        return -3;
    }

    //turning on the listetning mode for the listening socket
    if( listen(listening_socket, SOMAXCONN) < 0 ) {
        
        std::cerr << "Couldn't listen!" << std::endl;
        return -4;
    }
   

    fd_set master;
    int maxClients = 30;
    int *clientsTab = new int[maxClients];
    int maxSockNum = listening_socket;

    while(true) {

        FD_ZERO(&master);
        FD_SET(listening_socket, &master);
        addAllSockets(maxClients, maxSockNum, clientsTab, master);
        
        int fromSelect = select(maxSockNum+1, &master, nullptr, nullptr, nullptr);

        if (fromSelect < 0) {
            
            std::cerr << "Selecting went wrong!" << std::endl;
        }

        if( FD_ISSET(listening_socket, &master) ) {

            welcomeSocket(maxClients, listening_socket, clientsTab, addr, addr_len);
        }
        else {
            
            checkSockReq(clientsTab, maxClients, master, addr, addr_len);
        }
   }
    close(listening_socket);
    std::cin.get();
    return 0;
}