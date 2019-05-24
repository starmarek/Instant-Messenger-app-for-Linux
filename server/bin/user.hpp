#pragma once

#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <string>

class user {

public:

    char *username;    
    std::vector<user> connectedUsers; //vector containing all the actual connections
    int socket;                       //socket desciptor corresponding to this*
    int actuallySwitched;             //actually switched-to user
    int authotizatingCon;             //flag deciding wheter this* must make a decision about pending connection

    user(int sockNum,  char *newUser);
    char *showSwitched(std::vector<user> &tab); //sends to user himself inormation about switched-to buddy
    int showConnectedUsers();          //send to user himself inmformation about connected-to users
    int findConected(char *userName);   //checking if user with passed username is connected to this*
    int disconnect(char *usernamen);     //disconnecting this*, from the passed user 
};