/* --------------------------------------------------------------------------------------------------------------
   Peer to Peer Chat Application
   Animesh Jain  18CS10004
   Abhinav Bohra 18CS30049
   p2p-chat-app.cpp
-----------------------------------------------------------------------------------------------------------------*/
#include <sys/stat.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/wait.h>
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <netdb.h>    
#include <time.h>
#include <fcntl.h>
#include <arpa/inet.h> 
#include <errno.h> 
#include <netinet/in.h> 
#include <signal.h> 

#define MAXPEERS 5
#define MAXSIZE 100000
#define SA struct sockaddr 
#define STDIN 0  // file descriptor for standard console input

struct user_info{

	int port;			/* client's port number */
	char *IPaddr;			/* client's IP address */
	char *clientname;		/* client's name */	
};

struct user_info PeerUserInfo[MAXPEERS];

//This function creates a table that looks like:-
/*
	------------------------------------
	|PORT |   IP ADDRESS   | CLIENT NAME|
	------------------------------------
	|30001| 192.168.18.144 | User1 		|
	|30002| 192.168.18.144 | User2    	|
	|30003| 192.168.18.144 | User3      |
	|30004| 192.168.18.144 | User4      |
	|30005| 192.168.18.144 | User5  	|
	-------------------------------------
*/
void setup_user_info(){
	
	char *clientnames[MAXPEERS+1]={"User1","User2","User3","User4","User5"};
	int base_port = 30001;
	char *IPaddr = "192.168.18.144";

  	for(int i=0;i<MAXPEERS;i++){

		PeerUserInfo[i].port = base_port + i;
		PeerUserInfo[i].IPaddr = IPaddr;
		PeerUserInfo[i].clientname = clientnames[i];
	}

}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Function Prototyping of user defined functions
int getIndex(char * clientName);
void ParseMessage(char *buffer,char *clientName, char *message);
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int main(int argc, char **argv) 
{ 	
	if (argc < 2){
		printf("\033[3;33m\nInvalid Arguments. Please refere to Section 3.1 of Design Document.\033[0m\n");
		exit(EXIT_FAILURE);
	}
	
	setup_user_info();
	int connect_fd[MAXPEERS];
	int connect_read_fd[MAXPEERS];
	for(int i = 0; i < MAXPEERS ; i++){
		connect_fd[i]= -1;
		connect_read_fd[i]= -1;
	}
	
	printf("\x1b[36m\nSHARED USER INFO TABLE\x1b[0m\n"); 
	
	 for(int i=0;i<MAXPEERS;i++){

		printf("\n| %d | %s | %s \n",PeerUserInfo[i].port,
		PeerUserInfo[i].IPaddr,
		PeerUserInfo[i].clientname);
	}
	printf("\n");
	
	int serverIndex = getIndex(argv[1]);		  
	if (serverIndex == -1){
		printf("\033[3;33m\nPeer's Name not present in UserInfo Table.\033[0m\n");
		exit(EXIT_FAILURE);
	}  
	
	int serverSockfd, len, result,option=1;
	struct sockaddr_in servaddr, cli;
  
	// Step 1 : Socket Creation and Verification 
	serverSockfd = socket(AF_INET, SOCK_STREAM, 0); 
	setsockopt(serverSockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
	setsockopt(serverSockfd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option));
	
	if (serverSockfd < 0) { 
		printf("\x1b[31mSERVER SOCKET CREATION FAILED.....EXITING\x1b[0m\n"); 
		exit(EXIT_FAILURE); 
	} 
	else printf("\x1b[32mSERVER SOCKET CREATED SUCCESSFULLY\x1b[0m\n"); 
	bzero(&servaddr, sizeof(servaddr)); 

	// Filing Address Informaiton 
    servaddr.sin_family = AF_INET;           							   		//Domain - IPV4
   	servaddr.sin_addr.s_addr = inet_addr(PeerUserInfo[serverIndex].IPaddr);   	//Server IP Address
    servaddr.sin_port = htons(PeerUserInfo[serverIndex].port);            		//Server PORT

	// Step 2: Binding newly created socket to given IP and verification 
	if ((bind(serverSockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
		printf("\x1b[31mSERVER SOCKET BINDING FAILED.....EXITING\x1b[0m\n"); 
		exit(EXIT_FAILURE); 
	} 
    	else printf("\x1b[32mSERVER SOCKET SUCCESSFULLY BINDED\x1b[0m\n"); 
	
	// Now server is ready to listen 
	if ((listen(serverSockfd, MAXPEERS)) != 0) { 
		printf("\x1b[31mAHHH.....CANT LISTEN YOU :(\x1b[0m\n"); 
		exit(EXIT_FAILURE); 
	} 
	else  printf("\x1b[44mSERVER IS UP!........LISTENING @%s:%d\x1b[0m\n\n",inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port)); 
	len = sizeof(cli);  
	

	printf("\n\033[0;36m++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\033[0m\n");
	printf("\033[0;32m   Hi %s! Welcome to our Peer-to-Peer Chat Applicaion. \033[0m\n", argv[1]);
	printf("\033[3;35m  Enter the name of the peer, followed by a / , and then your message.\033[0m\n");
	printf("\033[0;36m++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\033[0m\n\n");
  
	// Create the readset & writeset file descriptor set
	
	fd_set tempset, readset;
	FD_ZERO(&readset);

	struct timeval timeout;
	timeout.tv_sec = 50000;
	int maxFD =0,no_of_cons=0;	


	while(1){
		// Add our first socket that we're interested in interacting with; the listening socket!
		FD_SET(serverSockfd, &readset);
		FD_SET(STDIN, &readset);	
		maxFD = serverSockfd;
		for(int i = 0; i < MAXPEERS ; i++)
		{
			FD_SET( connect_fd[i], &readset);
			FD_SET( connect_read_fd[i], &readset);			 
			maxFD = ( maxFD > connect_fd[i]) ? maxFD : connect_fd[i] ;
			maxFD = ( maxFD > connect_read_fd[i])? maxFD : connect_read_fd[i] ;

		}	
		maxFD++;	
		
		int socketCount = select(maxFD,&readset,NULL,NULL,&timeout);
		
	      	if (socketCount < 0){
			printf("\x1b[31mSELECT ERROR\x1b[0m\n"); 
		  	break;
		}
	        else if (socketCount == 0) {
	            	printf("\x1b[31mSELECT TIME OUT\x1b[0m\n"); 
		  	continue;
	        }

		//CASE 1 -> If it is Server Socket i.e. there is pending connection request
		if(FD_ISSET(serverSockfd, &readset)){
			
			//Accept connection request from client
			struct sockaddr_in cliaddr; 
			int clientSockfd = accept(serverSockfd, (SA*)&cliaddr, &len); 
	    		if (clientSockfd < 0) { 
	    	   		printf("\x1b[31mSERVER ACCEPT FAILED\x1b[0m\n"); 	
		    		exit(EXIT_FAILURE); 
		   	} 
			connect_read_fd[no_of_cons++] = clientSockfd;    
		
		}	

		//CASE 2-> If not a server socket but STDIN		
		if(FD_ISSET(STDIN, &readset)){
				
				//1. Get data from console & parse it
				char buffer[MAXSIZE];                    //Input from console
				char clientName[MAXSIZE];				 //Sender Name
				char message[MAXSIZE];			         //Message to be sent
				bzero(&buffer, sizeof(buffer)); 
				bzero(&clientName, sizeof(clientName)); 					
				bzero(&message, sizeof(message)); 
					
				int buffBytes = read(STDIN,buffer,sizeof(buffer));
				ParseMessage(buffer, clientName, message);
				//Check parsed messgge & clientnme, if NULL then exit	

				int clientIndex = getIndex(clientName);
				//2. If not, create a connection, add it to fd_set & send the message
				if(clientIndex == -1 ) {
  					printf("\x1b[31mPEER NOT FOUND IN USERINFO TABLE\x1b[0m\n"); 	
		 			continue;
				}
 
				struct sockaddr_in clientaddr;	
				bzero(&clientaddr, sizeof(clientaddr)); 
				clientaddr.sin_family = AF_INET; 
				clientaddr.sin_port = htons(PeerUserInfo[clientIndex].port);			                        
				clientaddr.sin_addr.s_addr = inet_addr(PeerUserInfo[clientIndex].IPaddr);
						 
				if(connect_fd[clientIndex]<0){

					connect_fd[clientIndex] = socket(AF_INET, SOCK_STREAM, 0);
					setsockopt(connect_fd[clientIndex], SOL_SOCKET, SO_REUSEPORT , &option, sizeof(option));
					
			    	if (connect_fd[clientIndex] < 0) printf("ERROR WHILE CRETING THE SOCKET");
					//Connect client to socket
					if(connect(connect_fd[clientIndex], (SA*)&clientaddr, sizeof(clientaddr))<0) continue;

				// Print the IP of the established connection									
				printf("\x1b[34mESTABLISHED CONNECTION WITH %s:%d\x1b[0m\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port)); 
					 
				}			 
			    
			    char MESSAGE1[MAXSIZE+1];
			    char MESSAGE2[2*MAXSIZE+2];
				sprintf(MESSAGE1,"%s%s",PeerUserInfo[serverIndex].clientname,"/");
				sprintf(MESSAGE2,"%s%s",MESSAGE1,message);
				
				printf("\x1b[36m[WRITING MESSAGE TO %s] %s\x1b[0m\n",PeerUserInfo[clientIndex].clientname,message); 
				int bytesWritten = write(connect_fd[clientIndex],MESSAGE2,strlen(MESSAGE2)+1);
		   		if (bytesWritten < 0) printf("ERROR WHILE WRITING TO SOCKET");
		}		
		//CASE 3 -> else it is a client socket
		for(int i=0;i<MAXPEERS;i++){
			if(FD_ISSET(connect_read_fd[i], &readset)){
		
				char incomingMSSG[MAXSIZE];              //Input from console
				char senderName[MAXSIZE];				 //Sender Name
				char message[MAXSIZE];			         //Message to be sent
				bzero(&incomingMSSG, sizeof(incomingMSSG)); 
				bzero(&senderName, sizeof(senderName)); 					
				bzero(&message, sizeof(message)); 
					
				int buffBytes = read( connect_read_fd[i], incomingMSSG, sizeof(incomingMSSG));	
				ParseMessage(incomingMSSG, senderName, message);
				
				if(buffBytes < 0 ) printf("ERROR WHILE READING FILE FROM SOCKET\n");
				//Close Connection if no incoming (empty) messages from client
				if(buffBytes==0){
					FD_CLR(connect_read_fd[i], &readset); 
					printf("\x1b[34m[%s DISCONNECTED]\x1b[0m\n",senderName); 
					close(connect_read_fd[i]);
					
				}
				printf("\x1b[33m[INCOMING MESSAGE FROM %s] %s\x1b[0m\n",senderName,message); 
			}		
		}	
	}

	FD_CLR(serverSockfd, &readset); // Remove the listening socket from the master file descriptor set and
	close(serverSockfd);	  	//Close it to prevent anyone else trying to connect.	
	
	return 0;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Helper Functions

int getIndex(char *clientName){

	for(int i=0; i< MAXPEERS;i++) 
		if(strcmp(PeerUserInfo[i].clientname, clientName) == 0 ) return i;

	return -1;
}

void ParseMessage(char *buffer,char *clientName, char *message){

    int index =0;
    while(buffer[index]!='/') {
	clientName[index] = buffer[index];
	index++;
    }

    clientName[index] = '\0';
    index++; //Skip '/'

    int j;
    for(j=index; j<MAXSIZE; j++) message[j-index]=buffer[j];
	message[j] = '\0';			
    //printf("FROM MESSAGE PARSER: Client - %s, Message - %s\n",clientName,message);
}	
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------		