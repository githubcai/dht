struct node{
	unsigned char id[20];
	struct sockaddr_storage ss;
	int sslen;
	time_t time;				// time of last message received
	time_t reply_time;			// time of last correct reply received
	time_t pinged_time;			// time of last request
	int pinged;					// how many requests we sent since last reply
	struct node *next;
};

struct bucket{
	int af;
	unsigned char first[20];
	int count;							// number of nodes
	int time;							// time of last reply in this bucket
	struct node *nodes;
	struct sockaddr_storage cached;		// the address of a likely candidate
	int cachedlen;
	struct bucket *next;
};
