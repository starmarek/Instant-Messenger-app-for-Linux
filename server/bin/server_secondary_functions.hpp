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

int pushMessFurther(char *buffer, user &actualUser);

void filesSending(user &actualUser);

int switchUser(char *buffer, user &actualUser);

int disconnectUsers(char *buffer, std::vector<user> &tab, user &actualUser);

int finishConnecting(char *buffer, std::vector<user> &tab, user &actualUser);

int connectUsers(char *buffer, std::vector<user> &tab, user &actualUser);

std::vector<user>::iterator closeConnection(std::vector<user> &tab, std::vector<user>::iterator &closingObject);

int setUsername(char *name, user &actualUser, std::vector<user> &tab);
