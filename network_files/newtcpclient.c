#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<sys/param.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>

#define SERVER_PORT 8047
#define MAX_LINE 256

#define GREEN   "\x1B[32m"

struct packet{
	short type;
	char uname[MAX_LINE];
	char mname[MAX_LINE];
	char data[MAX_LINE];
	short regTableIndex;
	short seqNumber;
	char groupId[MAX_LINE];//chat room
	short rttDelay;
};

struct rttFinder{
	short seqNumber;//seqNumber to keep track of packet
	long long sendTime;
	long long returnTime;
	short rtt;
};

int globalAvgTimeList[100];
int currentQueueIndex = 0;
char globalMachineName[MAX_LINE];
char globalUsername[MAX_LINE];
char globalGroupId[MAX_LINE];
struct timeval start, stop, initPause;
short globalSequenceNumber = 0;//sequence number for sync packets
short dataGlobalSequenceNumber = 0;//sequence number for data packets
struct rttFinder rttList[1000];//holds the rtts of up to 1000 packets
int allSetupPacketsSentBool = 0;//don't send a/v packets until setup complete
int startSendingAvPackets = 0;//wait until confirmation from server to send data packets

short getDataGlobalSequenceNumber(){
	short retVal = dataGlobalSequenceNumber;
	dataGlobalSequenceNumber = dataGlobalSequenceNumber + 1;
	if(dataGlobalSequenceNumber >= 1000){
		dataGlobalSequenceNumber = 0;
	}

	return retVal;
}

short getSequenceNumber(){

	short retVal = globalSequenceNumber;
	globalSequenceNumber = globalSequenceNumber + 1;
	if(globalSequenceNumber >= 1000){
		globalSequenceNumber = 0;
	}

	return retVal;
}

//everything that user is not actively typing is green
void printGreen(){
	printf(GREEN);
}

//set back to default color
void printColorReset(){
	printf("\033[0m");
	printf("\n");
}

short findTimeListAvg(){
	double retVal = 0;
	int sum = 0;
	int i = 0;
	for(i; i < 100; i++){
		sum = sum + globalAvgTimeList[i];
		retVal = sum / 100;
	}
	return (short)floor(retVal);
}

int getTimeListIndex(){
	int retVal = currentQueueIndex;
	currentQueueIndex++;
	if(currentQueueIndex>=100){
		currentQueueIndex = 0;
	}
	return retVal;
}

void addToTimeList(int time){
	globalAvgTimeList[getTimeListIndex()] = time;
}

