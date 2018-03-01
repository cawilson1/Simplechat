#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<sys/param.h>
#include<pthread.h>

#define SERVER_PORT 9025
#define MAX_LINE 256
#define MAX_PENDING 5

struct packet{
	short type;
	char uname[MAX_LINE];
	char mname[MAX_LINE];
	char data[MAX_LINE];
	short regTableIndex;
	short seqNumber = 0;
};

struct registrationTable{
	int port;
	int sockid;
	char mname[MAX_LINE];
	char uname[MAX_LINE];
};
struct registrationTable client_info{
	int port;
	int sockid;
	char mname[MAX_LINE];
	char uname[MAX_LINE];
}
pthread_t threads[2];
pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;

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
while(1){
void *multicaster(){
	char *filename;
	char text[100];
	int fd;

	filename = "input.txt";
	fd = open(filename, O_RDONLY, 0);
	//check whether any client is listed of the table
	int socket;
	pthread_mutex_lock(&my_mutex);
	for(int i = 0; i < sizeof(registrationTable), i++){
	socket = registrationTable[i];
	if(!socket == NULL){
	//if at least one client is listed, read 100 bytes of data from the file and
	//store it in text
	nread = read(fd, text, 100);
	break;
}
}
pthread_mutex_unlock(&my_mutex);
	//construct the data Packet
	//how does this recv() know what new_s is?
		if(recv(new_s, &chatDataPacket, sizeof(chatDataPacket), 0) < 0){
			printf("\nCould not receive the chat data packet packet\n");
			exit(1);
		}
		//recieved chat data packet
		else{
			printf("\nChat data packet %u recieved successfully from server for:", ntohs(chatDataPacket.type));
			printf("\n[%s]: ", chatDataPacket.uname);
			printf(" %s", chatDataPacket.data);


			pthread_mutex_lock(&my_mutex);

			createAcknowledgementPacket231();
			//updates sequence number--is this the right packet to update sequence number on?
			chatDataPacket.seqNumber = seqNumber;
			//send data packets to each client listed on the table
			for(int i = 0; i < sizeof(registrationTable); i++){
				if(!registrationTable[i].sockid == NULL){
					socket = registrationTable[i].sockid;
			if(send(socket, &chatResponsePacket, sizeof(chatResponsePacket), 0) < 0){
				printf("error sending acknowledgement 231\n");
				exit(1);
			}
			//Acknowledgement sent to echo chat on client side
			else{
				printf("Acknowledgement packet %u sent to %s\n", ntohs(chatResponsePacket.type), chatResponsePacket.uname);
			}
		}
	}
	pthread_mutex_unlock(&my_mutex);
	}
}

void *join_handler(struct registrationTable *clientData){
	int newsock;
	struct packet packet_reg;
	newsock = clientData->sockid;

	if(recv(newsock, &packet_reg, sizeof(packet_reg), 0) < 0){
		printf("\n Could not receive\n");
		exit(1);
	}
	//to receive third packet
	if(recv(newsock, &packet_reg, sizeof(packet_reg), 0) < 0){
		printf("\n Could not receive\n");
		exit(1);
	}
	//send registration acceptance to client
	createConfirmationPacket221();
//send acknowledgement/confirmation to the clients
	printf("\n\nConfirmation packet %u being sent ", ntohs(confirmationPacket.type));
	printf("to user: %s", confirmationPacket.uname);
	if(send(new_s, &confirmationPacket, sizeof(confirmationPacket), 0) < 0){
		printf("...error sending confirmation packet");
		exit(1);
	}
	else{
		printf("...sent successfully");
	}


	//update the table
	//add current message received to registration table
	pthread_mutex_lock(&my_mutex);
	registrationTable[registrationTableIndex].port = ntohs(clientData.sin_port);
	registrationTable[registrationTableIndex].sockid = clientData.sockid
	strncpy(registrationTable[registrationTableIndex].uname, clientData.uname, sizeof(clientData.uname));
	strncpy(registrationTable[registrationTableIndex].mname, clientData.mname, sizeof(clientData.mname));
	registrationTableIndex = registrationTableIndex + 1;
	printRegistrationTable();
	pthread_mutex_unlock(&my_mutex);

	//leave the thread
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
	pthread_create(&threads[1], NULL, chat_multicaster, NULL);

	/* wait for connection, then receive and print text */
	while(1){
		if((new_s = accept(s, (struct sockaddr *)&clientAddr, &len)) < 0){
			perror("tcpserver: accept");
			exit(1);
		}

		printf("\nClient's port is %d \n", ntohs(clientAddr.sin_port));

		//error, recieves clients send()
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


			//assigns info from this registration to new variable client_info of type registrationTable
			client_info.sockid = new_s;
			strncpy(client_info.uname, registrationPacket.uname, sizeof(registrationPacket.uname));
			strncpy(client_info.mname, registrationPacket.mname, sizeof(registrationPacket.mname));
			client_info.port = ntohs(clientAddr.sin_port);

			//create join handler thread
			pthread_create(&threads[0], NULL, join_handler, &client_info);
			pthread_join(threads[0], &exit_value);




			}

			//for some reason the last step does not execute until the program has concluded, so this is last step
			printf("\nUsername: ");
			printf("%s", registrationPacket.uname);

		}



	}
