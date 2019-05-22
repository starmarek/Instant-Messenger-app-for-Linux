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
#include <fstream>
#include <sys/stat.h> 
#include <fcntl.h>


class user {

public:

    char *username;    
    std::vector<user> connectedUsers; //vector containing all the actual connections
    int socket;                       //socket desciptor corresponding to this*
    int actuallySwitched;             //actually switched-to user
    int authotizatingCon;             //flag deciding wheter this* must make a decision about pending connection

    user(int sockNum,  char *newUser);

    int findConected(char *userName);  //checking if user with passed username is connected to this*

    int diconnect(char *usernamen);    //disconnecting this*, from the passed user 
};


 user::user(int sockNum,  char *newUser) : username(new char[4096]), socket(sockNum), actuallySwitched(0), authotizatingCon(0) {

    memset(username, 0, 4096);
    memcpy(username, newUser, 4096);
 }


 int user::findConected(char *userName) {

    for (auto it = connectedUsers.begin(); it != connectedUsers.end(); ++it) {
        
        if(strncmp(userName, it->username, strlen(userName) - 1) == 0) 
            return it->socket;
    }
    //not found
    return -1;
}


int user::diconnect(char *usernamen) {

    //if user is not connected to any other user, return
    if(!connectedUsers.size())
        return -2;

    //iterating through whole vector of connected users
    for (auto it = connectedUsers.begin(); it != connectedUsers.end(); ++it) {

        //if found match (also exclude empty username passed)
        if(strncmp(usernamen, it->username, strlen(usernamen) - 1) == 0   && strlen(usernamen) != 0) {

            //if this was actually switched to user, remove switch flag
            if (actuallySwitched == it->socket)
                actuallySwitched = 0;

            //informing user from whom he was disconnected from
            char mess[200];
            std::string mess2 = "Server Message: Disconnected from ";
            mess2.append(it->username);
            mess2.append("\n\n");
            strcpy(mess, mess2.c_str());
            send(socket, mess, strlen(mess), 0);

            //removing from vector
            connectedUsers.erase(it);

            //succes
            return 0;       
        }
    }
    //user not found in the vector
    return -1;
}


//returns file descriptor that belongs to the user whose nickname is passed 
int findSocket(std::vector<user> &tab,  char *client) {

    //for all users connected to the server
    for (auto it = tab.begin(); it != tab.end(); ++it) {
        
        //if there is a match and passed nickname wasn't  empty
        if(strncmp(client, it->username, strlen(client) - 1) == 0  && strlen(client) != 0) {

            return it->socket;
        }
    }
    //if user was not found
    return -1;
}


//adding all socket descriptor that are currently connected to the server to the fd_set object
void addAllSockets(int &maxSock, std::vector<user> &tab, fd_set &set) {
 
    for (auto it = tab.begin(); it != tab.end(); ++it) {

        int sockNumb = it->socket;
        FD_SET(sockNumb, &set);

        //if found that socket number is bigger than currently set one -> update
        if(sockNumb > maxSock)
            maxSock = sockNumb;
    }   
}


//sets the userName after LOGIN keyword. Perform safety checks
void setUsername(char *name, user &actualUser, std::vector<user> &tab) {

    //copy the login name to buffer
    char nameBuff[100];
    memset(nameBuff, 0, sizeof(nameBuff));
    memcpy(nameBuff, &name[6], strlen(name)); 

    if(strcmp(actualUser.username, "noname") != 0) { //first checks wheter user logged in already

        send(actualUser.socket, "Server Message: You are logged already!\n\n", 42, 0);
        return;
    }
    else if(strlen(nameBuff) == 0) {  //second if the buffer was empty

        send(actualUser.socket, "Server Message: Name is unavaliable!\n\n", 40, 0);
        return;
    }

    //checks if the name is already occupied
    for (auto it = tab.begin(); it != tab.end(); ++it) {
        
        if(strncmp(it->username, nameBuff, strlen(it->username)) == 0) {

            send(actualUser.socket, "Server Message: Name is unavaliable!\n\n", 40, 0);
            return;
        }
    }
    
    //if everything is OK setting username
    memset(actualUser.username, 0, strlen(actualUser.username));
    memcpy(actualUser.username, nameBuff, strlen(nameBuff) - 1); 

    send(actualUser.socket, "Server Message: Successfull login!\n\n", 38, 0);
    return;

}


