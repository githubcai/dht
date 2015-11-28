#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "fatal.h"

typedef struct{
	char nodeID[20];
	char ip[15];
	int port;
}Node;

void GetNodeInfo(const char *input, Node *node){
	char ip[4], port[2];
	strncpy(node->nodeID, input, 20);
	strncpy(ip, input+20, 4);
	inet_ntop(AF_INET, ip, node->ip, 15);
	node->port = ntohs(*((uint16_t *)port));
}

int GetEachNode(const char *input, char *result){
	char *p, *p2, *temp, len[5];
	int i, length;
	p = strstr(input, "nodes");
	//printf("In the GetEachNode: %s\n", p);
	
	p2 = index(p, ':');
	for(i=0,temp=p+5;temp!=p2;temp++,i++){
		len[i] = *temp;
	}
	len[i] = '\0';
	length = atoi(len);
	
	strncpy(result, p2+1, length);

	//printf("new: %d %s\n", atoi(len), len);
	return length;
}

int main(int argc, char *argv[]){
	int sfd;
	//char con[] = "d1:ad2:id20:abcdefsaij0123456789e1:q4:ping1:t2:aa1:y1:qe";
	char con[] = "d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:aa1:y1:qe";
	//char con[] = "d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:aa1:y1:qe";
	char resp[1000];
	struct sockaddr_in claddr;
	ssize_t numBytes;
	int i, temp;
	char test[] = {-61,-102,-24,-22};
	char port[] = {-56,-43}; 
	Node node;
	char result[500];

	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sfd==-1){
		Error("scoket");
	}

	bzero(&claddr, sizeof(struct sockaddr_in));
	claddr.sin_family = AF_INET;
	claddr.sin_addr.s_addr = inet_addr("195.154.232.234");//("91.121.59.153");
	claddr.sin_port = htons/*(54958);*/(51413);
	
	if(sendto(sfd, con, strlen(con), 0, (struct sockaddr*)&claddr, sizeof(claddr))<0){
		Error("sendto");
	}

	numBytes = recvfrom(sfd, resp, 1000, 0, NULL, NULL);
	if(numBytes==-1){
		Error("recvfrom");
	}

	printf("Response %d: %s\n", (int)numBytes, resp);
	for(i=0;i<numBytes;i++){
		printf("%u,", resp[i]);
	}
	printf("\n");
	i = GetEachNode(resp, result);
	printf("return len: %d, result len: %d, result: %s\n", i, (int)strlen(result), result);
	for(temp=0;temp<i;temp+=26){
		GetNodeInfo(result+temp, &node);
		printf("ip: %s, port: %d\n", node.ip, node.port);
	}

	printf("%u\n", ntohl(*(uint32_t*)test));

	inet_ntop(AF_INET,test,resp,i);
	printf("%s\n", resp);
	printf("%d\n",port[0]);
	printf("%u\n", ntohs(*((uint16_t *)port)));

	exit(EXIT_SUCCESS);
}
