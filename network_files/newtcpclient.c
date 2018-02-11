#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<sys/param.h>

#define SERVER_PORT 8017
#define MAX_LINE 256

struct packet{
	short type;
	char* uname;//manually check that this and two below are no greater than MAX_LINE in size
	char* mname;
	char* data;
};	


int main(int argc, char* argv[])
{

	struct packet packet_reg;

	struct hostent *hp;
	struct sockaddr_in sin;
	char *host;
	char myMachineName[MAXHOSTNAMELEN];
	char buf[MAX_LINE];
	int s;
	int len;

	fprintf(stdout,"argc is %d\n", argc); 
	if(argc == 3){
		host = argv[1];
		packet_reg.uname=argv[2];//strcpy causes segmentation fault
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
	packet_reg.type = htons(121);
	//set packet machine name
	strcpy(packet_reg.mname, myMachineName);	
	//packet_reg.mname = myhostname;

	printf("Username: ");
	printf(packet_reg.uname);
	printf("\nINFO: \n");
	printf("%u\n", ntohs(packet_reg.type));
	printf(packet_reg.mname);

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
	/* main loop: get and send lines of text */
	while(fgets(buf, sizeof(buf), stdin)){
		//send the registration packet to the server
		if(send(s, &packet_reg, sizeof(packet_reg), 0) < 0)
		{
			printf("\nSend failed\n");
			exit(1);
		}
		else{
			printf("%u", ntohs(packet_reg.type));	
		}	
	//	buf[MAX_LINE-1] = '\0';
	//	len = strlen(buf) + 1;
	//	send(s, buf, len, 0);
	}


}
