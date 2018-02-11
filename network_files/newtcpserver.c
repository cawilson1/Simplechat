#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>

#define SERVER_PORT 5432
#define MAX_LINE 256
#define MAX_PENDING 5

int main(int argc, char* argv[])
{
	struct packet{
			short type;
			char* uName;//manually check that this and two below are no greater than MAX_LINE in size
			char* mName;
			char* data;
		};

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
	/* initalizes len for use in the accept() function */
	len = sizeof(struct sockaddr_in);
	while(1){
		if((new_s = accept(s, (struct sockaddr *)&clientAddr, &len)) < 0){
			perror("tcpserver: accept");
			exit(1);
		}
		/* Prints request sent to server */
		printf("\n Accepted new client request, socket id is %d \n", new_s);


		if(recv(new_s, &packet_reg, sizeof(packet_reg), 0) < 0){
			printf("\n Could not receive first registration packer\n");
			exit(1);
		}
		struct registrationTable{
			int port;
			int sockid;
			char mName[MAX_LINE];
			char uName[MAX_LINE];
		};
		struct registrationTable table[10];

		table[0].port = clientAddr.sin_port;
		table[0].sockid = new_s;
		strcpy(table[0].uName, packet_reg.uName);
		strcpy(table[0].mName, packet_reg.mName);

		printf("\n Client's port is %d \n", ntohs(clientAddr.sin_port));

		/* Sends accept confirmation to client */
		/* I'm assuming 'sin' is the confirmation packet containing
		information about the server address, and I'm setting the type to 221 */
		sin.type = htons(221);
		if(send(new_s,(struct sockaddr *)&sin, sizeof(sin)) < 0){
			perror("tcpserver: send");
			exit(1);
		}
		/*prints response sent by server */
		printf("\n Server sent confirmation \n");

		/*prints prints chat message from client */
		while(len = recv(new_s, buf, sizeof(buf), 0))
			fputs(buf, stdout);
		close(new_s);

	}
}