//perform preliminary opartaions on the brand new socket
void welcomeSocket(int listening, std::vector<user> &tab) {
    
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    int newSocket = accept(listening, reinterpret_cast<sockaddr *>(&addr), &addr_len);
    char welcomeMessage[] = " \n\t******* Hello friend! You have connected to the chat server *********\n\n\t\t\tPlease type 'login *USERNAME*' to login.\n\n";
    send(newSocket, welcomeMessage, sizeof(welcomeMessage), 0);

    //adding user to the vector of all connections
    tab.emplace_back(user(newSocket, (char *)"noname"));

}


//sends question to the desired user wheter he wants to connect or not
//then sets its flag of pending authotiaztion to ON
void setAuthorization(user &actualUser, std::vector<user> &tab, int sock) {

    //for all users avaliable
    for (auto it = tab.begin(); it != tab.end(); ++it) {
        
        //if there is a match
        if(it->socket == sock) {

            //sending question
            char mess[200];
            std::string mess2 = "Server Message: There is a connection from ";
            mess2.append(actualUser.username);
            mess2.append("\nDo you accept a connection?\nY/N\n\n");
            strcpy(mess, mess2.c_str());
            send(sock, mess, strlen(mess), 0);

            //setting flag
            it->authotizatingCon = actualUser.socket;
            return;
        }
    }
}


//remove every trace of USER that is passed to the function from 'connectedUsers' vector of every other user;
//it explicitly disconnect all server users from the USER if he was connected to them
void disconnectAll(char *USERname, std::vector<user> &usertab, std::vector<user> &originaltab) {

    //for all users that the USER was connected to
    for (auto it = usertab.begin(); it != usertab.end(); ++it) {
        
        char *name = it->username;

        //for all users in the original, master vector
        for (std::vector<user>::iterator iter = originaltab.begin(); iter != originaltab.end(); ++iter) {

            //if found this actually iterated user
            if(strncmp(name, iter->username, strlen(name)) == 0) {

                //disconnect USER from this user
                (*iter).diconnect(USERname);
                break;
            }
        }
    }
}


//closes the connection with socket, which requires some cleanup operations
auto closeConnection(std::vector<user> &tab, std::vector<user>::iterator &closingObject) {

    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    //remove this user from the 'connected users' vector of other users it was connected to
    disconnectAll(closingObject->username, closingObject->connectedUsers, tab);

    //reports to the std::out which user has disconnected
    getpeername(closingObject->socket, reinterpret_cast<sockaddr *>(&addr), &addr_len);
    std::cout << closingObject->username << " on " << ntohs(addr.sin_port) << " disconnected!" << std::endl;

    delete[] closingObject->username;
    close(closingObject->socket);

    //returns the iterator to the following element of erased one
    return tab.erase(closingObject);
}


//perform operation of connecting users
//firts perform safety checks, then tries to connect 
//connection in the sense of this app means adding to the personal vector
void connectUsers(char *buffer, std::vector<user> &tab, user &actualUser) {

    char userConnectName[50];

    //copying name of user to connect to from buffer
    memset(userConnectName, 0, sizeof(userConnectName));
    memcpy(userConnectName, &buffer[8], 40);

    //returns file decriptor corresponding to the requested user
    int resultBudy = findSocket(tab, userConnectName);

    //this user is not logged into server
    if(resultBudy < 0){

        char noBuddyMess[] = "Server Message: There is no such user logged!\n\n";
        send(actualUser.socket, noBuddyMess, sizeof(noBuddyMess), 0);
    }
    else if(resultBudy == actualUser.socket) { //if there is a request to connect to himself

        char noBuddyMess[] = "Server Message: You cant connect to yourself!\n\n";
        send(actualUser.socket, noBuddyMess, sizeof(noBuddyMess), 0);
    }
    else if(actualUser.findConected(userConnectName) != -1) { //checking if this user is already connected

        char noBuddyMess[] = "Server Message: You are already connected to this user!\n\n";
        send(actualUser.socket, noBuddyMess, sizeof(noBuddyMess), 0);
    }
    else {  //if everything is all right, we can send an invitation to the desired user

        setAuthorization(actualUser, tab, resultBudy);
        char noBuddyMess[] = "Server Message: Request send!\n\n";
        send(actualUser.socket, noBuddyMess, sizeof(noBuddyMess), 0);
    }
} 


