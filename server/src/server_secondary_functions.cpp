#include "../bin/server_secondary_functions.hpp"


//sends to given user informations about himself
void introduceUser(user &actualUser) {

    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    char mess[200];
    char IP[100];
    memset(mess, 0, sizeof(mess));
    memset(IP, 0, sizeof(IP));

    //storing informations about fd in sockaddr_in
    getpeername(actualUser.socket, reinterpret_cast<sockaddr *>(&addr), &addr_len);
    
    sockaddr_in* pV4Addr = reinterpret_cast<sockaddr_in*>(&addr);
    in_addr ipAddr = pV4Addr->sin_addr;
   
    //storing IP address in string friendly way
    inet_ntop(AF_INET, &ipAddr, IP, INET_ADDRSTRLEN);

    //setting up the string to be sent
    std::string mess2 = "Username: ";
    mess2.append(actualUser.username);
    mess2.append("\nPort: ");
    mess2.append(std::to_string(ntohs(addr.sin_port)));
    mess2.append("\nIP: ");
    mess2.append(IP);
    mess2.append("\n\n");
    strcpy(mess, mess2.c_str());

    send(actualUser.socket, mess, strlen(mess), 0);    
} 


//sends massage to switched-to user
void pushMessFurther(char *buffer, user &actualUser) {

    //check to whom message should be sent
    int budyNum = actualUser.actuallySwitched;

    if (actualUser.connectedUsers.size() == 0) { //if sender is not connected to anybody

        char mess[] = "Server Message: You are not connected to anybody.\nUse 'connect' keyword, followed by an nickname of your college\n\n";
        send(actualUser.socket, mess, sizeof(mess), 0);
    }
    else if (budyNum == 0) { //if sender is not switched to anybody

        char mess[] = "Server Message: You are not switched to anybody!\n\n";
        send(actualUser.socket, mess, sizeof(mess), 0);
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


//recieves file and send it further to the switched-to user
void filesSending(user &actualUser) {

    //check if user is switched to anyone
    if(actualUser.actuallySwitched == 0) {

        send(actualUser.socket, "\nServer Message: not switched to anyone!\n\n", 45, 0);
        return;
    }
    else{
        send(actualUser.socket, "OK", 2, 0);
    }

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
        char *fileBuff = new char[atoi(buffer) + 1];

        //actual file content
        recv(actualUser.socket, fileBuff, atoi(buffer) + 1, 0);
        send(actualUser.actuallySwitched, fileBuff, atoi(buffer) + 1, 0);
        delete[] fileBuff;
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
    if(strncmp(fooBuff, actualUser.username, strlen(actualUser.username)) == 0 && strlen(fooBuff) != 0) {

        char mess[] = "Server Message: You cant switch to yourself!\n\n";
        send(actualUser.socket, mess, sizeof(mess), 0);
    }
    else if(!actualUser.connectedUsers.size()) { //if user is not conected to anybody

        char mess[] = "Server Message: You are not connected to anyone!\n\n";
        send(actualUser.socket, mess, sizeof(mess), 0);
    }
    else if(result < 0) {   //if person to switch-to is not actually connected to the user

        char mess[] = "Server Message: You are not connected to such a person!\n\n";
        send(actualUser.socket, mess, sizeof(mess), 0);
    }
    else {  //finally perform switching

        actualUser.actuallySwitched = result; //saving the socket of person switched-to in the flag
        char mess[] = "Server Message: Succesfully switched!\n\n";
        send(actualUser.socket, mess, sizeof(mess), 0);
    }
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
    if(strncmp(fooBuff, actualUser.username, strlen(actualUser.username)) == 0 && strlen(fooBuff) != 0) {

        char mess[] = "Server Message: You cant disconnect from yourself!\n\n";
        send(actualUser.socket, mess, sizeof(mess), 0);
        return;
    }
    else if(result == -1) { //if didn't find desired person while executing disconect method

        char mess[] = "Server Message: There is no such user connected!\n\n";
        send(actualUser.socket, mess, sizeof(mess), 0);
        return;;
    }
    else if(result == -2) { //if user is not even connected to anybody so he cant disconnect obviously

        char mess[] = "Server Message: You are not connected to anybody!\n\n";
        send(actualUser.socket, mess, sizeof(mess), 0);
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
            
                //remove flag
                actualUser.authotizatingCon = 0;

                return;
            }
        }
        //if didnt find user he must have disconnected meanwhile
        char mess[100];
        memset(mess, 0, sizeof(mess));
        std::string mess2 = "Server Message: The other user is not online anymore!\n\n";
        strcpy(mess, mess2.c_str());
        send(actualUser.socket, mess, strlen(mess), 0);
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


//returns file descriptor that belongs to the user whose nickname is passed 
int findSocketByName(std::vector<user> &tab,  char *client) {

    //for all users connected to the server
    for (auto it = tab.begin(); it != tab.end(); ++it) {
        
        //if there is a match and passed nickname wasn't  empty
        if(strncmp(client, it->username, strlen(it->username)) == 0  && strlen(client) != 0) {
            return it->socket; 
        }
    }
    //if user was not found
    return -1;
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


//perform operation of connecting users
//firts perform safety checks, then tries to connect 
//connection in the sense of this app means adding to the personal vector
void connectUsers(char *buffer, std::vector<user> &tab, user &actualUser) {

    char userConnectName[50];

    //copying name of user to connect to from buffer
    memset(userConnectName, 0, sizeof(userConnectName));
    memcpy(userConnectName, &buffer[8], 40);

    //returns file decriptor corresponding to the requested user
    int resultBudy = findSocketByName(tab, userConnectName);

    //this user is not logged into server
    if(resultBudy < 0){

        char mess[] = "Server Message: There is no such user logged!\n\n";
        send(actualUser.socket, mess, sizeof(mess), 0);
    }
    else if(resultBudy == actualUser.socket) { //if there is a request to connect to himself

        char mess[] = "Server Message: You cant connect to yourself!\n\n";
        send(actualUser.socket, mess, sizeof(mess), 0);
    }
    else if(actualUser.findConected(userConnectName) != -1) { //checking if this users are already connected

        char mess[] = "Server Message: You are already connected to this user!\n\n";
        send(actualUser.socket, mess, sizeof(mess), 0);
    }
    else {  //if everything is all right, we can send an invitation to the desired user

        setAuthorization(actualUser, tab, resultBudy);
        char mess[] = "Server Message: Request send!\n\n";
        send(actualUser.socket, mess, sizeof(mess), 0);
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


//closes the connection with socket. Perform several cleanup operations
std::vector<user>::iterator closeConnection(std::vector<user> &tab, std::vector<user>::iterator &closingObject) {

    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    //if flag was set, we need to inform the awaiting user (user who wanted to establish connection) that his peer is going offline
    if(closingObject->authotizatingCon != 0) {

        char mess[100];
        memset(mess, 0, sizeof(mess));
        std::string mess2 = "Server Message: ";
        mess2.append(closingObject->username);
        mess2.append(" went offline!\n\n");
        strcpy(mess, mess2.c_str());
        send(closingObject->authotizatingCon, mess, strlen(mess), 0);
    }

    //remove this user from the 'connected users' vector of other users it was connected to
    disconnectAll(closingObject->username, closingObject->connectedUsers, tab);

    //reports to the std::out which user has disconnected
    getpeername(closingObject->socket, reinterpret_cast<sockaddr *>(&addr), &addr_len);
    std::cout << closingObject->username << " on " << ntohs(addr.sin_port) << " disconnected!" << std::endl;

    delete[] closingObject->username;
    close(closingObject->socket);

    //returns the iterator to the following element of erased one. Its important to avoid seg. fault
    return tab.erase(closingObject);
}


//returns username that belongs to the user whose socket is passed 
char *findSocketByDescriptor(std::vector<user> &tab,  int socket) {

    //for all users connected to the server
    for (auto it = tab.begin(); it != tab.end(); ++it) {
        
        //if there is a match
        if(it->socket == socket) {

            return it->username;
        }
    }
    //if user was not found
    return (char *)"Err";
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