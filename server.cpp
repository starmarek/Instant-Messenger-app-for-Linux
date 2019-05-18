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


struct user {

    char *username;
    std::vector<user> connectedUsers;
    int socket;   
    int actuallySwitched;
    int  authotizatingCon;


    user(int sockNum,  char *newUser) : username(new char[4096]), socket(sockNum) {
        memset(username, 0, 4096);
        memcpy(username, newUser, 4096);

        actuallySwitched = 0;
        authotizatingCon = 0;;

    }

    int findConected(char * username) {

        for (std::vector<user>::iterator it = connectedUsers.begin(); it != connectedUsers.end(); ++it) {
           
            if(strncmp(username, it->username, strlen(username) - 1) == 0) {
                return it->socket;
            }
        }
        return -1;
    }

    int diconnectt(char *usernamen) {

        if(!connectedUsers.size()) {
            return -2;
        }
        for (std::vector<user>::iterator it = connectedUsers.begin(); it != connectedUsers.end(); ++it) {
            if(strncmp(usernamen, it->username, strlen(usernamen) - 1) == 0   && strlen(usernamen) != 0) {
                if (actuallySwitched == it->socket) {
                    actuallySwitched = 0;
                }

                send(socket, "Server Message: Disconnected from ", 35, 0);
                send(socket, it->username,  strlen(it->username), 0);
                send(socket, "\n", 2, 0);                
                connectedUsers.erase(it);
                return 0;       
            }
        }
        return -1;
    }
};


int findSocket(std::vector<user> &tab,  char *client) {

    for (std::vector<user>::iterator it = tab.begin(); it != tab.end(); ++it) {
       
        if(strncmp(client, it->username, strlen(client) - 1) == 0  && strlen(client) != 0) {
            return it->socket;
        }
    }
    return -1;
}


void addAllSockets(int &maxSock, std::vector<user> &tab, fd_set &set) {
 
    for (std::vector<user>::iterator it = tab.begin(); it != tab.end(); ++it) {

        int sockNumb = it->socket;
        FD_SET(sockNumb, &set);
        if(sockNumb > maxSock)
            maxSock = sockNumb;
    }   
}


void welcomeSocket(int listening, std::vector<user> &tab, sockaddr_in &addr, socklen_t &len) {
    
    int newSocket = accept(listening, reinterpret_cast<sockaddr *>(&addr), &len);
    char welcomeMessage[] = "\n\t******* Hello friend! You have connected to the chat server *********\n\n";
    send(newSocket, welcomeMessage, sizeof(welcomeMessage), 0);
    send(newSocket, "Server Message: Please choose your username: \n", 46, 0);
    tab.emplace_back(user(newSocket, (char *)"noname"));

}


void connectEm(user &username, std::vector<user> &tab, int sock) {

    for (std::vector<user>::iterator it = tab.begin(); it != tab.end(); ++it) {
        
        if(it->socket == sock) {
            char noBuddyMess[] = "Server Message: There is a connection from ";
            send(sock, noBuddyMess, sizeof(noBuddyMess), 0);
            send(sock, username.username, strlen(username.username), 0);
            char BuddyMess[] = "Do you accept a connection?\nY/N\n";
            send(sock, BuddyMess, sizeof(BuddyMess), 0);
            it->authotizatingCon = username.socket;
            return;
        }
    }
}

void boundEm(user &client, std::vector<user> &tab) {

    for (std::vector<user>::iterator it = tab.begin(); it != tab.end(); ++it) {
        
        if(it->socket == client.authotizatingCon) {
            it->connectedUsers.emplace_back(user(client.socket, client.username));
            client.connectedUsers.emplace_back(user(it->socket, it->username));
            send(it->socket, "Server Message: Succesfully connected to ", 41, 0);
            send(it->socket, client.username, strlen(client.username), 0);
            send(it->socket, "\n", 2, 0);
            send(client.socket, "Server Message: Succesfully connected to ", 41, 0);
            send(client.socket, it->username, strlen(it->username), 0);
            send(client.socket, "\n", 2, 0);
            client.authotizatingCon = 0;
            return;
        }
    }
}