//handles the response from user wheter he wants to connect or not
//function is called after detecting that 'authorizingCon' flag is set, which means that user is obligated to make a decision
void finishConnecting(char *buffer, std::vector<user> &tab, user &actualUser) {

    //if user accepted connection
    if(strncmp("y", buffer, 1) == 0 || strncmp("yes", buffer, 3) == 0 || strncmp("Y", buffer, 1) == 0) {
       
       //find the user who started the connection proccess
        for (auto it = tab.begin(); it != tab.end(); ++it) {
        
            if(it->socket == actualUser.authotizatingCon) {

                //add objects to 'connectedUsers' vector of both participants
                it->connectedUsers.emplace_back(user(actualUser.socket, actualUser.username));
                actualUser.connectedUsers.emplace_back(user(it->socket, it->username));

                //inform about success of connection
                char mess[200];
                memset(mess, 0, sizeof(mess));
                std::string mess2 = "Server Message: Succesfully connected to ";
                mess2.append(actualUser.username);
                mess2.append("\n\n");
                strcpy(mess, mess2.c_str());
                send(it->socket, mess, strlen(mess), 0);

                memset(mess, 0, sizeof(mess));
                mess2 = "Server Message: Succesfully connected to ";
                mess2.append(it->username);
                mess2.append("\n\n");
                strcpy(mess, mess2.c_str());
                send(actualUser.socket, mess, strlen(mess), 0);
                
                break;
            }
        }       
    }
    else { //otherwise inform user who started the proccess about the failure
        
        char mess[200];
        memset(mess, 0, sizeof(mess));
        std::string mess2 = "Server Message: Connection from ";
        mess2.append(actualUser.username);
        mess2.append(" refused!\n\n");
        strcpy(mess, mess2.c_str());
        send(actualUser.authotizatingCon, mess, strlen(mess), 0);
    }

    //remove flag
    actualUser.authotizatingCon = 0;
}


//perform disconnecting action along with safety checks
//removes user object from 'actuallyConnected' vector of both connected togehter users
void disconnectUsers(char *buffer, std::vector<user> &tab, user &actualUser) {

    //copying information about username to the fooBuff
    char fooBuff[50];
    memset(fooBuff, 0, sizeof(fooBuff));
    memcpy(fooBuff, &buffer[11], 40);
    
    //attempt to disconnect from desired user
    int result = actualUser.diconnect(fooBuff);

    //checks if user want to disconnect from himself
    if(strncmp(fooBuff, actualUser.username, strlen(fooBuff) - 1) == 0 && strlen(fooBuff) != 0) {

        char noBuddyMess[] = "Server Message: You cant disconnect from yourself!\n\n";
        send(actualUser.socket, noBuddyMess, sizeof(noBuddyMess), 0);
        return;
    }
    else if(result == -1) { //if didn't find desired person while executing disconect method

        char noBuddyMess[] = "Server Message: There is no such user logged!\n\n";
        send(actualUser.socket, noBuddyMess, sizeof(noBuddyMess), 0);
        return;;
    }
    else if(result == -2) { //if user is not even connected to anybody so he cant disconnect obviously

        char noBuddyMess[] = "Server Message: You are not connected to anybody!\n\n";
        send(actualUser.socket, noBuddyMess, sizeof(noBuddyMess), 0);
        return;
    }

    //if safety checks went well, it time to disconnect the desired user from the user who started the whole thing in the first place
    for (auto it = tab.begin(); it != tab.end(); ++it) {
        
        if(strncmp(fooBuff, it->username, strlen(fooBuff) - 1) == 0) {
            it->diconnect(actualUser.username);
            return;
        }
    }
}


