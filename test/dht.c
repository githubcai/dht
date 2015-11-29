#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "fatal.h"

#define MAX(x, y) ((x) >= (y) ? (x) : (y))
#define MIN(x, y) ((x) <= (y) ? (x) : (y))

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
    unsigned char first[20];
    int count;
    int time;
    struct node *nodes;
    struct sockaddr_storage cached;
    int cachedlen;
    struct bucket *next;
};

typedef struct{
	char nodeID[20];
	char ip[15];
	int port;
}Node;

static struct bucket *buckets = NULL;

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

static int
set_nonblocking(int sfd, int nonblocking){
	int rc;
	rc = fcntl(sfd, F_GETFL, 0);
	if(rc < 0){
		return -1;
	}

	rc = fcntl(sfd, F_SETFL, nonblocking ? (rc | O_NONBLOCK) : (rc & ~O_NONBLOCK));
	if(rc < 0){
		return -1;
	}

	return 0;
}

static void
print_hex(FILE *f, const unsigned char *buf, int buflen){
    int i;
    for(i = 0; i < buflen; i++){
        fprintf(f, "%02x", buf[i]);
    }
}

static int
id_cmp(const unsigned char *id1, const unsigned char *id2){
    return memcmp(id1, id2, 20);
}

static int
lowbit(const unsigned char *id){
    int i, j;
    for(i = 19; i >= 0; i--){
        if(id[i] != 0){
            break;
        }
    }

    if(i < 0){
        return -1;
    }

    for(j = 7; j >= 0; j--){
        if((id[i] & (0x80 >> j)) != 0){
            break;
        }
    }

    return 8 * i + j;
}

#define CHECK(offset, delta, size)			\
	if(delta < 0 || offset + delta > size){	\
		errno = ENOSPC;						\
		return -1;							\
	}

#define INC(offset, delta, size)			\
	CHECK(offset, delta, size);				\
	offset += delta;

#define COPY(buf, offset, src, delta,size)	\
	CHECK(offset, delta, size);				\
	memcpy(buf + offset, src, delta);		\
	offset += delta;



static int
dht_send(int sfd, const void *buf, size_t len, int flags,
		const struct sockaddr *sa, int salen){
	if(salen == 0){
		abort();
	}
	if(sa->sa_family == AF_INET){
		return sendto(sfd, buf, len, flags, sa, salen);
	}else{
		errno = EAFNOSUPPORT;
		return -1;
	}
}

static int
send_ping(int sfd, const struct sockaddr *sa, int salen, char *myid,
		  const unsigned char *tid, int tid_len){
	int size = 512;
	char buf[size];
	int i = 0, rc;
	rc = snprintf(buf + i, 512 - i, "d1:ad2:id20:");
	INC(i, rc, size);
	COPY(buf, i, myid, 20, size);
	rc = snprintf(buf + i, 512 - i, "e1:q4:ping1:t%d:", tid_len);
	INC(i, rc, size);
	COPY(buf, i, tid, tid_len, size);
	rc = snprintf(buf + i, 512 - i, "1:y1:qe");
	INC(i, rc, size);

	return dht_send(sfd, buf, i, 0, sa, salen);
}

static int
send_find_node(int sfd, const struct sockaddr *sa, int salen, char *myid,
			   const unsigned char *tid, int tid_len,
			   const unsigned char *target){
	int i = 0, size = 512, rc;
	char buf[size];
	rc = snprintf(buf + i, 512 - i, "d1:ad2:id20:");
	INC(i, rc, size);
	COPY(buf, i, myid, 20, size);
	rc = snprintf(buf + i, 512 - i, "6:target20:");
	INC(i, rc, size);
	COPY(buf, i, target, 20, size);
	rc = snprintf(buf + i, 512 - i, "e1:q9:find_node1:t%d:", tid_len);
	INC(i, rc, size);
	COPY(buf, i, tid, tid_len, size);
	rc = snprintf(buf + i, 512 - i, "1:y1:qe");
	INC(i, rc, size);
	return dht_send(sfd, buf, i, 0, sa, salen);
}

