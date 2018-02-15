#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<sys/param.h>

#define SERVER_PORT 9025
#define MAX_LINE 256
#define MAX_PENDING 5

struct packet{
	short type;
	char uname[MAX_LINE];
	char mname[MAX_LINE];
	char data[MAX_LINE];
	short regTableIndex;
};	

struct registrationTable{
	int port;
	int sockid;
	char mname[MAX_LINE];
	char uname[MAX_LINE];
};

struct packet registrationPacket, confirmationPacket, chatDataPacket, chatResponsePacket;
struct registrationTable registrationTable[10];
int registrationTableIndex = 0;


void createConfirmationPacket221(){
	confirmationPacket.type = htons(221);
	strncpy(confirmationPacket.uname, registrationPacket.uname, sizeof(registrationPacket.uname));
	//From this point on, the client needs to store its index in the registration table on the server
	confirmationPacket.regTableIndex = htons(registrationTableIndex);
}

void createAcknowledgementPacket231(){
	chatResponsePacket.type = htons(231);
	strncpy(chatResponsePacket.uname, chatDataPacket.uname, sizeof(chatDataPacket.uname));
	strncpy(chatResponsePacket.data, chatDataPacket.data, sizeof(chatDataPacket.data));
	//recieve reg table index from client
	chatDataPacket.regTableIndex = ntohs(registrationTableIndex);
}

void printRegistrationTable(){
	printf("\n\n---------------Created new registration table entry:------------------\n");
	printf("port: %u\n", registrationTable[registrationTableIndex].port);
	printf("sockid: %u\n", registrationTable[registrationTableIndex].sockid);
	printf("username: %s\n", registrationTable[registrationTableIndex].uname);
	printf("machine name: %s\n", registrationTable[registrationTableIndex].mname);
	printf("table entry index: %u", registrationTableIndex);
	printf("\n-----------------------------------------------------------------------\n");
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

	/* wait for connection, then receive and print text */
	while(1){
		if((new_s = accept(s, (struct sockaddr *)&clientAddr, &len)) < 0){
			perror("tcpserver: accept");
			exit(1);
		}
		
		printf("\nClient's port is %d \n", ntohs(clientAddr.sin_port)); 

		//error
		if(recv(new_s, &registrationPacket, sizeof(registrationPacket), 0) < 0){
			printf("\nCould not receive the first registration packet\n");
			exit(1);
		}
		//Registration packet
		else if(htons(registrationPacket.type)==121){
			printf("\nRegistration packet:");
			printf("\nPacket type: ");
			printf("%u",htons(registrationPacket.type));
			
			printf("\nUsername: ");
			printf("%s", registrationPacket.uname);

			printf("\nMachine Name: ");
			printf("%s", registrationPacket.mname);

			//add current message received to registration table
			registrationTable[registrationTableIndex].port = ntohs(clientAddr.sin_port);
			registrationTable[registrationTableIndex].sockid = new_s;
			strncpy(registrationTable[registrationTableIndex].uname, registrationPacket.uname, sizeof(registrationPacket.uname));
			strncpy(registrationTable[registrationTableIndex].mname, registrationPacket.mname, sizeof(registrationPacket.mname));

			printRegistrationTable();

			//send registration acceptance to client
			createConfirmationPacket221();	

			//Eventually this should be stored in a struct, but for now with one host, this suffices. Index will always be zero.
			registrationTableIndex = registrationTableIndex + 1;

			printf("\n\nConfirmation packet %u being sent ", ntohs(confirmationPacket.type));
			printf("to user: %s", confirmationPacket.uname);
			if(send(new_s, &confirmationPacket, sizeof(confirmationPacket), 0) < 0){
				printf("...error sending confirmation packet");
				exit(1);
			}
			else{
				printf("...sent successfully");

				//receive chat data packet
				while(1){
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
				}

			}

			//for some reason the last step does not execute until the program has concluded, so this is last step
			printf("\nUsername: ");
			printf("%s", registrationPacket.uname);

		}

		//while(len = recv(new_s, buf, sizeof(buf), 0))
		//	fputs(buf, stdout);
		//close(new_s);


	}
}