void discAll(char * namee, std::vector<user> &tab, std::vector<user> &tabb) {

    for (std::vector<user>::iterator it = tab.begin(); it != tab.end(); ++it) { //users con to deleting
        
        char * name = it->username;
        for (std::vector<user>::iterator iter = tabb.begin(); iter != tabb.end(); ++iter) { // all users

            if(strncmp(name, iter->username, strlen(name)) == 0) {
                (*iter).diconnectt(namee);
                break;
            }
        }
    }
}


void checkSockReq(std::vector<user> &tab, fd_set &set, sockaddr_in &addr, socklen_t &len) {

    char buffer[4096];
    char userConnectName[50];
 
    for (std::vector<user>::iterator it = tab.begin(); it != tab.end(); ++it) {
        
        int sd = it->socket;      

        if(FD_ISSET(sd, &set)) {
           
            memset(buffer, 0, sizeof(buffer));
            int bytesRecv = recv(sd, buffer, sizeof(buffer), 0);
            
            if (bytesRecv == 0) {
                
                discAll(it->username, it->connectedUsers, tab);
                getpeername(sd, reinterpret_cast<sockaddr *>(&addr), &len);
                std::cout << it->username << " on " << ntohs(addr.sin_port) << " disconnected!" << std::endl;
                delete[] it->username;
                close(sd);
                it = tab.erase(it);
                if(it == tab.end()) break;
            }
            else {

                if(strncmp("connect", buffer, 7) == 0) {
                    
                    memset(userConnectName, 0, sizeof(userConnectName));
                    memcpy(userConnectName, &buffer[8], 40);

                    int resultBudy = findSocket(tab, userConnectName);
                    if(resultBudy < 0){

                        char noBuddyMess[] = "Server Message: There is no such user logged!\n";
                        send(sd, noBuddyMess, sizeof(noBuddyMess), 0);
                    }
                    else if(resultBudy == sd) {
                        char noBuddyMess[] = "Server Message: You cant connect to yourself!\n";
                        send(sd, noBuddyMess, sizeof(noBuddyMess), 0);
                    }
                    else {

                        connectEm(*it, tab, resultBudy);
                        char noBuddyMess[] = "Server Message: Request send!\n";
                        send(sd, noBuddyMess, sizeof(noBuddyMess), 0);
                    }
                }
               
                else if(strncmp("disconnect", buffer, 10) == 0) {

                    memset(userConnectName, 0, sizeof(userConnectName));
                    memcpy(userConnectName, &buffer[11], 40);
                    int result = it->diconnectt(userConnectName);
                    if(strncmp(userConnectName, it->username, strlen(userConnectName) - 1) == 0 && strlen(userConnectName) != 0) {

                        char noBuddyMess[] = "Server Message: You cant disconnect from yourself!\n";
                        send(sd, noBuddyMess, sizeof(noBuddyMess), 0);
                    }
                    else if(result == -1) {

                        char noBuddyMess[] = "Server Message: There is no such a person!\n";
                        send(sd, noBuddyMess, sizeof(noBuddyMess), 0);
                        continue;
                    }
                    else if(result == -2) {

                        char noBuddyMess[] = "Server Message: You are not connected to anybody!\n";
                        send(sd, noBuddyMess, sizeof(noBuddyMess), 0);
                        continue;
                    }
                   
                    for (std::vector<user>::iterator iter = tab.begin(); iter != tab.end(); ++iter) {
                        if(strncmp(userConnectName, iter->username, strlen(userConnectName) - 1) == 0) {
                            iter->diconnectt(it->username);
                        }
                    }
                }
                 else if(strncmp("switch", buffer, 6) == 0) {
                    memset(userConnectName, 0, sizeof(userConnectName));
                    memcpy(userConnectName, &buffer[7], 40);
                    
                    if(strncmp(userConnectName, it->username, strlen(userConnectName) - 1) == 0 && strlen(userConnectName) != 0) {

                        char noBuddyMess[] = "Server Message: You cant switch to yourself!\n";
                        send(sd, noBuddyMess, sizeof(noBuddyMess), 0);
                        continue;
                    }
                    else if(!it->connectedUsers.size()) {
                        char noBuddyMess[] = "Server Message: You are not connected to anyone!\n";
                        send(sd, noBuddyMess, sizeof(noBuddyMess), 0);
                        continue;
                    }
                    memset(userConnectName, 0, sizeof(userConnectName));
                    memcpy(userConnectName, &buffer[7], 40);
                    int result = it->findConected(userConnectName);
                    if(result < 0) {
                        char noBuddyMess[] = "Server Message: You are not connected to such a person!\n";
                        send(sd, noBuddyMess, sizeof(noBuddyMess), 0);
                    }
                    else{
                        it->actuallySwitched = result;
                        char noBuddyMess[] = "Server Message: Succesfully switched!\n";
                        send(sd, noBuddyMess, sizeof(noBuddyMess), 0);
                    }
                }
                else {

                    if(it->authotizatingCon) {

                        if(strncmp("Y", buffer, 1) == 0) {
                            boundEm(*it, tab);
                        }
                        else {
                            send(it->authotizatingCon, "Server Message: Connection from ", 32, 0);
                            send(it->authotizatingCon, it->username, strlen(it->username), 0);
                            send(it->authotizatingCon, "refused!\n", 10, 0);
                            it->authotizatingCon = 0;
                        }
                    }
                    else if (it->connectedUsers.size() == 0) {
                        
                        if(strncmp(it->username, "noname", 6) == 0) {
                            memset(it->username, 0, 4096);
                            memcpy(it->username, buffer, strlen(buffer)-1);
                            send(sd, "Server Message: Name set!\n", 27, 0);   
                            continue;
                        }

                        char noBuddyMess[] = "Server Message: You are not connected to any person!.\nUse 'connect' keyword, followed by an nickname of your college\n";
                        send(sd, noBuddyMess, sizeof(noBuddyMess), 0);
                    }
                    else{

                        int budyNum = it->actuallySwitched;
                        if(budyNum == 0) {
                            char noBuddyMess[] = "Server Message: You are not switched to anybody!\n";
                            send(sd, noBuddyMess, sizeof(noBuddyMess), 0);
                        }
                        else {
                            send(budyNum, it->username, strlen(it->username), 0);
                            send(budyNum, " >> ", 4, 0);
                            send(budyNum, buffer, strlen(buffer), 0);
                        }
                    }
                }
            }
        }   
    }
}