static int
send_get_peers(int sfd, const struct sockaddr *sa, int salen, char *myid,
			   unsigned char *tid, int tid_len, unsigned char *infohash){
	int i = 0, rc, size = 512;
	char buf[size];
	rc = snprintf(buf + i, size - i, "d1:ad2:id20:");
	INC(i, rc, size);
	COPY(buf, i, myid, 20, size);
	rc = snprintf(buf + i, size - i, "9:info_hash20:");
	INC(i, rc, size);
	COPY(buf, i, infohash, 20, size);
	rc = snprintf(buf + i, size - i, "e1:q9:get_peers1:t%d:", tid_len);
	INC(i, rc, size);
	COPY(buf, i, tid, tid_len, size);
	rc = snprintf(buf + i, size - i, "1:y1:qe");
	INC(i, rc, size);
	return dht_send(sfd, buf, i, 0, sa, salen);
}

#undef CHECK
#undef INC
#undef COPY


#define ERROR			0
#define REPLY			1
#define PING			2
#define FIND_NODE		3
#define GET_PEERS		4
#define ANNOUNCE_PEER	5
static int
parse_message(const unsigned char *buf, int buflen,
			  unsigned char *tid_return, int *tid_len,
			  unsigned char *id_return, unsigned char *info_hash_return,
			  unsigned char *target_return, unsigned short *port_return,
			  unsigned char *token_return, int *token_len,
			  unsigned char *nodes_return, int *nodes_len,
			  unsigned char *values_return, int *values_len){
	const unsigned char *p;

	if(buf[buflen] != '\0'){
		printf("Eek!   parse_message with unterminated buffer.\n");
		return -1;
	}

#define CHECK(ptr, len)												\
	if(((unsigned char*)ptr) + (len) > (buf) + (buflen)){			\
		printf("Truncated message.\n");								\
		return -1;													\
	}

	if(tid_return){
		p = memmem(buf, buflen, "1:t", 3);
		if(p){
			long l;
			char *q;
			l = strtol((char*)p + 3, &q, 10);
			if(q && *q == ':' && l > 0 && l < *tid_len){
				CHECK(q + 1, l);
				memcpy(tid_return, q + 1, l);
				*tid_len = l;
			}else{
				tid_len = 0;
			}
		}
	}

	if(id_return){
		p = memmem(buf, buflen, "2:id20:", 7);
		if(p){
			CHECK(p + 7, 20);
			memcpy(id_return, p + 7, 20);
		}else{
			memset(id_return, 0, 20);
		}
	}

	if(info_hash_return){
		p = memmem(buf, buflen, "9:info_hash20:", 14);
		if(p){
			CHECK(p + 14, 20);
			memcpy(info_hash_return, p + 14, 20);
		}else{
			memset(info_hash_return, 0, 20);
		}
	}

	if(port_return){
		p = memmem(buf, buflen, "porti", 5);
		if(p){
			long l;
			char *q;
			l = strtol((char*)p + 5, &q, 10);
			if(q && *q == 'e' && l > 0 && l < 0x10000){
				*port_return = l;
			}else{
				*port_return = 0;
			}
		}else{
			*port_return = 0;
		}
	}
	
	if(target_return){
		p = memmem(buf, buflen, "6:target20:", 11);
		if(p){
			CHECK(p + 11, 20);
			memcpy(target_return, p + 11, 20);
		}else{
			memset(target_return, 0, 20);
		}
	}

	if(token_return){
		p = memmem(buf, buflen, "5:token", 7);
		if(p){
			long l;
			char *q;
			l = strtol((char*)p + 7, &q, 10);
			if(q && *q == ':' && l > 0 && l < *token_len){
				CHECK(q + 1, l);
				memcpy(token_return, q + 1, l);
				*token_len = 0;
			}
		}else{
			*token_len = 0;
		}
	}

	if(nodes_len){
		p = memmem(buf, buflen, "5:nodes", 7);
		if(p){
			long l;
			char *q;
			l = strtol((char*)p + 7, &q, 10);
			if(q && *q == ':' && l > 0 && l < *nodes_len){
				CHECK(q + 1, l);
				memcpy(nodes_return, q +1, l);
				*nodes_len = l;
			}else{
				*nodes_len = 0;
			}
		}else{
			*nodes_len = 0;
		}
	}

	if(values_len){
		p = memmem(buf, buflen, "6:valuesl", 9);
		if(p){
			int i = p - buf +9, j = 0;
			while(1){
				long l;
				char *q;
				l = strtol((char*)buf + i, &q, 10);
				if(q && *q == ':' && l > 0){
					CHECK(q +1, l);
					i = q + 1 + l - (char*)buf;
                    if(l == 6){
                        if(j + l > *values_len){
                            continue;
                        }
                        memcpy((char *)values_return + j, q + 1, l);
                        j += l;
                    }else{
                        printf("Received weird value -- %d bytes.\n", (int)l);
                    }
				}else{
                    break;
                }
			}
            if(i >= buflen || buf[i] != 'e'){
                printf("Eek... unexpected end for values.\n");
            }
            if(values_len){
                *values_len = j;
            }
		}else{
			*values_len = 0;
		}
	}

#undef CHECK
	if(memmem(buf, buflen, "q:y1:r", 6)){
		return REPLY;
	}
	if(memmem(buf, buflen, "1:y1:e", 6)){
		return ERROR;
	}
	if(!memmem(buf, buflen, "1:y1:q", 6)){
		return -1;
	}
	if(memmem(buf, buflen, "1:q4:ping", 9)){
		return PING;
	}
	if(memmem(buf, buflen, "1:q9:find_node", 14)){
		return FIND_NODE;
	}
	if(memmem(buf, buflen, "1:q9:get_peers", 14)){
		return GET_PEERS;
	}
	if(memmem(buf, buflen, "1:q13:announcer_peer", 19)){
		return ANNOUNCE_PEER;
	}

	return -1;
}

