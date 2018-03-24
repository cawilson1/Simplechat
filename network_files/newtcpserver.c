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

#define SERVER_PORT 9035
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
int currentSeqNum = 0;
//used in join handler for adding new clients
struct registrationTable tempTable;


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

void *chatMulticaster(void* uniqueSocket){

	//convert back to int
	int uniqueSocketInt = (intptr_t)uniqueSocket;
	printf("\nChat Multicaster created for client with sockId %i", uniqueSocketInt);
			
	struct packet broadcastPacket;
	char *filename;
	char text[20];
	int fd;
	filename = "input.txt";
	fd = open(filename,O_RDONLY, 0);

	while(1){
		//only starts once at least one client is in the table	
		while(clientInTableBool){

			int i;
			//10 is size of regTableEntries
			for(i = 0; i < 10; i++){
				//if this is 1, then this table index contains an entry. Send a packet to this entry.
				if(registrationTableEntries[i].entryExistsBool == 1){

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
						printf("\nChat data packet %u recieved successfully from server for:", ntohs(chatDataPacket.type));
				short groupId;//chat room		printf("\n[%s]: ", chatDataPacket.uname);
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
					/*if(send(new_s, &broadcastPacket, sizeof(broadcastPacket), 0) < 0){
						printf("\nCouldn't send packet");
						exit(1);
					}
					else{
						printf("\npacket sent to %s with sequence number %i", broadcastPacket.uname, ntohs(broadcastPacket.seqNumber));
					}*/
				}
			}
			//sleep(1);
			//currentSeqNum++;
			
			
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
	strncpy(registrationTableEntries[currentRegTableIndex].groupId, registrationPacket.groupId, sizeof(registrationPacket.groupId));
	registrationTableEntries[currentRegTableIndex].entryExistsBool = 1;
	pthread_mutex_unlock(&my_mutex);
	printf("The group ID updated in the tabkle is %s", registrationTableEntries[currentRegTableIndex].groupId);

	//table now contains a client
	clientInTableBool = 1;
	printRegistrationTable();

	sendConfirmationPacket(new_s);

	//create multicaster thread for unique client
	pthread_create(&threads[1], NULL, chatMulticaster, (void*)(intptr_t)new_s);

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
	//pthread_create(&threads[1], NULL, chatMulticaster, NULL);
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
			pthread_join(threads[0], NULL);
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
