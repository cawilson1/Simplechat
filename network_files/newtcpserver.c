#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<sys/param.h>
#include<pthread.h>
#include<stdint.h>
#include <fcntl.h>

#define SERVER_PORT 8046
#define MAX_LINE 256
#define MAX_PENDING 5

struct packet{
	short type;
	char uname[MAX_LINE];
	char mname[MAX_LINE];
	char data[MAX_LINE];
	short regTableIndex;
	short seqNumber;
	char groupId[MAX_LINE];//chat room
	short rttDelay;
};	

struct registrationTable{
	int port;
	int sockid;
	char mname[MAX_LINE];
	char uname[MAX_LINE];
	int entryExistsBool;//initialize this value to 1 when creating entry
	char groupId[MAX_LINE];//chat room that the person is in
};

struct packet registrationPacket, confirmationPacket;
struct registrationTable registrationTableEntries[10];

pthread_t threads[3];
int clientInTableBool = 0;//as soon as 1 or more clients have joined, multicasting starts
pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER;//for the buffer table
int currentSeqNum = 0;

struct registrationTable tempTable;//used in join handler for adding new clients
struct packet packetBuffer[MAX_LINE];//holds chat data to broadcast
struct packet dataBuffer[1000];//for the data constantly sent
short bufferIndex = 0;//specify available position in the buffer
int clientsReadyToCom = 0;//once this is > 1 start sending packets


void createConfirmationPacket221(int index){
	confirmationPacket.type = htons(221);
	strncpy(confirmationPacket.uname, registrationPacket.uname, sizeof(registrationPacket.uname));
	//From this point on, the client needs to store its index in the registration table on the server
	confirmationPacket.regTableIndex = htons(index);
}


void printRegistrationTable(int index){
	printf("\n\n---------------Created new registration table entry:------------------\n");
	printf("port: %u\n", registrationTableEntries[index].port);
	printf("sockid: %u\n", registrationTableEntries[index].sockid);
	printf("username: %s\n", registrationTableEntries[index].uname);
	printf("machine name: %s\n", registrationTableEntries[index].mname);
	printf("table entry index: %u", index);
	printf("\n-----------------------------------------------------------------------\n");
}

//iterates through buffer and sends each message to all clients in same room
void *chatMulticaster(){
			
	struct packet broadcastPacket;//packet to send

	while(1){
		//only starts once at least one client is in the table	n
		while(clientInTableBool){
			
			//needs to be locked for entire duration of this process
			pthread_mutex_lock(&bufferMutex);
			int i, j;
			//iterate through to the current highest used index
			for(i = 0; i < bufferIndex; i++){
				broadcastPacket.type = htons(301);
				strncpy(broadcastPacket.uname, packetBuffer[i].uname, sizeof(packetBuffer[i].uname));
				strncpy(broadcastPacket.groupId, packetBuffer[i].groupId, sizeof(packetBuffer[i].groupId));
				strncpy(broadcastPacket.data, packetBuffer[i].data, sizeof(packetBuffer[i].data));
				//10 is the max numb of reg table entries
				for(j = 0; j < 10; j++){
					//this entry is in the same chat room that the buffered message should be sent to
					if(registrationTableEntries[j].entryExistsBool == 1 && (strcmp(registrationTableEntries[j].groupId,broadcastPacket.groupId)==0)){
						pthread_mutex_lock(&my_mutex);
						//get socket for this entry						
						int new_s = registrationTableEntries[j].sockid;
						pthread_mutex_unlock(&my_mutex);
						if(send(new_s, &broadcastPacket, sizeof(broadcastPacket), 0) < 0){
							printf("error sending acknowledgement 231\n");
							exit(1);
						}
						else{
							printf("\nbroadcast packet %i sent to %s", ntohs(broadcastPacket.type), registrationTableEntries[j].uname);
						}
					}
				}
			}
			bufferIndex = 0;//reset buffer index to 0. All packets have been sent. Start process over
			pthread_mutex_unlock(&bufferMutex);
		}
	}
}

void sendConfirmationPacket(int new_s, int index){
	createConfirmationPacket221(index);	

	printf("\n\nConfirmation packet %u being sent ", ntohs(confirmationPacket.type));
	printf("to user: %s", confirmationPacket.uname);
		
	//send confirmation packet
	if(send(new_s, &confirmationPacket, sizeof(confirmationPacket), 0) < 0){
		printf("...error sending confirmation packet");
		exit(1);
	}
	else{
		printf("...sent successfully");
	}
}

//frees up space in reg table for client that is leaving chat
void findAndRemoveRegTableEntry(int socket){
	int i = 0;
	for(i; i < 10; i++){
		if(registrationTableEntries[i].entryExistsBool == 1 && registrationTableEntries[i].sockid == socket){
			printf("\nRemoving %s from registration table", registrationTableEntries[i].uname);
			registrationTableEntries[i].entryExistsBool = 0;//set entry exists to false
			i = 10;//exit loop
		}
	}
}


