# Instant-Messenger-app-for-Linux
Console interface application that allows you to to chat with several buddies at the same time and send files to them as well.
App uses TCP protocol and sockets functionality so as a client you only need to know the IP addres and port of server application.

## How to get started
Just  `cmake .` and then `make` in client or server directory to get the rdy-to-launch executables.\
In order to run server on desired machine just type: <br/>`./[server's_executable_name] [avaliable_port_number]` <br/> Now you are ready to connect with your client exe's.\
Let's assume that server is running on your friends machine in the local network, on port 12345 and IP address 192.168.xx.xx .
Your job is to run the client app from the console: <br/> `./[client's_executable_name] [192.168.xx.xx] [12345]` <br/>and that's it!\
When you are trying to connect to server outside of your local network it's ***very*** important to make sure that port forwarding rules in the 
router settings are established and firewalls are down. Nevertheless, privacy settings and protection from unwanted connections is sometimes unbeatable and you won't be able to connect to server outside of your network.
## How does it work
As you are now connected to the server there are few things you need to know. App is using **KEYWORD** system, or just command'ish mechanic to 
get things done. As a user you are able to type-in some of the commands:
- *keywordhelp* -> prints quick reminder of avaliable-to-use keywords with brief explanation of what they do
- *exit* -> closes the app
- *login [userName]* -> the first command you will need to type, in order start interacting with other users. The only thing you will be able to achieve without logging-in is filesqueue manipulation and *keywordhelp* command use. 
Besides you can of course use *exit* to close the app. **Remember** that as you will login, you won't be able to change the username without re-connecting to the server
- *queue-add [fileName]* -> lets you add a file to the queue of files-to-be-sent. The file must exist and be present in the programs directory!
- *queue-remove [fileName]* -> allows you to remove a single file from the filesqueue
- *queue-clear* -> remove all the files from the queue
- *queue-show* -> prints the actual status of filesqueue to the console
- *sendfiles* -> begin the process of queued files sending. You are able to abort the sending at any moment by typing **stop** to the console
- *connect [userName]* -> performs the connecting action. The other user you want to connect-to must be of course connected 
to the server. You can be connected to several users at a time. Beeing connected to the user allows you to recieve messeges and files from the other user so beware who you connect to!
- *switch [userName]* -> you can only switch to the connected-to users! The switched-to user is the one who will recieve messages and files from you. Nevertheless, you will still recieve messages from all **connected-to** users.
- *disconnect [userName]* -> you can only disconnect from connected-to users. After this operation you won't be able to exchange messages and files
- *whoami* -> prints to the console basic informations about yourself
- *switchedto* -> prints to the console the user that you are switched-to at the moment
- *connectedto* -> enumerate to the console the users that you are connected-to at the moment
## Additional functionality
Program is euqipped in the gtest Unity Tests. You can find them both in the *server* and *client* directory int the *tests* dir. \
The tests are provided for nearly all of the functions of the program.
## Credits
- **Aleksander Pucher** -> all of the code, and program structure as well.
