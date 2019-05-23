#include "../bin/server_secondary_functions.hpp"

#include <sys/time.h>
#include <sys/select.h>


//perform preliminary opartaions on the brand new socket
void welcomeSocket(int listening, std::vector<user> &tab) {
    
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    int newSocket = accept(listening, reinterpret_cast<sockaddr *>(&addr), &addr_len);
    
    char welcomeMessage[] = " \n\t******* Hello friend! You have connected to the chat server *********\n\n\t\t\tPlease type 'login [userName]' to login.\n\t   Type 'keywordhelp' to recieve information about avaliable keywords\n\n";
    send(newSocket, welcomeMessage, sizeof(welcomeMessage), 0);

    //adding user to the vector of all connections
    tab.emplace_back(user(newSocket, (char *)"noname"));

    std::cout << "noname on " << ntohs(addr.sin_port) << " connected!" << std::endl;
}


//detects which of the socket changed its state and interacts with it
void checkSockReq(std::vector<user> &tab, fd_set &set) {

    char buffer[4096];
    
    //for all users connected to the server
    for (auto it = tab.begin(); it != tab.end(); ++it) {
        
        //if this decscriptor is set, get into interaction with it
        if(FD_ISSET(it->socket, &set)) {
           
            //reset the buffer and read the incoming information from socket
            memset(buffer, 0, sizeof(buffer));
            int bytesRecv = recv(it->socket, buffer, sizeof(buffer), 0);
                          
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
                send(it->socket, "Server Message: Please log in first!\n\n", 40, 0);   

            else if(it->authotizatingCon) //if users flag of connection authorization is set -> perform further steps towards connecting users
                finishConnecting(buffer, tab, *it);

            else if(strncmp("switchedto", buffer, 10) == 0) //SWITCHEDTO keyword
                it->showSwitched(tab);

            else if(strncmp("connectedto", buffer, 11) == 0) //CONNECTEDTO keyword
                it->showConnectedUsers();

            else if(strncmp("connect", buffer, 7) == 0) //CONNECT keyword
                connectUsers(buffer, tab, *it);  

            else if(strncmp("whoami", buffer, 6) == 0) //WHOAMI keyword
                introduceUser(*it);

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

        std::cerr << "Usage: [portNumber]" << std::endl;
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