#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fatal.h"
#include <arpa/inet.h>

typedef struct{
	uint8_t nodeID[21];
	uint16_t port;
	char ip[16];
}Node;

void StructNodeFromBinary(const uint8_t *input, Node *node){
	uint8_t ip[4];
	uint16_t port;
	memset(node, 0, sizeof(Node));
	strncpy(node->nodeID, input, 20);
	strncpy(ip, input+20, 4);
	inet_ntop(AF_INET, ip, node->ip, 15);
	node->port = ntohs(port);
}

int GetNodesMapFromResp(const uint8_t *input, uint8_t *result){
	char *ctemp, *lNodes, length[5];
	int itemp, len;
	ctemp = strstr(input, "nodes");
	lNodes = index(ctemp, ':');

	for(itemp=0, ctemp+=5;ctemp!=lNodes;ctemp++,itemp++){
		length[itemp] = *ctemp;
	}
	length[itemp] = '\0';
	len = atoi(length);

	strncpy(result, lNodes+1, len);

	return len;
}

int main(int argc, char *argv[]){
	exit(EXIT_SUCCESS);
}
