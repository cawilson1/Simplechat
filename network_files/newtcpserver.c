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

		printf("\n Client's port is %d \n", ntohs(clientAddr.sin_port)); 

		while(len = recv(new_s, buf, sizeof(buf), 0))
			fputs(buf, stdout);
		close(new_s);

	}
}
