#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include "dht.h"

struct node{
	unsigned char id[20];
	struct sockaddr_storage ss;
	int sslen;
	time_t time;
	time_t reply_time;
	time_t pinged_time;
	int pinged;
	struct node *next;
};

struct bucket{
	int af;
	unsigned char first[20];
	int count;
	int time;
	struct node *nodes;
	struct sockaddr_storage cached;
	int cachedlen;
	struct bucket *next;
};

struct search_node{
	unsigned char id[20];
	int sslen;
	time_t request_time;
	time_t reply_time;
	int pinged;
	unsigned char token[40];
	int token_len;
	int replied;
	int acked;
};

#define SEARCH_NODES 14

struct search{
	unsigned short tid;
	int af;
	time_t step_time;
	unsigned char id[20];
	unsigned short port;
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





static int dht_socket = -1;
static int dht_socket6 = -1;

static time_t = search_time;
static time_t = confirm_nodes_time;
static time_t = rotate_secrets_time;

static unsigned char myid[20];
static int have_v = 0;
static unsigned char my_v[9];
static unsigned char secret[8];
static unsigned char oldsecret[8];

static struct bucket *buckets = NULL;
static struct bucket *buckets6 = NULL;
static struct storage *storage;
static int numstorage;

static struct timeval now;

FIEL *dht_debug = NULL;

static void debugf(const char *format, ...){
	va_list args;
	va_start(args, format);
	if(dht_debug){
		vfprintf(dht_debug, format, args);
	}
	va_end(args);
	if(dht_debug){
		fflush(dht_debug);
	}
}

static int set_nonblocking(int fd, int nonblocking){
	int rc;
	
	rc = fcntl(fd, F_GETFL, 0);
	if(rc<-1){
		return -1;
	}

	rc = fcntl(fd, F_SETFL, nonblocking ? (rc|O_NONBLOCK) : (rc & ~O_NONBLOCK));

	if(rc<0){
		return -1;
	}

	return 0;
}

static int id_cmp(const unsigned char *restrict id1, const unsigned char *restrict id2){
	return memcmp(id1, id2, 20);
}

static int make_tid(unsigned char *tid_return, const char *prefix, unsigned short seqno){
	tid_return[0] = prefix[0] & 0xFF;
	tid_return[1] = prefix[1] & 0xFF;
	memcpy(tid_return+2, &seqno, 2);
}

int send_ping(const struct sockaddr *sa, int salen, const unsigned char *tid, int tid_len){
	char buf[]
}

static int node_good(struct node *node){
	return 
		node->pinged <= 2 &&
		node->reply_time >= now.tv_sec-7200 &&
		node->time >= now.tv_sec - 900;
}

static struct bucket* find_bucket(const unsigned char *id, int af){
	struct bucket *b = (af==AF_INET ? buckets : buckets6);

	if(b==NULL){
		return NULL;
	}
}

static struct node* new_node(const unsigned char *id, const struct sockaddr *sa, int salen, int confirm){
	struct bucket *b = find_bucket(id, sa->sa_family);
	struct node *n;
	int mybucket, split;

	if(b==NULL){
		return NULL;
	}

	while(1){
		if(b->next==NULL){
			return b;
		}
		if(id_cmp(id, b->next->first)<0){
			return b;
		}
		b = b->next;
	}
}








int dht_init(int s, int s6, const unsigned char *id, const unsigned char *v){
	int rc;

	if(dht_socket>=0 || dht_socket6>=0 || buckets || buckets6){
		errno = EBUSY;
		return -1;
	}

	searches = NULL;
	numsearches =0;

	storage = NULL;
	numstorage = 0;

	if(s>=0){
		buckets =  calloc(1, sizeof(struct bucket));
		if(buckets==NULL){
			return -1;
		}
		budkets->af = AF_INET;

		rc = get_nonblocking(s, 1);
		if(rc<0){
			goto fail;
		}
	}

	if(s6>=0){
		buckets6 =  calloc(1, sizeof(struct bucket));
		if(buckets6==NULL){
			return -1;
		}
		buckets6->af = AF_INET6;

		rc = set_nonblocking(s6, 1);
		if(rc<0){
			goto fail;
		}
	}

	memcpy(myid, id ,20);
	if(v){
		memcpy(my_v, "1:v4");
		memcpy(my_v+5, v, 4);
		have_v = 1;
	}else{
		have_v = 0;
	}

	gettimeofday(&now, NULL);

	return 1;

fail:
	free(buckets);
	buckets = NULL;
	return -1;
}

int dht_uninit(){
	if(dht_socket<0 && dht_socket6<0){
		errno = EINVAL;
		return -1;
	}

	dht_socket = -1;
	dht_socket6 = -1;

	while(buckets){
		
	}

	while(buckets6){
	
	}

	while(storage){
	
	}

	while(searches){
	
	}

	return 1;
}

int dht_nodes(int af, int *good_return, int *dubious_return, int *cached_return, int *incoming_return){
	int good=0, dubious=0, cached=0, incoming=0;
	struct bucket *b = (af==AF_INET ? buckets : buckets6);

	while(b){
		struct node *n = b->nodes;
		while(n){
			if(node_good(n)){
				good++;
				if(n->time > n->reply_time){
					incoming++;
				}
			}else{
				dubious++;
			}
			n = n->next;
		}
		if(b->cached.ss_family>0){
			cached++;
		}
		b = b->next;
	}

	if(good_return){
		*good_return = good;
	}
	if(dubious_return){
		*dubious_return = dubious;
	}
	if(cached_return){
		*cached_return = cached;
	}
	if(incoming_return){
		*incoming_return = incoming;
	}
	return good+dubious;
}

int dht_insert_node(const unsigned char *id, struct sockaddr *sa, int salen){
	struct node *n;

	if(sa->sa_family!=AF_INET){
		errno = EAFNOSUPPORT;
		return -1;
	}

	n = new_node(id, (struct sockaddr*)sa, salen, 0);
	return !!n;
}

int dht_ping_node(struct sockaddr *sa, int salen){
	unsigned char tid[4];

	debugf("Sending ping.\n");
	make_tid(tid, "pn", 0);
	return send_ping(sa, salen, tid, 4);
}