void *sendPackets(void* arg){
	struct packet sendDataPacket, syncPacket, syncPacketResp;
	struct rttFinder currentRttFinder;
	int s = (intptr_t)(void*)arg;//convert arg back to int
	
	int i = 0;

	syncPacket.type = htons(801);//packet type
	strncpy(syncPacket.mname, globalMachineName, sizeof(globalMachineName));//set client machine name
	strncpy(syncPacket.uname, globalUsername, sizeof(globalUsername));//set client username
	strncpy(syncPacket.groupId, globalGroupId, sizeof(globalGroupId));//set chatroom name

	int stay = 1;
	for(i=0; i < 150 && stay; i++){
		//initPause.tv_nsec = 20000000;//20 ms
		if(nanosleep((const struct timespec[]){{0,60000000L}}, NULL)<0){
			printf("\nnanosleep failed");
		}

		short tempSeqNum = getSequenceNumber();
		syncPacket.seqNumber = htons(tempSeqNum);
		gettimeofday(&start, 0);
		if(send(s, &syncPacket, sizeof(syncPacket), 0) < 0){
			printf("\nSend failed\n");
			exit(1);
		}
		else{
			long long tempStart = (((long long) start.tv_sec)*1000)+(start.tv_usec/1000);
			//printf("\nSystem time before sending packet = %lu", tempStart	);
			printf("\nsync packet successfully sent with seqNumber %i. Updating rttList", ntohs(syncPacket.seqNumber));
			currentRttFinder.seqNumber = tempSeqNum;
			currentRttFinder.sendTime = tempStart;
			rttList[tempSeqNum] = currentRttFinder;
			
		}
		//packets have been sent and given ample time to return
		if(i == 148){
		}
	}
			//send packet to let server know that client is ready to start sending a/v
	syncPacket.type = htons(501);
	strncpy(syncPacket.uname, globalUsername, sizeof(globalUsername));

	if(send(s, &syncPacket, sizeof(syncPacket), 0) < 0){
		printf("\nfailed to send initiate packet");
		exit(1);
	}
	else{
		printf("\nclient has let server know that it is ready to start sending. Waiting on another client.");
	}

		sleep(1);//short delay for organization
		while(1){//excuse the formatting
			if (startSendingAvPackets){
					printf("\n the value inside of startSendingAvPackets is %i", startSendingAvPackets);

				i = 0;
				for(i; i < 5; i++){
					sendDataPacket.type = htons(901);//packet type
					sendDataPacket.rttDelay = htons(findTimeListAvg());
					strncpy(sendDataPacket.mname, globalMachineName, sizeof(globalMachineName));//set client machine name
					strncpy(sendDataPacket.uname, globalUsername, sizeof(globalUsername));//set client username
					strncpy(sendDataPacket.groupId, globalGroupId, sizeof(globalGroupId));//set chatroom name
					
					printf("\n the username being sent is %s", sendDataPacket.uname);
			
					if(send(s, &sendDataPacket, sizeof(sendDataPacket), 0) < 0){
						printf("\nSend failed\n");
						exit(1);
					}
					else{
						printf("\na/v data packet successfully sent with current avg rtt:%i", ntohs(sendDataPacket.rttDelay));
					}
					sleep(1);
				}
				syncPacket.type = htons(801);//packet type
				syncPacket.rttDelay = htons(findTimeListAvg());
				strncpy(syncPacket.mname, globalMachineName, sizeof(globalMachineName));//set client machine name
				strncpy(syncPacket.uname, globalUsername, sizeof(globalUsername));//set client username
				strncpy(syncPacket.groupId, globalGroupId, sizeof(globalGroupId));//set chatroom name
		
				short tempSeqNum = getSequenceNumber();
				syncPacket.seqNumber = htons(tempSeqNum);
		
				gettimeofday(&start, 0);
				if(send(s, &syncPacket, sizeof(syncPacket), 0) < 0){
					printf("\nSend failed\n");
					exit(1);
				}
				else{
					long long tempStart = (((long long) start.tv_sec)*1000)+(start.tv_usec/1000);
					printf("\nsync packet successfully sent with seqNumber %i. Updating rttList", ntohs(syncPacket.seqNumber));
					currentRttFinder.seqNumber = tempSeqNum;
					currentRttFinder.sendTime = tempStart;
					rttList[tempSeqNum] = currentRttFinder;
					printf("\nrtt avgerage of last 100 packets %i", findTimeListAvg());
	
				}
			}
		}
//	}
}

short calculateTimeDiff(short seqNum){
	rttList[seqNum].rtt = (short)(rttList[seqNum].returnTime - rttList[seqNum].sendTime);	
	return rttList[seqNum].rtt;
}

//receive packets from other clients in this room as well as sync acknowledgement packets
void *recvServerPackets(void* arg){

	struct packet chatResponsePacket;
	int s = (intptr_t)(void*)arg;//convert arg back to int

	while(1){

		//chat packet sent successfully
		//chatResponsePacket (acknowledgement of reception of previous packet) from server
		if(recv(s, &chatResponsePacket, sizeof(chatResponsePacket), 0) < 0){
			printf("didn't receive acknowledgement");
			exit(1);
		}

		else if (chatResponsePacket.type == ntohs(301)){//chat data
			//message recieved
			printf("\nreceived a chat data packet");
			printGreen();
			printf("\n[%s]:", chatResponsePacket.uname);//name of user
			printf("%s", chatResponsePacket.data);//data sent
			printColorReset();
		}
		else if (chatResponsePacket.type == ntohs(811)){//received ack
			//if(nanosleep((const struct timespec[]){{0,30000000L}}, NULL)<0){
				//for the client with artificial delay. Compile with diff name and comment this out	
//			}
			gettimeofday(&stop, 0);
			long long tempStopTime = (((long long) stop.tv_sec)*1000)+(stop.tv_usec/1000);
			printf("\nReceived sync packet number %i", ntohs(chatResponsePacket.seqNumber));
			short currentSeqNum = ntohs(chatResponsePacket.seqNumber);
			rttList[currentSeqNum].returnTime = tempStopTime;
			short difference = calculateTimeDiff(currentSeqNum);
			printf("\nThe time difference for packet %i is %i milliseconds.", currentSeqNum, difference);
			addToTimeList(difference);
			//at least 100 timing packets have been received. Now a/v packets can be sent.	
			if(currentSeqNum >= 99){
				allSetupPacketsSentBool = 1;
			}
		}
		else if(ntohs(chatResponsePacket.type) == 999){//print the buffer time
			printf("\nthe buffer time from here to host with socket id %i is %i ms", ntohs(chatResponsePacket.seqNumber), ntohs(chatResponsePacket.rttDelay));
		}
		else if (ntohs(chatResponsePacket.type) == 401){//start sending av packets
			startSendingAvPackets = 1;
			printf("\n start sending a/v packets");
		}
		else{
			printf("not a valid packet type. type is %i", ntohs(chatResponsePacket.type));
			exit(1);
		}
	}	
}

