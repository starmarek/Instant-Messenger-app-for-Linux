#include "../bin/client_secondary_functions.hpp"


void showHelp() {

std::cout << "\n\t------------- KEYWORD HELP SECTION -----------------\n\n";
std::cout << "- queue-clear -> removes all files from file queue\n";
std::cout << "- queue-add [fileName] -> add a single file to the queue\n";
std::cout << "- queue-show -> prints all files that are currently stored in the queue\n";
std::cout << "- queue-remove [fileName] -> removes a signle file from the queue\n";
std::cout << "- connect [userName] -> connect to the desired user, if he is connected to the server\n";
std::cout << "- disconnect [userName] -> disconnect from desired user\n";
std::cout << "- switch [userName]-> switch between connected users. Actually switched user is the one who will recieve messeges and files\n";
std::cout << "- sendfiles -> send all queued files to the switched user. During process you can anytime type 'stop' to abort sending\n";
std::cout << "- whoami -> display information about yourself\n";
std::cout << "- connectedto -> display information about the actually connect-to users\n";
std::cout << "- switchedto -> display information about the actually switched-to user\n\n";
}


//erase all files from the queue; returns number of files erased
int killQueue(std::vector<char*> &tab) {

    int counter = 0;
    for(auto it = tab.begin(); it != tab.end(); ++it) {
        ++counter;
        delete[] *it;
    }
    tab.clear();
    std::cout <<  "\nSuccessfull!\n" << std::endl;
    
    return counter;
}


//show status of the queue to the user
int showQueue(std::vector<char*> &tab) {

    if(tab.size() == 0) {
        std::cout << "\nEMPTY!\n" << std::endl;
        return -1;
    }

    //enumerate the files
    int counter = 0;

    for(auto it = tab.begin(); it != tab.end(); ++it) {
        std::cout << std::endl << ++counter << ". " << *it << std::endl;
    }
    std::cout << std::endl;
    
    return 0;
}


//remove a single file from the queue
int removeFileQueue(std::vector<char *> &tab, char *input) {

    //copy the name of desired file
    char fileName[100];
    memset(fileName, 0, sizeof(fileName));
    memcpy(fileName, &input[13], 90);

    //if file was not specified
    if (strlen(fileName) <= 1){
        std::cout << "\nNo file specified, try again!\n" << std::endl; 
        return -1;
    }
    for(auto it = tab.begin(); it != tab.end(); ++it) {
        
        //if the file was found
        if(strncmp(fileName, *it, strlen(*it) - 1) == 0) {
           
            delete[] *it;
            tab.erase(it); 
            std::cout <<  "\nSuccessfull!\n" << std::endl;
            return 0;
        }
    }

    std::cout <<  "\nThere is no such file, try again!\n" << std::endl;    
    return -2;
}


//add a file to the queue
int queueFile(std::vector<char *> &tab, char *input) {

    //copy the file name and crete the file instance
    char fileName[100];
    memset(fileName, 0, sizeof(fileName));
    memcpy(fileName, &input[10], 90);

    if (strlen(fileName) <= 1) { //if file name was empty

        std::cout << "\nNo file specified, try again!\n" << std::endl; 
        return -1;
    }
    char *newFile = new char[100];
    memcpy(newFile, fileName, strlen(fileName) - 1);

    //open this file
    int file = open(newFile, O_RDONLY);

    if(file == -1) {  //if opening was unsuccessfull 
        
        std::cout << "\nThis file doesn't exist!\n" << std::endl;
        return -2;
    }

    //finally, add file to the vector
    tab.push_back(newFile);
    close(file);
    std::cout <<  "\nSuccessfull!\n" << std::endl;
    
    return 0;
}