//perform switching action along with safety checks
//changes 'actuallyswitched' flag in the object
void switchUser(char *buffer, user &actualUser) {

    //copying name of person to switch to
    char fooBuff[50];
    memset(fooBuff, 0, sizeof(fooBuff));
    memcpy(fooBuff, &buffer[7], 40);

    //finding socket of this person
    int result = actualUser.findConected(fooBuff);
    
    //if user want to switch to himself; also prevents empty name buffer
    if(strncmp(fooBuff, actualUser.username, strlen(fooBuff) - 1) == 0 && strlen(fooBuff) != 0) {

        char noBuddyMess[] = "Server Message: You cant switch to yourself!\n\n";
        send(actualUser.socket, noBuddyMess, sizeof(noBuddyMess), 0);
    }
    else if(!actualUser.connectedUsers.size()) { //if user is not conected to anybody

        char noBuddyMess[] = "Server Message: You are not connected to anyone!\n\n";
        send(actualUser.socket, noBuddyMess, sizeof(noBuddyMess), 0);
    }
    else if(result < 0) {   //if person to switch to is not actually connected to the user

        char noBuddyMess[] = "Server Message: You are not connected to such a person!\n\n";
        send(actualUser.socket, noBuddyMess, sizeof(noBuddyMess), 0);
    }
    else {  //finally perform switching

        actualUser.actuallySwitched = result; //saving the socket of person switched to in the flag
        char noBuddyMess[] = "Server Message: Succesfully switched!\n\n";
        send(actualUser.socket, noBuddyMess, sizeof(noBuddyMess), 0);
    }
}


//recieves file and send it further to the switched to user
void filesSending(user &actualUser) {

    char buffer[1000];
    //first informs user that filessending keyword was used, and than from whom the files are coming
    send(actualUser.actuallySwitched, "sendfiles", 9, 0);
    usleep(10000);
    send(actualUser.actuallySwitched, actualUser.username, strlen(actualUser.username), 0);


    //amount of files to be sent
    memset(buffer, 0, sizeof(buffer));
    recv(actualUser.socket, buffer, sizeof(buffer), 0);
    send(actualUser.actuallySwitched, buffer, strlen(buffer), 0);
    int count = atoi(buffer);

    //for all the files
    for(int i = 0; i < count; ++i) {
        
        //file name or skip information or stop keyword
        //anyway the information is send further. Skip means that file couldnt be open on the client side. Stop means the client aborted sending
        memset(buffer, 0, sizeof(buffer));
        recv(actualUser.socket, buffer, sizeof(buffer), 0);
        send(actualUser.actuallySwitched, buffer, strlen(buffer), 0);
        if(strcmp(buffer, "skip") == 0)
            continue;
        else if (strcmp(buffer, "stop") == 0) 
            break;
        
        //size of file; if its empty just continue without recieving further information
        memset(buffer, 0, sizeof(buffer));
        recv(actualUser.socket, buffer, sizeof(buffer), 0);
        send(actualUser.actuallySwitched, buffer, strlen(buffer), 0);
        if(atoi(buffer) == 0) {
            continue;
        }

        //alloc proper buffer size
        char *fileBuff = new char[atoi(buffer)-1];

        //actual file content
        recv(actualUser.socket, fileBuff, atoi(buffer), 0);
        send(actualUser.actuallySwitched, fileBuff, strlen(fileBuff), 0);
        delete[] fileBuff;
    }
}


//sends massage to desired user
void pushMessFurther(char *buffer, user &actualUser) {

    //check to whom message should be sent
    int budyNum = actualUser.actuallySwitched;

    if (actualUser.connectedUsers.size() == 0) { //if sender is not connected to anybody

        char noBuddyMess[] = "Server Message: You are not connected to anybody.\nUse 'connect' keyword, followed by an nickname of your college\n\n";
        send(actualUser.socket, noBuddyMess, sizeof(noBuddyMess), 0);
    }
    else if (budyNum == 0) { //if sender is not switched to anybody

        char noBuddyMess[] = "Server Message: You are not switched to anybody!\n\n";
        send(actualUser.socket, noBuddyMess, sizeof(noBuddyMess), 0);
    }
    else { //finally send the massage

        char mess[200];
        memset(mess, 0, sizeof(mess));
        std::string mess2 = actualUser.username;
        mess2.append(" >>>> ");
        mess2.append(buffer);
        mess2.append("\n");
        strcpy(mess, mess2.c_str());
        send(budyNum, mess, strlen(mess), 0);    
    }
}


