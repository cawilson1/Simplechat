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

#define SERVER_PORT 9036
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
};	

struct registrationTable{
	int port;
	int sockid;
	char mname[MAX_LINE];
	char uname[MAX_LINE];
	int entryExistsBool;//initialize this value to 1 when creating entry
	char groupId[MAX_LINE];//chat room that the person is in
};

struct packet registrationPacket, confirmationPacket, chatDataPacket, chatResponsePacket;
struct registrationTable registrationTableEntries[10];
int currentRegTableIndex = 0;
pthread_t threads[2];
int clientInTableBool = 0;
pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER;//for the buffer table
int currentSeqNum = 0;

struct registrationTable tempTable;//used in join handler for adding new clients
struct packet packetBuffer[MAX_LINE];
short bufferIndex = 0;






void createConfirmationPacket221(){
	confirmationPacket.type = htons(221);
	strncpy(confirmationPacket.uname, registrationPacket.uname, sizeof(registrationPacket.uname));
	//From this point on, the client needs to store its index in the registration table on the server
	confirmationPacket.regTableIndex = htons(currentRegTableIndex);
}

void createAcknowledgementPacket231(){
	chatResponsePacket.type = htons(231);
	strncpy(chatResponsePacket.uname, chatDataPacket.uname, sizeof(chatDataPacket.uname));
	strncpy(chatResponsePacket.data, chatDataPacket.data, sizeof(chatDataPacket.data));
	//recieve reg table index from client
	chatDataPacket.regTableIndex = ntohs(currentRegTableIndex);
}

void printRegistrationTable(){
	printf("\n\n---------------Created new registration table entry:------------------\n");
	printf("port: %u\n", registrationTableEntries[currentRegTableIndex].port);
	printf("sockid: %u\n", registrationTableEntries[currentRegTableIndex].sockid);
	printf("username: %s\n", registrationTableEntries[currentRegTableIndex].uname);
	printf("machine name: %s\n", registrationTableEntries[currentRegTableIndex].mname);
	printf("table entry index: %u", currentRegTableIndex);
	printf("\n-----------------------------------------------------------------------\n");
}

void *chatMulticaster(){//void *tempGroupString){ //void* uniqueSocket){
	
	//char chatroom[sizeof(tempGroupString)];
	//strncpy(chatroom, tempGroupString, sizeof(tempGroupString));
	//printf("\nchatroom %s\n", chatroom);
	
	//convert back to int
	//int uniqueSocketInt = (intptr_t)uniqueSocket;
	//printf("\nChat Multicaster created for client with sockId %i", uniqueSocketInt);
			
	struct packet broadcastPacket;

	while(1){
		//only starts once at least one client is in the table	
		while(clientInTableBool){
			
			//needs to be locked for entire duration of this process
			pthread_mutex_lock(&bufferMutex);
			int i, j;
			//iterate through to the current highest used index
			for(i = 0; i < bufferIndex; i++){
				strncpy(broadcastPacket.uname, packetBuffer[i].uname, sizeof(packetBuffer[i].uname));
				strncpy(broadcastPacket.groupId, packetBuffer[i].groupId, sizeof(packetBuffer[i].groupId));
				strncpy(broadcastPacket.data, packetBuffer[i].data, sizeof(packetBuffer[i].data));
				//10 is the max numb of reg table entries
				for(j = 0; j < 10; j++){
					//this entry is in the same chat room that the buffered message should be sent to
					if(registrationTableEntries[j].entryExistsBool == 1 && (strcmp(registrationTableEntries[j].groupId,broadcastPacket.groupId)==0)){
						pthread_mutex_lock(&my_mutex);						
						int new_s = registrationTableEntries[j].sockid;
						pthread_mutex_unlock(&my_mutex);
						if(send(new_s, &broadcastPacket, sizeof(broadcastPacket), 0) < 0){
							printf("error sending acknowledgement 231\n");
							exit(1);
						}
						else{
							printf("\nbroadcast packet sent to %s", registrationTableEntries[j].uname);
						}
					}
				}
			}
			bufferIndex = 0;
			pthread_mutex_unlock(&bufferMutex);
			sleep(5);
		}
/*
			int i;
			//10 is size of regTableEntries
			for(i = 0; i < 10; i++){
				//if this is 1, then this table index contains an entry. Iterates through all members with this groupId. Send a packet to this entry.
				if(registrationTableEntries[i].entryExistsBool == 1 && (strcmp(registrationTableEntries[i].groupId,chatroom)==0)){

					//strncpy(broadcastPacket.data, text, sizeof(text));
					pthread_mutex_lock(&my_mutex);
					//strncpy(broadcastPacket.uname, registrationTableEntries[i].uname, sizeof(registrationTableEntries[i].uname));
					int new_s = registrationTableEntries[i].sockid;
					pthread_mutex_unlock(&my_mutex);
		
		
					if(recv(new_s, &chatDataPacket, sizeof(chatDataPacket), 0) < 0){
						printf("\nCould not receive the chat data packet packet\n");
						exit(1);
					}
					//recieved chat data packet
					else{
						printf("\nChat data packet %u recieved successfully from server for: %s", ntohs(chatDataPacket.type),chatDataPacket.uname);
						printf("\nData recieved: %s", chatDataPacket.data);

						//send acknowledgment
						createAcknowledgementPacket231();
						if(send(new_s, &chatResponsePacket, sizeof(chatResponsePacket), 0) < 0){
							printf("error sending acknowledgement 231\n");
							exit(1);
						}
						//Acknowledgement sent to echo chat on client side
						else{
							printf("Acknowledgement packet %u sent to %s\n", ntohs(chatResponsePacket.type), chatResponsePacket.uname);
						}
					}
				}
			}
			
		}	//*/
	}


	//all of this will be used for future assignments
	//convert back to int
	//int new_s = (intptr_t)arg;
	
	//receive chat data packet
	/*while(1){

		if(recv(new_s, &chatDataPacket, sizeof(chatDataPacket), 0) < 0){
			printf("\nCould not receive the chat data packet packet\n");
			exit(1);
		}		
		//recieved chat data packet
		else{
			printf("\nChat data packet %u recieved successfully from server for:", ntohs(chatDataPacket.type));
			printf("\n[%s]: ", chatDataPacket.uname);
			printf(" %s", chatDataPacket.data);

			//send acknowledgment
			createAcknowledgementPacket231();
			if(send(new_s, &chatResponsePacket, sizeof(chatResponsePacket), 0) < 0){
				printf("error sending acknowledgement 231\n");
				exit(1);
			}
			//Acknowledgement sent to echo chat on client side
			else{
				printf("Acknowledgement packet %u sent to %s\n", ntohs(chatResponsePacket.type), chatResponsePacket.uname);
			}
		}
		
	}*/

}

