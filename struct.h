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

struct search_node{
	unsigned char id[20];
	int sslen;
	time_t request_time;				// the time of the last unanswered request
	time_t reply_time;					// the time of the last reply
	int pinged;
	unsigned char token[40];
	int token_len;
	int replied;						// whether we have received a reply
	int acked;							// whether they acked our announcement
};

/* When performing a search, we search for up to SEARCH_NODES closest nodes to the destination, and use the additional ones to backtrack if any of the target 8 turn out to be dead */
#define SEARCH_NODES 14

struct search{
	unsigned short tid;
	int af;
	time_t step_time;					// the time of the last search_step
	unsigned char id[20];
	unsigned short port;				// 0 for pure searches
	int done;
	struct search_node nodes[SEARCH_NODES];
	int numnodes;
	struct search *next;
};

struct peer{
	time_t time;
	unsigned char ip[16];
	unsigned short len;
	unsigned short port;
};

struct storage{
	unsigned char id[20];
	int numpeers, maxpeers;
	struct peer *peers;
	struct storage *next;
};
