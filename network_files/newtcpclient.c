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

struct packet{
	short type;
	char uname[MAX_LINE];
	char mname[MAX_LINE];
	char data[MAX_LINE];
};	


int main(int argc, char* argv[])
{

	struct packet registrationPacket, confirmationPacket, chatDataPacket, chatResponsePacket;

	struct hostent *hp;
	struct sockaddr_in sin;
	char *host;
	char myMachineName[MAX_LINE];
	char buf[MAX_LINE];
	char arg2Username[MAX_LINE];

	int s;
	int len;

	if(argc == 3){
		printf("-----------------New Chat Initiated--------------------\n");
		host = argv[1];
		strcpy(arg2Username,argv[2]);
	}
	else{
		fprintf(stderr, "incorrect argument count\n");
		exit(1);
	}


	/* translate host name into peer's IP address */
	hp = gethostbyname(host);//old way
	if(!hp){
		fprintf(stderr, "unkown host: %s\n", host);
		exit(1);
	}
	
	//hostname for current client machine instance
	gethostname(myMachineName, sizeof myMachineName);
	printf("me: %s\n", myMachineName);

	//create registration packet from command line args
	//packet type
	registrationPacket.type = htons(121);
	//set packet machine name
	strncpy(registrationPacket.mname, myMachineName, sizeof(myMachineName));
	//set packet username
	strncpy(registrationPacket.uname, arg2Username, sizeof(arg2Username));	
	//registrationPacket.mname = myhostname;

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

	
	if(connect(s,(struct sockaddr *)&sin, sizeof(sin)) < 0){
		perror("tcpclient: connectasdfasdf");
		close(s);
		exit(1);
	}

	//send the registration packet to the server
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
		if(recv(s, &confirmationPacket, sizeof(confirmationPacket), 0) < 0){
			printf("\ndidn't receive confirmation packet");
			exit(1);
		}
		//confirmation packet received without error
		else{
			printf("\n\nConfirmation packet %u recieved successfully from server for", ntohs(confirmationPacket.type));
			printf(" user %s\n", confirmationPacket.uname);

			//loop to receive input to send the chat data packet to the server
			while(fgets(buf, sizeof(buf),stdin)){
				buf[MAX_LINE-1] = '\0';
				len = strlen(buf) + 1;
				chatDataPacket.type = htons(131);
				strncpy(chatDataPacket.uname,confirmationPacket.uname,sizeof(confirmationPacket.uname));
				strncpy(chatDataPacket.data,buf,len);
				//send chat data packet
				if(send(s, &chatDataPacket,sizeof(chatDataPacket),0) < 0){
					printf("\nchat data packet error");
					exit(1);
				}
				//chat packet sent successfully
				else{
					printf("Chat data packet %u sent successfully\n", ntohs(chatDataPacket.type));

					//chatResponsePacket (acknowledgement of reception of previous packet) from server
					if(recv(s, &chatResponsePacket, sizeof(chatResponsePacket), 0) < 0){
						printf("didn't receive acknowledgement");
						exit(1);
					}
					else{
						printf("\nReceived acknowledgement packet type %u from server", ntohs(chatResponsePacket.type));
						printf("\nEnter chat to send: ");
					}
					
				}
			}
		}
			
	}
	/* main loop: get and send lines of text */
	//while(fgets(buf, sizeof(buf), stdin)){
	
	//	buf[MAX_LINE-1] = '\0';
	//	len = strlen(buf) + 1;
	//	send(s, buf, len, 0);
	//}


}
