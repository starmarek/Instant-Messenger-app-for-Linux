#pragma once

#include <vector>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <iostream> 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/sendfile.h>
#include <stdlib.h>


void killQueue(std::vector<char*> &tab);

void showQueue(std::vector<char*> &tab);

void removeFileQueue(std::vector<char *> &tab, char *input);

void queueFile(std::vector<char *> &tab, char *input);

void sendFiles(int socket, std::vector<char*> &tab, char *userInput);

void recvFiles(int socket);

void showHelp();