//detects which of the socket changed its state and interacts with it
void checkSockReq(std::vector<user> &tab, fd_set &set) {

    char buffer[4096];
    
    //for all users connected to the server
    for (auto it = tab.begin(); it != tab.end(); ++it) {
        
        int socketDescriptor = it->socket;      

        //if this decscriptor is set, get into interaction with it
        if(FD_ISSET(socketDescriptor, &set)) {
           
            //reset the buffer and read the incoming information from socket
            memset(buffer, 0, sizeof(buffer));
            int bytesRecv = recv(socketDescriptor, buffer, sizeof(buffer), 0);
                          
            if(bytesRecv == 0) { //recieving 0, means that socket closed connection

                //if returned iterator points to the end, is important to prevent ++it
                it = closeConnection(tab, it);
                if(it == tab.end()) 
                    break;
            } 
            else if(strlen(buffer) == 0) //if information was empty, move on
                continue;  

            else if(strncmp("login", buffer, 5) == 0) //LOGIN keyword
                setUsername(buffer, *it, tab);

            else if(strncmp(it->username, "noname", 6) == 0) //if user is not logged yet
                send(socketDescriptor, "Server Message: Please log in first!\n\n", 40, 0);   

            else if(it->authotizatingCon) //if users flag of connection authorization is set -> perform further steps towards connecting users
                finishConnecting(buffer, tab, *it);

            else if(strncmp("connect", buffer, 7) == 0) //CONNECT keyword
                connectUsers(buffer, tab, *it);  
           
            else if(strncmp("disconnect", buffer, 10) == 0) //DISCONNECT keyword
                disconnectUsers(buffer, tab, *it);
           
            else if(strncmp("sendfiles", buffer, 9) == 0) //SENDFILES keyword
                filesSending(*it);
           
            else if(strncmp("switch", buffer, 6) == 0) //SWITCH keyword
                switchUser(buffer, *it);
           
            else  //if possible options has ended it must be a massage to the other user
                pushMessFurther(buffer, *it);
        }   
    }
}


//sets up the listening socket for the sever
int setupListening(int port) {

    sockaddr_in addr;

    //creating sokcet
    int listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(listening_socket < 0) {

        std::cerr << "Listening socket couldn't be created!" << std::endl;
        return -2;
    }

    //setting up properties for socket
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr); //0.0.0.0 means that every IP addres of machine will be listened

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

        std::cerr << "Usage: portNumber" << std::endl;
        return -1;
    }

    //if setting up will fail, return
    int listening_socket = setupListening(atoi(argv[1]));
    if(listening_socket < 0) 
        return listening_socket;

    fd_set master;                      //master set for file desctriptors (socket decsriptors, beeing honest)
    std::vector<user> clientsTab;       //vector of all users currently connected to server
    int maxSockNum = listening_socket;  //max file descriptor number for select() function

    //run basically forever
    while(true) {

        //prepering fd_set for select()
        FD_ZERO(&master);
        FD_SET(listening_socket, &master);
        addAllSockets(maxSockNum, clientsTab, master);
        
        //detects if state of any of the added file descriptors has changed
        int fromSelect = select(maxSockNum+1, &master, nullptr, nullptr, nullptr);
        if (fromSelect < 0)
            std::cerr << "Selecting went wrong!" << std::endl;

        //if state of listening socket has changed -> NEW USER COMING!
        if( FD_ISSET(listening_socket, &master) ) {

            welcomeSocket(listening_socket, clientsTab);
            
            //if only one socket was set and it was the listening one, end this iteration
            if((fromSelect - 1) == 0)
                continue;
        }
        //otherwise it must be some kind of imformation from existing sockets
        checkSockReq(clientsTab, master);
   }
} 