void updateGlobals(char mname[MAX_LINE], char uname[MAX_LINE], char groupId[MAX_LINE]){
	strncpy(globalMachineName,mname, sizeof(mname));
	strncpy(globalUsername,uname, sizeof(uname));
	strncpy(globalGroupId, groupId, sizeof(groupId));
	printf("\nlocal uname: %s, globalUsername: %s", uname, globalUsername);
}

int main(int argc, char* argv[]){

	printColorReset();
	struct packet registrationPacket, confirmationPacket, chatDataPacket;

	struct hostent *hp;
	struct sockaddr_in sin;
	char *host;
	char myMachineName[MAX_LINE];
	char buf[MAX_LINE];
	char arg2Username[MAX_LINE];
	

	pthread_t threads[2];
	int s;
	int len;

	//groupId is the chat room
	char groupId[MAX_LINE];

	//After contact is made, this value should be stored
	int serverRegTableIndex;

	if(argc == 4){
		printf("-----------------New Chat Initiated--------------------\n");
		host = argv[1];
		strcpy(arg2Username,argv[2]);
		strncpy(groupId, argv[3], sizeof(argv[3]));
	}
	else{
		fprintf(stderr, "incorrect argument count\nUse: ./<executable_file_name> <server_name_or_ip> <username><groupId>\n"
			"Example: ./client macs.citadel.edu my_username 4\n");
		exit(1);
	}


	/* translate host name into peer's IP address */
	hp = gethostbyname(host);
	if(!hp){
		fprintf(stderr, "unkown host: %s\n", host);
		exit(1);
	}
	
	//hostname for current client machine instance
	gethostname(myMachineName, sizeof myMachineName);
	printf("me: %s\n", myMachineName);

	updateGlobals(myMachineName, arg2Username, groupId);


	//create registration packet from command line args
	registrationPacket.type = htons(121);//packet type
	strncpy(registrationPacket.mname, myMachineName, sizeof(myMachineName));//set client machine name
	strncpy(registrationPacket.uname, arg2Username, sizeof(arg2Username));//set client username
	strncpy(registrationPacket.groupId, groupId, sizeof(groupId));//set chatroom name

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
		perror("tcpclient: connect");
		close(s);
		exit(1);
	}

	//send reg packet three
	int i;
	for(i = 0; i < 3; i++){
		//send the registration packet to the server
		gettimeofday(&start, 0);
		printf("current system time at program start = %lu\n", (((long long) start.tv_sec)*1000) + (start.tv_usec/1000));

		if(send(s, &registrationPacket, sizeof(registrationPacket), 0) < 0)
		{
			printf("\nSend failed\n");
			exit(1);
		}
		//registration packet sent without error
		else{
			printf("\nSent packet %u registration packet %i", ntohs(registrationPacket.type), i + 1); 
			printf("\n-----------------------------------------------------\n");
		}
		
	}

	//receive confirmation packet
	if(recv(s, &confirmationPacket, sizeof(confirmationPacket), 0) < 0){
		printf("\ndidn't receive confirmation packet");
		exit(1);
	}
		//confirmation packet received without error
		else{
			long tempStop = (((long long) start.tv_sec)*1000) + (start.tv_usec/1000) + 30;
			printf("System time after receiving packet = %lu\n", tempStop);	

			printf("\n\nConfirmation packet %u recieved successfully from server for", ntohs(confirmationPacket.type));
			printf(" user\n%s\n", arg2Username);

			//using confirmation packet username instead of arg2Username to ensure packets are being passed correctly
			printf("\n[%s]:", confirmationPacket.uname);

			//set server registration table index
			serverRegTableIndex = ntohs(confirmationPacket.regTableIndex);
	}

	pthread_create(&threads[1], NULL, sendPackets, (void*)(intptr_t)s);//thread to listen for others in chatroom. socket is argument
	pthread_create(&threads[0], NULL, recvServerPackets, (void*)(intptr_t)s);//thread to listen for others in chatroom. socket is argument

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
		//else sent successfully
	}
}
