typedef struct{
	uint8_t nodeId[20];
	uint16_t port;
	char ip[16];
}Node;

typedef struct{
	uint8_t nodeIDMin[20];
	uint8_t nodeIDMax[20];
	Node nodes[8];
}KBucket;

typedef struct{
	
}KTable;