//send all queued files
void sendFiles(int socket, std::vector<char*> &tab, char *userInput) {

    char fooBuff[4096];
    fd_set stopSendSet;      //for select() to observe the input descriptor
    timeval tv;              //timeout for select()
    struct stat stat_buf{};  //needed to get the information about the size of file

    //simple safety check
    if(tab.size() == 0) {
        std::cout << "\nNo files in the queue!\nSending aborted!\n" << std::endl;
        return;
    }

    //first inform the server that files sending operation will be performed
    send(socket, userInput, 9, 0);  
    usleep(100000); 

    //safety check. If not actually switched to anyone, stop sending.
    memset(fooBuff, 0, sizeof(fooBuff));
    recv(socket, fooBuff, sizeof(fooBuff), 0);
    if(strncmp(fooBuff, "OK", 2) != 0) {

        std::cout << std::string(fooBuff, strlen(fooBuff));
        return;
    }

    std::cout << "\n\t------- If you want to stop process, type 'stop' ------" << std::endl;

    //second inform server about the amount of files
    memset(fooBuff, 0, sizeof(fooBuff));
    sprintf(fooBuff, "%d", (int)tab.size());
    send(socket, fooBuff, strlen(fooBuff), 0);
    usleep(100000);

    //for all files in the queue
    for (auto it = tab.begin(); it != tab.end(); ++it) {
        
        std::cout << "\nSending "<< *it << " file now...\n" << std::endl;

        //if file opening was unsuccessfull skip this file
        int file = open(*it, O_RDONLY);
        if(file == -1) {

            send(socket, "skip", 4, 0);
            usleep(100000);
            std::cerr << "\nThere was problem opening file!" << std::endl;
            continue;
        }
        
        //before actual send give user a 5 sec timeout so he can stop the whole process
        FD_ZERO(&stopSendSet);
        FD_SET(0, &stopSendSet);
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        select(socket + 1, &stopSendSet, nullptr, nullptr, &tv);

        //if made an input
        if(FD_ISSET(0, &stopSendSet)) {
            memset(fooBuff, 0, sizeof(fooBuff));
            read(0, fooBuff, 4096);

            //if the input means that user want to stop the process
            if(strncmp("stop", fooBuff, 4) == 0) {
                send(socket, "stop", 4, 0);
                std::cout << "\nSending aborted!\n" << std::endl;
                return;
            }
        }

        //send the name of file so on the other side appropriee file extension is used
        send(socket, *it, strlen(*it), 0);
        usleep(100000);
        
        //send size of file in bytes so appropriete memory for file is allcoated
        fstat(file, &stat_buf);       
        int bytes = stat_buf.st_size;
        memset(fooBuff, 0, sizeof(bytes));
        sprintf(fooBuff, "%d", bytes);
        send(socket, fooBuff, strlen(fooBuff), 0);
        usleep(100000);

        //if file is empty dont send it, there is no point
        if(bytes == 0) {
            continue;
        }

        //finally send file
        sendfile(socket, file, nullptr, stat_buf.st_size+1);
    }
    std::cout << "Sending complete!\n" << std::endl;
}


//recieve files as a response to the other user who has started files sending process
void recvFiles(int socket) {

    char buffer[1000];
    
    //recv name of person who is sending files
    memset(buffer, 0, sizeof(buffer));
    recv(socket, buffer, sizeof(buffer), 0);
    std::cout << "Recieving files from " << buffer << " ...\n" << std::endl;

    //amount of files to be sent
    memset(buffer, 0, sizeof(buffer));
    recv(socket, buffer, sizeof(buffer), 0);
    int count = atoi(buffer);
    int counter = 0;           //for files enumration

    for(int i = 0; i < count; ++i) {
        
        //open file if file name was recieved, otherwise it is a skip(couldnt open file) or stop(sending aborted)
        memset(buffer, 0, sizeof(buffer));
        recv(socket, buffer, sizeof(buffer), 0);

        if(strcmp(buffer, "skip") == 0){
            continue;
        }
        else if (strcmp(buffer, "stop") == 0) {
            std::cout << "Sending was aborted!\n" << std::endl;
            return;
        }

        //crete new file and open it in read/write permissions
        int filee = open(buffer, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

        std::cout << ++counter << ". " << buffer << std::endl << std::endl;

        //size of file, for proper fileBuffer alloc; if its empty (size==0) than just close file and continue
        memset(buffer, 0, sizeof(buffer));
        recv(socket, buffer, sizeof(buffer), 0);

        if(atoi(buffer) == 0) {
            close(filee);
            continue;
        }
        char *fileBuff = new char[atoi(buffer) + 1];
        memset(fileBuff, 0, atoi(buffer) + 1);

        //actual file content. After reciev, writing it to file
        recv(socket, fileBuff, atoi(buffer) + 1, 0);
        write(filee, fileBuff, strlen(fileBuff));
        close(filee);
        delete[] fileBuff;
    }
    std::cout << "Recieving complete!\n" << std::endl;
}