void sendConfirmationPacket(int new_s){
	createConfirmationPacket221();	

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

//finds the first available index (unused slot) in the indexRegistrationEntries table
int findNextRegTableIndex(){
	
	int firstAvailableIndex = -1;
	int i = 0;
	//length of table is 10
	for(i; i < 10;i++){
		if(registrationTableEntries[i].entryExistsBool == 0){
			firstAvailableIndex = i;
			goto LeaveLoop;
		}
	}
	
	LeaveLoop:
	//unsuccessful in finding an open slot after searching through entire table
	if(firstAvailableIndex == -1){
		printf("\nThere are no available slots. The entry table is at max capacity.");
		exit(1);
	}

	return firstAvailableIndex;
}

void *joinHandler(){

	pthread_mutex_lock(&my_mutex);
	int new_s = tempTable.sockid;
	pthread_mutex_unlock(&my_mutex);
	char chatRoom[MAX_LINE];

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
			printf("The group ID from the packet %s\n", registrationPacket.groupId);
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

	pthread_mutex_lock(&my_mutex);
	//add current client received to registration table
	registrationTableEntries[currentRegTableIndex].port = tempTable.port;
	registrationTableEntries[currentRegTableIndex].sockid = new_s;
	strncpy(registrationTableEntries[currentRegTableIndex].uname, tempTable.uname, sizeof(tempTable.uname));
	strncpy(registrationTableEntries[currentRegTableIndex].mname, tempTable.mname, sizeof(tempTable.mname));
	strncpy(registrationTableEntries[currentRegTableIndex].groupId, chatRoom, sizeof(chatRoom));
	registrationTableEntries[currentRegTableIndex].entryExistsBool = 1;
	pthread_mutex_unlock(&my_mutex);
	printf("The group ID updated in the table is %s", registrationTableEntries[currentRegTableIndex].groupId);

	//table now contains a client
	clientInTableBool = 1;
	printRegistrationTable();

	sendConfirmationPacket(new_s);


	//create multicaster thread for unique client
	//pthread_create(&threads[1], NULL, chatMulticaster, (void*)registrationPacket.groupId);//(void*)(intptr_t)new_s);

	//pthread_exit(NULL);

	//constantly check for new data and update table
	while(1){
		if(recv(new_s, &chatDataPacket, sizeof(chatDataPacket), 0) < 0){
			printf("\nCould not receive the chat data packet packet\n");
			exit(1);
			}
		//recieved chat data packet
		else{
			printf("\nChat data packet %u recieved successfully from server for: %s", ntohs(chatDataPacket.type),chatDataPacket.uname);
			printf("\nData recieved: %s", chatDataPacket.data);

			//update buffer and increment index
			pthread_mutex_lock(&bufferMutex);
			//packetBuffer[bufferIndex].sockId = new_s;
			strncpy(packetBuffer[bufferIndex].uname, chatDataPacket.uname, sizeof(chatDataPacket.uname));
			strncpy(packetBuffer[bufferIndex].groupId, chatRoom, sizeof(chatRoom));
			strncpy(packetBuffer[bufferIndex].data, chatDataPacket.data, sizeof(chatDataPacket.data));
			bufferIndex++;
			pthread_mutex_unlock(&bufferMutex);
			//printf("\n the buffer was updated and uname and groupId are %s and %s", packetBuffer[bufferIndex-1].uname, packetBuffer[bufferIndex-1].groupId);
		}
	}
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
			//pthread_join(threads[0], NULL);
		}
		//packet recieved but wrong type
		else{
			printf("Wrong type of packet recieved.");
		}
	}
		//while(len = recv(new_s, buf, sizeof(buf), 0))
		//	fputs(buf, stdout);
		//close(new_s);
}
