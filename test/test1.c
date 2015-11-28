#include <stdio.h>
#include <stdlib.h>

typedef struct{
	uint8_t nodeID[20];
	char ip[15];
	int port;
}Node;

typedef struct{
	uint8_t nodeIDMin[20];
	uint8_t nodeIDMax[20];
	Node *nodes[8];
	int count;
	int time;
}KBucket;

typedef struct{
	KBucket 
}KTable;

int main(int argc, char *argv[]){
	
	
	exit(EXIT_FAILURE);
}
