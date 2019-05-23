#pragma once

#include "user.hpp"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <sys/stat.h> 
#include <fcntl.h>


void introduceUser(user &actualUser);

void pushMessFurther(char *buffer, user &actualUser);

void filesSending(user &actualUser);

void switchUser(char *buffer, user &actualUser);

void disconnectUsers(char *buffer, std::vector<user> &tab, user &actualUser);

void finishConnecting(char *buffer, std::vector<user> &tab, user &actualUser);

void connectUsers(char *buffer, std::vector<user> &tab, user &actualUser);

std::vector<user>::iterator closeConnection(std::vector<user> &tab, std::vector<user>::iterator &closingObject);

void setUsername(char *name, user &actualUser, std::vector<user> &tab);
