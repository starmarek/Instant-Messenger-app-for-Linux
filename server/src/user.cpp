#include "../bin/user.hpp"


char *findSocketByDescriptor(std::vector<user> &tab,  int socket);


//first finds the appropriete user using socket, than sends the information to *this
char *user::showSwitched(std::vector<user> &tab) {

    //if not switched at all
    if(actuallySwitched == 0) {
        send(socket, "Server Message: Not switched to anyone!\n\n", 43, 0);
        return (char *)"Err";
    }

    //finding username of switched-to descriptor. In this position its quite impossible to recieve "Err" message from function because it would 
    //have been detected in the previous 'if' statement
    char *name = findSocketByDescriptor(tab, actuallySwitched);
    if(strcmp(name, "Err") == 0)
        return name;

    char mess[200];
    std::string mess2;

    //prepering message
    mess2.append("User: ");
    mess2.append(name);
    mess2.append("\n\n");

    //sending
    strcpy(mess, mess2.c_str());
    send(socket, mess, strlen(mess), 0);

    return name;
}


//prepares the message about connected-to users, and send it to *this
int user::showConnectedUsers() {

    //if not connected to anyone
    if(connectedUsers.size() == 0) {
        send(socket, "Server Message: Not connected to anyone!\n\n", 44, 0);
        return -1;
    }

    char mess[200];
    std::string mess2;
    int counter = 0;

    //collecting informations and preparing message
    for (auto it = connectedUsers.begin(); it !=connectedUsers.end(); ++it) {

        mess2.append(std::to_string(++counter));
        mess2.append(". ");
        mess2.append(it->username);
        mess2.append("\n");
    } 
    //sending
    mess2.append("\n");
    strcpy(mess, mess2.c_str());
    send(socket, mess, strlen(mess), 0);

    return counter;
}


//simple c-tor. Just initialization
 user::user(int sockNum,  char *newUser) : username(new char[4096]), socket(sockNum), actuallySwitched(0), authotizatingCon(0) {

    memset(username, 0, 4096);
    memcpy(username, newUser, 4096);
 }


//returns the socket of passed user, if its present in the vector of connectedUsers
 int user::findConected(char *userName) {

    for (auto it = connectedUsers.begin(); it != connectedUsers.end(); ++it) {
        
        if(strncmp(userName, it->username, strlen(it->username)) == 0) 
            return it->socket;
    }
    //not found
    return -1;
}


//disconnect this* from passed user
int user::disconnect(char *usernamen) {

    //if user is not connected to any other user, return
    if(!connectedUsers.size())
        return -2;

    //iterating through whole vector of connected users
    for (auto it = connectedUsers.begin(); it != connectedUsers.end(); ++it) {

        //if found match (also exclude empty username passed)
        if(strncmp(usernamen, it->username, strlen(it->username)) == 0   && strlen(usernamen) != 0) {

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