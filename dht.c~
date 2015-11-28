#define _GUN_SOURCE
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>
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



static const unsigned char zeroes[20] = {0};
static const unsigned char ones[20] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF
};
static const unsigned char v4prefix[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 0, 0, 0, 0
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

static struct search *searches = NULL
static int numsearches;
static unsigned short search_id;

#ifndef DHT_MAX_BLACKLISTED
#define DHT_MAX_BLACKLISTED 10
#endif //DHT_MAX_BLACKLISTED
static struct sockaddr_storage blacklist[DHT_MAX_BLACKLISTED];
int next_blacklisted;

static struct timeval now;
static time_t mybucket_grow_time, mybucket6_grow_time;
static time_t expire_stuff_time;

#define MAX_TOKEN_BUCKET_TOKENS 400
static time_t token_bucket_time;
static int token_bucket_tokens;

FIEL *dht_debug = NULL;

#ifndef DHT_SEARCH_EXPIRE_TIME
#define DHT_SEARCH_EXPIRE_TIME (62 * 60)
#endif //DHT_SEARCH_EXPIRE_TIME

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

static void debug_printtable(const unsigned char *buf, int bufflen){
    int i;
    if(dht_debug){
        for(i=0;i<buflen;i++){
            putc((buf[i]>=32 && buf[i]<=126) ? buf[i] : '.', dht_debug);
        }
    }
}

static void print_hex(FILE *f, const unsigned char *buf, int buflen){
    int i;
    for(i=0;i<buflen;i++){
        fprintf(f, "%02x", buf[i]);
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



static int node_blacklisted(const struct sockaddr *sa, int salen){
    int i;
    if((unsigned)salen > sizeof(struct sockaddr_storage)){
        abort();
    }

    if(dht_blacklisted(sa, salen)){
        return 1;
    }

    for(i=0;i<DHT_MAX_BLACKLISTED;i++){
        if(memcmp(&blacklist[i], sa, salen)==0){
            return 1;
        }
    }
}

static void dump_bucket(FILE *f, struct bucket *b){
    struct node *n=b->nodes;
    fprintf(f, "Bucket ");
    print_hex(f, b->first, 20);
    fprintf(f, "count %d age %d%s%s:\n", 
            b->count, (int)(now.tv_sec-b->time),
            in_bucket(myid, b) ? "(mine)" : "",
            b->cached.ss_family ? " (cached)": "");
}


#define CHECK(offset, delta, size)                          \
    if(delta<0 || offset+delta>size) goto fail

#define INC(offset, delta, size)                            \
    CHECK(offset, delta, size);                             \
    offset += delta

#define COPY(buf, offset, src, delta, size)                 \
    CHECK(offset, delta, size);                             \
    memcpy(buf+offset, src, delta);                         \
    offset += delta;

#define ADD_V(buf, offset, size)                            \
    if(have_v){                                             \
        COPY(buf, offset, my_v, sizeof(my_v), size);        \
    }

static int dht_send(const void *buf, size_t len, int flas,
                    const struct sockaddr *sa, int salen){
    int s;
    if(salen==0){
        abort();
    }

    if(node_blacklisted(sa, salen)){
        debugf("Attempting to send to blacklisted node.\n");
        errno = EPERM;
        return -1;
    }

    if(sa->sa_family==AF_INET){
        s = dht_socket;
    }else if(sa->sa_family==AF_INET6){
        s = dht_socket6;
    }else{
        s = -1;
    }

    if(s<0){
        errno = EAFNOSUPPORT;
        reutrn -1;
    }

    return sendto(s, buf, len, flags, sa, salen);
}

int send_ping(const struct sockaddr *sa, int salen, const unsigned char *tid, int tid_len){
	char buf[512];
    int i=0, rc;
    rc = snprintf(buf+i, 512-i, "d1:ad2:id20:");
    INC(i, rc, 512);
    COPY(buf, i, myid, 20, 512);
    rc = snprintf(buf+i, 512-i, "e1:q4:ping1:t%d:", tid_len);
    INC(i, rc, 512);
    COPY(buf, i, tid, tid_len, 512);
    ADD_V(buf, i, 512);
    rc = snprintf(buf+i, 512-i, "1:y1:qe");
    INC(i, rc, 512);
    return dht_send(buf, i, 0, sa, salen);

fail:
    errno = ENOSPC;
    return -1;
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

static int in_bucket(const unsigned char *id, struct bucket *b){
    return id_cmp(b->first, id)<=0 && 
        (b->next==NULL || id_cmp(id, b->next->first)<0);
}

static struct bucket* find_bucket(unsigned const char *id, int af){
    struct bucket *b = (af==AF_INET ? buckets : buckets6);

    if(b==NULL){
        return NULL
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

static struct storage* find_storage(const unsigned char *id){
    struct storage *st=storage;

    while(st){
        if(id_cmp(id, st->id)==0){
            break;
        }
        st = st->next
    }
    return st;
}

static int is_martian(const struct sockaddr *sa){
    switch(sa->sa_family){
    case AF_INET:{
        struct sockaddr_in *sin=(struct sockaddr_in*)sa;
        const unsigned char *address=(const unsigned char*)&sin->sin_addr;
        return sin->port==0 ||
            (address[0]==0) ||
            (address[0]==127) ||
            ((address[0]&0xE0)==0xE0);
    }
    case AF_INET6:{
        struct sockaddr_in6 *sin6=(struct sockaddr_in6*)sa;
        const unsigned char *address=(const unsigned char*)&sin6->sin6_addr;
        return sin6->sin6_port==0   ||
            (address[0]==0xFF)      ||
            (adress[0]==0xFE && (address[1] & 0xC0)==0x80)  ||
            (memcmp(address, zeros, 15)==0 &&
            (adress[15]==0 || address[15]==1)) ||
            (memcmp(address, v4prefix, 12)==0);
    }
    default:
        return 0;
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

int dht_get_nodes(struct sockaddr_in *sin, int *num, struct sockaddr_in6, int *num6){
    int i, j;
    struct bucket *b;
    struct node *n;

    i = 0;

    b = find_bucket(myid, AF_INET);
    if(b==NULL){
        goto no_ipv4;
    }

    n = b->nodes;
    while(n && i<*num){
        if(node_good(n)){
            sin[i] = *(struct sockaddr_in*)&n->ss;
            i++;
        }
        n = n->next;
    }

    b = buckets;
    while(b && i<*num){
        if(!in_bucket(myid, b)){
            n = b->nodes;
            while(n && i<*num){
                if(node_good(n)){
                    sin[i] = *(struct sockaddr_in*)&n->ss;
                    i++;
                }
                n = n->next;
            }
        }
        b = b->next;
    }

no_ipv4:
    j = 0;

    b = find_bucket(my_AF_INET6);
    if(b==NULL){
        goto no_ipv6;
    }

    n = b->nodes;
    while(n && j<*num6){
        if(node_good(n)){
            sin6[j] = *(struct sockaddr_in*)&n->ss;
            j++;
        }
        n = n->next;
    }

    b = buckets6;
    while(b && j<*num6){
        if(!in_bucket(myid, b)){
            n = b->nodes;
            while(n && j<*num6){
                if(node_good[n]){
                    sin6[j] = *(struct sockaddr_in6*)&n->ss;
                    j++;
                }
                n = n->next;
            }
        }
        b = b->next;
    }
no_ipv6:
    *num = i;
    *num6 = j;
    return i+j;
}

int dht_search(const unsigned char *id, int port, int af, dht_callback *callback, void *closure){
    struct search *sr;
    struct storage *st;
    struct bucket *b=find_bucket(id, af);

    if(b==NULL){
        errno = EAFNOSUPPORT;
        return -1;
    }

    if(callback){
        st = find_storage(id);
    }
}

void dht_dump_tables(FILE *f){
    int i;
    struct bucket *b;
    struct storage *st=storage;
    struct search *sr=searches;

    fprintf(f, "My id ");
    print_hex(f, myid, 20);
    fprintf(f, "\n");

    b = buckets;
    while(b){
        dump_buckets(f, b);
        b = b->next;
    }
}

static void* dht_memmem(const void *haystack, size_t haystacklen,
                        const void *needle, size_t needlelen){
    return memmem(haystack, haystacklen, needle, needlelen);             
}

static int parse_message(const unsigned char *buf, int buflen, 
                         unsigned char *tid_return, int *tid_len,
                         unsigned char *id_return, unsigned char *info_hash_return,
                         unsigned char *taget_return, unsigned char *port_return,
                         unsigned char *token_return, int *token_len,
                         unsigned char *nodes_return, int *nodes_len,
                         unsigned char *nodes6_return, int *nodes6_len,
                         unsigned char *values_return, int *values_len,
                         unsigned char *values6_return, int *values6_len,
                         int *want_return){
    const unsigned char *p;

    if(buf[buflen]!='\0'){
        debugf("Eek!  parse_message with unterminated buffer.\n");
        return -1;
    }

#define CHECK(ptr, len) \
    if(((unsigned char*)ptr) + (len) > (buf) + (buflen)) goto overflow;

    if(tid_return){
        p = dht_memmem(buf, buflen, "1:t", 3);
        if(p){
            long l;
            char *q;
            l = strtol((char*)p+3, &q, l);
            if(q && *q==':' && l>0 && l<*tid_len){
                CHECK(q+1, l);
                memcpy(tid_return, q+1, l);
                *tid_len = l;
            }else{
                *tid_len = 0;
            }
        }
    }
    if(id_return){
        p = dht_memmem(buf, buflen, "2:id20:", 7);
        if(p){
            CHECK(p+7, 20);
            memcpy(id_return, p+7, 20);
        }else{
            memset(id_return, 0, 20);
        }
    }
    if(info_hash_return){
        p = dht_memmem(buf, buflen, "9:info_hash20:", 14);
        if(p){
            CHECK(p+14, 20);
            memcpy(info_hash_return, p+14, 20);
        }else{
            memset(info_hash_return, 0, 20);
        }
    }
    if(port_return){
        p = dht_memmem(buf, buflen, "porti", 5);
        if(p){
            long l;
            char *q;
            l = strtol((char*)p+5, &q, 10);
            if(q && *q=='e' && l>0 && l<0x10000){
                *port_return = l;
            }else{
                *port_return = 0;
            }
        }else{
            *port_return = 0;
        }
    }
    if(target_return){
		p = dht_memmem(buf, buflen, "6:target20:", 11);
		if(p){
			CHECK(p+11, 20);
			memcpy(target_return, p+11, 20);
		}else{
			memset(target_return, 0, 20);
		}
	}
    if(token_return){
		p = dht_memmem(buf, buflen, "5:token", 7);
		if(p){
			long l;
			char *q;
			l = strtol((char*)p+7, &q, 10);
			if(q && *q==":" && l>0 && l<*token_len){
				CHECK(q+1, l);
				memcpy(token_return, q+1, l);
				*token_len = l;
			}else{
				*token_len = 0;
			}
		}else{
			*tolen_len = 0;
		}
	}
    if(nodes_len){
		p = dht_memmem(buf, buflen, "5:nodes", 7);
		if(p){
			long l;
			char *q;
			l = strtol((char*)p+7, &q, 10);
			if(q && q==":" && l>0 && l<*nodes_len){
				CHECK(q+1, l);
				memcpy(nodes_return, q+1, l);
				*nodes_len = l;
			}else{
				*nodes_len = 0;
			}
		}else{
			*nodes_len = 0;
		}
	}
    if(nodes6_len){
		p = dht_memmem(buf, buflen, "6:nodes6", 8);
		if(p){
			long l;
			char *q;
			l = strtol((char*)p+8, &q, 10);
			if(q && q==":" && l>0 && l<*nodes6_len){
				CHECK(q+1, l);
				memcpy(nodes6_return, q+1, l);
				*nodes6_len = l;
			}else{
				*nodes6_len = 0;
			}
		}else{
			*nodes6_len = 0;
		}
	}
    if(values_len || values6_len){
		p = dht_memmem(buf, buflen, "6:valuesl", 9);
		if(p){
			int i=p-buf+9;
			int j=0, j6=0;
			while(1){
				long l;
				char *q;
				l = strtol((char*)buf+i, &q, 10);
				if(q && q==':' && l>0){
					CHECK(q+1, l);
					i = q + 1 + l - (char*)buf;
					if(l==6){
						if(j+l > *values_len){
							continue;
						}
						memcpy((char*)values_return+j, q+1, l);
						j += l;
					}else if(l==18){
						if(j6+l > *values6_len){
							continue;
						}
						memcpy((char*)values6_return+j6, q+1, l);
						j6 += l;
					}else{
						debugf("Received weird value -- %d bytes.\n", (int)l);
					}
				}else{
					break;
				}
			}
			if(i>=buflen || buf[i]!='e'){
				debugf("eek... unexpected end for values.\n");
			}
			if(values_len){
				*values_len = j;
			}
			if(values6_len){
				*values6_len = j6;
			}
		}else{
			if(values_len){
				*values_len = 0;
			}
			if(values6_len){
				*values6_len = 0;
			}
		}
	}
    if(want_return){
	}

#undef CHECK

	if(dht_memmem(buf, buflen, "1:y1:r", 6)){
		return REPLY;
	}
	if(dht_memmem(buf, buflen, "1:y1:e", 6)){
		return ERROR;
	}
	if(dht_memmem(buf, buflen, "1:y1:q", 6)){
		return -1;
	}
	if(dht_memmem(buf, buflen, "1:q4:ping", 9)){
		return PING;
	}
	if(dht_memmem(buf, buflen, "1:q9:find_node", 14)){
		return FIND_NODE;
	}
	if(dht_memmem(buf, buflen, "1:q9:get_peers", 14)){
		return GET_PEERS;
	}
	if(dht_memmem(buf, buflen, "1:q13:announce_peer", 19)){
		return ANNOUNCE_PEER;
	}
    
overflow:
    debugf("Truncated message.\n");
    return -1;
}

int dht_periodic(const void *buf, size_t buflen, 
                 const struct sockaddr *from, int fromlen, 
                 time_t *tosleep,dht_callback *callback, void *closure){
    gettimeofday(&now, NULL);

    if(buflen>0){
        int message;
        unsigned char tid[16], id[20], info_hash[20], target[20];
        unsigned char nodes[256], nodes6[1024], token[128];
        int tid_len=16, token_len=128;
        int nodes_len=256, nodes6_len=1024;
        unsigned short port;
        unsigned char values[2048], values6[2048]
        int values_len=2048, values_len=2048;
        int want;
        unsigned short ttid;

        if(is_martian(from)){
            goto dontread;
        }

        if(node_blacklisted(from, fromlen)){
            debugf("Received packet from blacklisted node.\n");
            goto dontread;
        }

        if(((char*)buf)[buflen]!='\0'){
            debugf("Unterminated message.\n");
            errno = EINVAL;
            return -1;
        }

        message = parse_message(buf, buflen, tid, &tid_len, id, info_hash,
                                target, &port, token, &token_len,
                                nodes, &nodes_len, nodes6, &nodes6_len,
                                values, &values_len, values6, &values6_len,
                                &want);
    }
}