static int
bucket_middle(struct bucket *b, unsigned char *id_return){
    int bit1 = lowbit(b->first);
    int bit2 = b->next ? lowbit(b->next->first) : -1;
    int bit = MAX(bit1, bit2) + 1;

    if(bit >= 160){
        return -1;
    }

    memcpy(id_return, b->first, 20);
    id_return[bit / 8] |= (0x80 >> (bit % 8));
	return 1;
}

static struct bucket*
find_bucket(unsigned const char *id){
    struct bucket *b = buckets;

    if(b = NULL){
        return NULL;
    }

    while(1){
        if(b->next == NULL){
            return b;
        }
        if(id_cmp(id, b->next->first) < 0){
            return b;
        }
        b = b->next;
    }
}

static struct node*
insert_node(struct node *node){
	struct bucket *b = find_bucket(node->id);

	if(b == NULL){
		return NULL;
	}

	node->next = b->nodes;
	b->nodes = node;
	b->count++;
	return node;
}

static struct bucket*
split_bucket(struct bucket *b){
    struct bucket *new;
    struct node *nodes;
    int rc;
    unsigned char new_id[20];

    rc = bucket_middle(b, new_id);
    if(rc < 0){
        return NULL;
    }
	
	new = calloc(1, sizeof(struct bucket));
	if(new == NULL){
		return NULL;
	}
	
	memcpy(new->first, new_id, 20);
	nodes = b->nodes;
	b->nodes = NULL;
	b->count = 0;
	new->next = b->next;
	b->next = new;
	while(nodes){
		struct node *n;
		n = nodes;
		nodes = nodes->next;
	}

    return b;
}

static struct node*
new_node(const unsigned char *id, const struct sockaddr *sa, int salen, const unsigned char *myid){
    struct bucket *b = buckets;
    struct node *n;

    if(b == NULL){
        return NULL;
    }

    if(id_cmp(id, myid) == 0){
        return NULL;
    }

    n = b->nodes;
    while(n){
        if(id_cmp(n->id, id) == 0){
            memcpy((struct sockaddr*)&n->ss, sa, salen);
            return n;
        }
        n = n->next;
    }
    
    n = calloc(1, sizeof(struct node));
    if(n == NULL){
        return NULL;
    }
    memcpy(n->id, id, 20);
    memcpy(&n->ss, sa, salen);
    n->sslen = salen;
    n->next = b->nodes;
    b->nodes = n;
    b->count++;
    return n;
}