//finds the first available index (unused slot) in the indexRegistrationEntries table
int findNextRegTableIndex(){
	
	int firstAvailableIndex = -1;//initialize
	int i = 0;
	//length of table is 10
	for(i; i < 10;i++){
		//entry exists is false
		if(registrationTableEntries[i].entryExistsBool == 0){
			firstAvailableIndex = i;
			goto LeaveLoop;
		}
	}
	
	LeaveLoop:
	
	if(firstAvailableIndex == -1){
		//unsuccessful in finding an open slot after searching through entire table
		printf("\nThere are no available slots. The entry table is at max capacity.");
		exit(1);
	}

	return firstAvailableIndex;
}

void *initPacket(void* arg){
	int new_s = (intptr_t)(void*)arg;//convert back to int
	struct packet initPacket;
	int initMessageSent = 0;

	initPacket.type = htons(401);

	while(1){
		if((clientsReadyToCom > 1) && !initMessageSent){
			if(send(new_s, &initPacket, sizeof(initPacket), 0) < 0){
				printf("\nwas not able to send init packet");
				exit(1);
			}
			else{
				printf("\ninit message %i sent", ntohs(initPacket.type));
				initMessageSent = 1;
			}
		}
	}


}

void *joinHandler(){
	int currentRegTableIndex = 0;//so this thread always has this clients index in reg table
	pthread_mutex_lock(&my_mutex);
	int new_s = tempTable.sockid;//get sockid from temporary global table
	pthread_mutex_unlock(&my_mutex);
	char chatRoom[MAX_LINE];//name of chatroom
	struct packet chatDataPacket, syncPacket;//packet receiving data
	char localUsername[MAX_LINE];

	char exitString[MAX_LINE];//if user enters this string, user exits and the thread terminates
	strncpy(exitString, "EXIT", sizeof("EXIT"));//if user enters this string, exit
	exitString[4] = 10;//default packet data string uses a different char to signal end of string. Change this string to that so they match

	//needs to get two more confirmation packets to send ACK
	int i;
	for(i = 0; i < 2; i++){
		//attempt to recieve registration packet
		if(recv(new_s, &registrationPacket, sizeof(registrationPacket), 0) < 0){
			printf("\nCould not receive registration packet %i\n", i);
			exit(1);
		}
		//Registration packet
		else if(htons(registrationPacket.type)==121){
			printf("\nRecieved registration packet %i\n", i + 2);
			printf("The group ID (chatroom) for this user is %s\n", registrationPacket.groupId);
			strncpy(chatRoom,registrationPacket.groupId,sizeof(registrationPacket.groupId));
			
		}
		else{
			//packet recieved but wrong type
			printf("Wrong type of packet recieved.");
			exit(1);
		}
	}//all three registration packets have been recieved
	
	//find the first available index in the registration table
	currentRegTableIndex = findNextRegTableIndex();

	//add current client received to registration table
	pthread_mutex_lock(&my_mutex);
	registrationTableEntries[currentRegTableIndex].port = tempTable.port;
	registrationTableEntries[currentRegTableIndex].sockid = new_s;
	strncpy(registrationTableEntries[currentRegTableIndex].uname, tempTable.uname, sizeof(tempTable.uname));
	strncpy(registrationTableEntries[currentRegTableIndex].mname, tempTable.mname, sizeof(tempTable.mname));
	strncpy(localUsername, tempTable.uname, sizeof(tempTable.uname));
	strncpy(registrationTableEntries[currentRegTableIndex].groupId, chatRoom, sizeof(chatRoom));
	registrationTableEntries[currentRegTableIndex].entryExistsBool = 1;
	pthread_mutex_unlock(&my_mutex);
	printf("The group ID updated in the table is %s", registrationTableEntries[currentRegTableIndex].groupId);

	clientInTableBool = 1;//table now contains at least 1 client. Multicast can start
	printRegistrationTable(currentRegTableIndex);//print info for this new client stored in reg table

	sendConfirmationPacket(new_s, currentRegTableIndex);//send confirmation to client


	//constantly check for new packet data and update buffer if found
	//this neeeds to be in its own new thread
	pthread_create(&threads[2], NULL, initPacket, (void*)(intptr_t)new_s);

	while(1){
		if(recv(new_s, &chatDataPacket, sizeof(chatDataPacket), 0) < 0){
			printf("\nCould not receive the chat data packet packet\n");
			exit(1);
		}
		//recieved chat data packet
		else if (htons(chatDataPacket.type)==131){
			printf("\nChat data packet %u recieved successfully from server for: %s", ntohs(chatDataPacket.type),chatDataPacket.uname);
			printf("\nData recieved: %s", chatDataPacket.data);
		
			//exits if client types EXIT
			if(strcmp(chatDataPacket.data, exitString)==0){
				printf("\n%s terminating thread", chatDataPacket.uname);
				goto EXIT;
			}

			//update buffer and increment index
			pthread_mutex_lock(&bufferMutex);
			strncpy(packetBuffer[bufferIndex].uname, chatDataPacket.uname, sizeof(chatDataPacket.uname));
			strncpy(packetBuffer[bufferIndex].groupId, chatRoom, sizeof(chatRoom));
			strncpy(packetBuffer[bufferIndex].data, chatDataPacket.data, sizeof(chatDataPacket.data));
			bufferIndex++;
			pthread_mutex_unlock(&bufferMutex);
			printf("\nThe buffer was updated");		
		}
		//recieved constant packet data flow from client (a/v simulation)
		else if (htons(chatDataPacket.type)==901){
			printf("\nreceived an a/v packet from %s", chatDataPacket.uname);
		}
		//recieved a sync packet. send back to sender immediately
		else if (htons(chatDataPacket.type)==801){
			syncPacket.type=htons(811);
			strncpy(syncPacket.uname, chatDataPacket.uname, sizeof(chatDataPacket.uname));	
			syncPacket.seqNumber = chatDataPacket.seqNumber;
			

		//	if(syncPacket.seqNumber == ntohs(401){
		//		if(send(new_s, &syncPacket, sizeof(syncPacket), 0) < 0){
		//			printf("...error sending start av packets");
		//			exit(1);
		//		}
		//		else{
		//			printf("send the message to start sending av packets here");
		//		}
		//	}
			
			if(send(new_s, &syncPacket, sizeof(syncPacket), 0) < 0){
				printf("...error sending sync packet");
				exit(1);
			}
			else{
				printf("\n...sent sync packet type %i seqNum %i ack successfully to %s", htons(syncPacket.type), htons(syncPacket.seqNumber), syncPacket.uname);
				printf("");
			}	
		}
		else if(htons(chatDataPacket.type)==501){
			clientsReadyToCom = clientsReadyToCom + 1;
			printf("\nclient %s is ready to start sending av packets", chatDataPacket.uname);
			printf("\number of active clients: %i", clientsReadyToCom);

		}
		else{
			("\nThe data packet type %i does not match a specified data type\n", htons(chatDataPacket.type));
		}
	}
	EXIT:
	//frees up this registration table entry for a new chat
	findAndRemoveRegTableEntry(new_s);
	clientsReadyToCom = clientsReadyToCom - 1;//1 less client to communicate
	printf("\none less communicating client. Number of active clients:%i\n", clientsReadyToCom);
	pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
	struct sockaddr_in sin;
	struct sockaddr_in clientAddr;
	char buf[MAX_LINE];
	int s, new_s;
	int len;

	/* setup passive open */
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("tcpserver: socket");
		exit(1);
	}


	/* build address data structure */
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(SERVER_PORT);


	if(bind(s,(struct sockaddr *)&sin, sizeof(sin)) < 0){
		perror("tcpclient: bind");
		exit(1);
	}
	listen(s, MAX_PENDING);

	//create multicaster thread
	pthread_create(&threads[1], NULL, chatMulticaster, NULL);
	printf("Waiting for connection...\n");


	/* wait for connection, then receive and print text */
	while(1){

		if((new_s = accept(s, (struct sockaddr *)&clientAddr, &len)) < 0){
			perror("tcpserver: accept");
			exit(1);
		}
		
		printf("\nClient's port is %d \n", ntohs(clientAddr.sin_port)); 
		
		//attempt to recieve registration packets
		if(recv(new_s, &registrationPacket, sizeof(registrationPacket), 0) < 0){
			printf("\nCould not receive the first registration packet\n");
			exit(1);
		}
		//First registration packet received
		else if(htons(registrationPacket.type)==121){
			printf("\nRecieved registration packet 1");


			//tempTable is global in this implementation, lock it so it can only be used one at a time
			pthread_mutex_lock(&my_mutex);
			tempTable.port = ntohs(clientAddr.sin_port);
			tempTable.sockid = new_s;
			strncpy(tempTable.uname, registrationPacket.uname, sizeof(registrationPacket.uname));
			strncpy(tempTable.mname, registrationPacket.mname, sizeof(registrationPacket.mname));
			pthread_mutex_unlock(&my_mutex);
			

			//call join handler
			pthread_create(&threads[0], NULL, joinHandler, NULL);
		}
		//packet recieved but wrong type
		else{
			printf("Wrong type of packet recieved.");
		}
	}
}
