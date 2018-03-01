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

#define GREEN   "\x1B[32m"

struct packet{
	short type;
	char uname[MAX_LINE];
	char mname[MAX_LINE];
	char data[MAX_LINE];
	short regTableIndex;
	short seqNumber;
};

void printGreen(){
	printf(GREEN);
}

void printColorReset(){
	printf("\033[0m");
}

int main(int argc, char* argv[])
{

	struct packet registrationPacket, confirmationPacket, chatDataPacket, chatResponsePacket;

	struct hostent *hp;
	struct sockaddr_in sin;//socket id
	char *host;
	char myMachineName[MAX_LINE];
	char buf[MAX_LINE];
	char arg2Username[MAX_LINE];

	int s;
	int len;

	//After contact is made, this value should be stored
	int serverRegTableIndex;

	if(argc == 3){
		printf("-----------------New Chat Initiated--------------------\n");
		host = argv[1];
		strcpy(arg2Username,argv[2]);
	}
	else{
		fprintf(stderr, "incorrect argument count\nUse: ./<executable_file_name> <server_name_or_ip> <username>\n"
			"Example: ./client macs.citadel.edu my_username\n");
		exit(1);
	}


	/* translate host name into peer's IP address */
	hp = gethostbyname(host);//info of client
	if(!hp){
		fprintf(stderr, "unkown host: %s\n", host);
		exit(1);
	}

	//hostname for current client machine instance
	gethostname(myMachineName, sizeof myMachineName);//info of client
	printf("me: %s\n", myMachineName);

	//create registration packet from command line args
	//packet type
	registrationPacket.type = htons(121);
	//set packet machine name
	strncpy(registrationPacket.mname, myMachineName, sizeof(myMachineName));//takes 3 arguments
	//set packet username
	strncpy(registrationPacket.uname, arg2Username, sizeof(arg2Username));

	printf("Username: ");
	printf(registrationPacket.uname);

	/* active open */
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("tcpclient: socket");
		exit(1);
	}

	/* build address data structure */
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = htons(SERVER_PORT);

//s = socket
	if(connect(s,(struct sockaddr *)&sin, sizeof(sin)) < 0){
		perror("tcpclient: connect");
		close(s);
		exit(1);
	}

	//send the registration packet to the server two more times
	for(int i = 0; i < 2; i++){
	if(send(s, &registrationPacket, sizeof(registrationPacket), 0) < 0)
	{
		printf("\nSend failed\n");
		exit(1);
	}
	//registration packet sent without error
	else{
		printf("\nSent packet %u registration packet ", ntohs(registrationPacket.type));
		printf("\n-----------------------------------------------------\n");
		//receive confirmation packet
		while(1){
		if(recv(s, &confirmationPacket, sizeof(confirmationPacket), 0) < 0){
			printf("\ndidn't receive confirmation packet");
			exit(1);
		}
		//confirmation packet received without error
		else{
			printf("\n\nConfirmation packet %u recieved successfully from server for", ntohs(confirmationPacket.type));
			printf(" user\n%s\n", arg2Username);

			//using confirmation packet username instead of arg2Username to ensure packets are being passed correctly
			printf("\n[%s]:", confirmationPacket.uname);
		}
	}
}
}

			//set server registration table index
			serverRegTableIndex = ntohs(confirmationPacket.regTableIndex);

			//loop to receive input to send the chat data packet to the server
			while(fgets(buf, sizeof(buf),stdin)){
				buf[MAX_LINE-1] = '\0';
				len = strlen(buf) + 1;
				//create chat data packet
				chatDataPacket.type = htons(131);
				strncpy(chatDataPacket.uname,confirmationPacket.uname,sizeof(confirmationPacket.uname));
				strncpy(chatDataPacket.data,buf,len);
				chatDataPacket.regTableIndex = htons(serverRegTableIndex);

				//send chat data packet
				if(send(s, &chatDataPacket,sizeof(chatDataPacket),0) < 0){
					printf("\nchat data packet error");
					exit(1);
				}
				//chat packet sent successfully
				else{
					//chatResponsePacket (acknowledgement of reception of previous packet) from server
					if(recv(s, &chatResponsePacket, sizeof(chatResponsePacket), 0) < 0){
						printf("didn't receive acknowledgement");
						exit(1);
					}
					else{
						//message recieved
						printGreen();
						printf("[%s]:", chatResponsePacket.uname);
						printf("%s", chatResponsePacket.data);
						printColorReset();

						//prompt for user to type next method
						printf("\n[%s]:", chatResponsePacket.uname);
					}

				}


	}

	}
	//notes:
//one thread registers/getting input from new clients, second completes registration,  last one serves existing clients,
		//reads table, sends to all clients
//main prog in server handles the registration process
//multicaster sends data to all clients
//once the join handler updates the table, it terminates the thread
//join handler uses new_s, main thread is using original s




}
