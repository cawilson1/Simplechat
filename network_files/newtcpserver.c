#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>

#define SERVER_PORT 8017
#define MAX_LINE 256
#define MAX_PENDING 5

struct packet{
	short type;
	char uname[MAX_LINE];//manually check that this and two below are no greater than MAX_LINE in size
	char* mname;
	char* data;
};	


int main(int argc, char* argv[])
{
	
	struct packet packet_reg;	

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
		if(recv(new_s, &packet_reg, sizeof(packet_reg), 0) < 0){
			printf("\nCould not receive the first registration packet\n");
			exit(1);
		}
		//Registration packet
		else if(htons(packet_reg.type)==121){
			printf("\nRegistration packet:");
			printf("\nPacket type: ");
			printf("%u",htons(packet_reg.type));
			
			//Don't know why this isn't working.
			printf("\nUsername: ");
			printf("%c",packet_reg.uname);
			
		}

		while(len = recv(new_s, buf, sizeof(buf), 0))
			fputs(buf, stdout);
		close(new_s);




		//printf("Packet received. Username :");
		//printf(packet_reg.uname);

	}
}