int main(int argc, char *argv[]){
	int sfd;
	//char con[] = "d1:ad2:id20:abcdefsaij0123456789e1:q4:ping1:t2:aa1:y1:qe";
	char con[] = "d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:aa1:y1:qe";
	//char con[] = "d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:aa1:y1:qe";
	char resp[1000] = {0};
	struct sockaddr_in claddr;
	ssize_t numBytes;
	int i, temp;
	char test[] = {-61,-102,-24,-22};
	char port[] = {-56,-43}; 
	Node node;
	char result[500];
	char ping_result[512] = {0};
	char tid_return[4], id_return[20], nodes_return[512];
	int tid_len = 4, nodes_len = 512;
    struct node *n = NULL;
    struct bucket *b = NULL;
    char myid[20] = "abcdefsaij0123456789";

	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sfd==-1){
		Error("scoket");
	}

	set_nonblocking(sfd, 1);

	bzero(&claddr, sizeof(struct sockaddr_in));
	claddr.sin_family = AF_INET;
	claddr.sin_addr.s_addr = inet_addr("91.121.59.153");
	claddr.sin_port = htons(6881);
/*	
	if(sendto(sfd, con, strlen(con), 0, (struct sockaddr*)&claddr, sizeof(claddr))<0){
		Error("sendto");
	}
*/	
	while(1){
	struct timeval tv;
	fd_set readfds;
	tv.tv_sec = 5;
	tv.tv_usec = random() % 1000000;
	int rc;

	if(send_find_node(sfd, (struct sockaddr*)&claddr, sizeof(claddr), myid,  "aa", 2, "abcdefghij01234567899") < 0){
		Error("sendto");
	}
	
	FD_ZERO(&readfds);
	FD_SET(sfd, &readfds);
	rc = select(sfd, &readfds, NULL, NULL, &tv);
	if(rc < 0){
		perror("select");
		sleep(1);
		continue;
	}
	
	numBytes = recvfrom(sfd, resp, 1000, 0, NULL, NULL);
	if(numBytes==-1){
		//Error("recvfrom");
		continue;
	}
	
	parse_message(resp, (int)numBytes,
				  tid_return, &tid_len,
				  id_return, NULL, NULL, NULL,
				  NULL, NULL, nodes_return, &nodes_len,
				  NULL, NULL
				  );
	printf("tid_return = %s, tid_len = %d\n", tid_return, tid_len);
    printf("id_return = ");print_hex(stdout, id_return, 20);printf("\n");
    printf("nodes_return = ");print_hex(stdout, nodes_return, nodes_len);printf(", nodes_len = %d\n", nodes_len);

    buckets = calloc(sizeof(struct bucket), 1);
    if(buckets == NULL){
        exit(EXIT_FAILURE);
    }
    n = calloc(sizeof(struct node), 1);
    if(n == NULL){
        exit(EXIT_FAILURE);
    }
    memcpy(n->id, myid, 20);
    n->next = NULL;
    buckets->nodes = n;
    buckets->count = 1;
    for(temp = 0; temp < nodes_len / 26; temp++){
        unsigned char *ni = nodes_return + temp * 26;
        struct sockaddr_in sin;
        if(id_cmp(ni, myid) == 0){
            continue;
        }
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        memcpy(&sin.sin_addr, ni + 20, 4);
        memcpy(&sin.sin_port, ni + 24, 2);
        new_node(ni, (struct sockaddr*)&sin, sizeof(sin), myid);
    }
    printf("buckets->count = %d\n", buckets->count);
    
    n = buckets->nodes;
    while(n){
        printf("id = ");print_hex(stdout, n->id, 20);printf("\n");
        n = n->next;
    }
	
	if(!split_bucket(buckets)){
		exit(EXIT_FAILURE);
	}
	if(!split_bucket(buckets)){
		exit(EXIT_FAILURE);
	}
	if(!split_bucket(buckets)){
		exit(EXIT_FAILURE);
	}
	b = buckets;
	while(b){
		printf("first = ");print_hex(stdout, b->first, 20);printf("\n");
		b = b->next;
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

	
	
	
	}
	exit(EXIT_SUCCESS);
}
