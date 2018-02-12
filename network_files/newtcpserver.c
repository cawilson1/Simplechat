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
};	

struct registrationTable{
	int port;
	int sockid;
	char mname[MAX_LINE];
	char uname[MAX_LINE];
};

struct packet registrationPacket, confirmationPacket, chatDataPacket, chatResponsePacket;//initialize these
struct registrationTable registrationTable[10];
int registrationTableIndex = 0;


void createConfirmationPacket221(){
	confirmationPacket.type = htons(221);
	strncpy(confirmationPacket.uname, registrationPacket.uname, sizeof(registrationPacket.uname));
}

void createAcknowledgementPacket231(){
	chatResponsePacket.type = htons(231);
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
			registrationTable[registrationTableIndex].port = clientAddr.sin_port;
			registrationTable[registrationTableIndex].sockid = new_s;
			strncpy(registrationTable[registrationTableIndex].uname, registrationPacket.uname, sizeof(registrationPacket.uname));
			strncpy(registrationTable[registrationTableIndex].mname, registrationPacket.mname, sizeof(registrationPacket.mname));

			printRegistrationTable();
		
			//Eventually this should be stored in a struct, but for now with one host, this suffices. Index will always be zero.
			registrationTableIndex = registrationTableIndex + 1;

			//send registration acceptance to client
			createConfirmationPacket221();	
			printf("\n\nConfirmation packet being sent: %u", ntohs(confirmationPacket.type));
			printf("\nConfirmation packet username: %s", confirmationPacket.uname);
			if(send(new_s, &confirmationPacket, sizeof(confirmationPacket), 0) < 0){
				printf("error");
			}
			else{
				printf("\nConfirmation packet 221 sent");

				//receive chat data packet
				while(1){
					if(recv(new_s, &chatDataPacket, sizeof(chatDataPacket), 0) < 0){
						printf("\nCould not receive the chat data packet packet\n");
						exit(1);
					}		
					//recieved chat data packet
					else{
						printf("\n\nChat data packet %u recieved successfully from server for", ntohs(chatDataPacket.type));
						printf(" user %s: ", chatDataPacket.uname);
						printf("\n%s", chatDataPacket.data);

						//send acknowledgment
						createAcknowledgementPacket231();
						if(send(new_s, &chatResponsePacket, sizeof(chatResponsePacket), 0) < 0){
							printf("error sending acknowledgement 231");
						}
						//Acknowledgement sent
						else{
							printf("\nAcknowledgement packet %u sent", ntohs(chatResponsePacket.type));
					
							//without putting this last command previous will not run for some reason.
							printf("\nAcknowledgement packet %u sent", ntohs(chatResponsePacket.type));
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