int setupListening(sockaddr_in &addr, socklen_t &len, int port) {

    int listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(listening_socket < 0) {

        std::cerr << "Listening socket couldn't be created!" << std::endl;
        return -2;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr); //means ANY

    if( bind(listening_socket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        std::cerr << "Couldn't bind listening socket!" << std::endl;
        return -3;
    }

    //turning on the listetning mode for the listening socket
    if( listen(listening_socket, SOMAXCONN) < 0 ) {
        
        std::cerr << "Couldn't listen!" << std::endl;
        return -4;
    }
    return listening_socket;
}


int main (int argc, char *argv[]) {

    //checking if listening socket port was defined
    if(argc != 2) {

        std::cerr << "Something went wrong with the arguments passed to the program!\nRemember to pass the port number for listening socket!" << std::endl;
        return -1;
    }

    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
  
    int listening_socket = setupListening(addr, addr_len, atoi(argv[1]));
    if(listening_socket < 0) 
        return listening_socket;
    
    fd_set master;
    std::vector<user> clientsTab;
    int maxSockNum = listening_socket;

    while(true) {

        FD_ZERO(&master);
        FD_SET(listening_socket, &master);
        addAllSockets(maxSockNum, clientsTab, master);
        
        int fromSelect = select(maxSockNum+1, &master, nullptr, nullptr, nullptr);
        if (fromSelect < 0) {
            
            std::cerr << "Selecting went wrong!" << std::endl;
        }

        if( FD_ISSET(listening_socket, &master) ) {

            welcomeSocket(listening_socket, clientsTab, addr, addr_len);
        }
        else {

            checkSockReq(clientsTab, master, addr, addr_len);
        }
   }
    close(listening_socket);
    std::cin.get();
    return 0;
}