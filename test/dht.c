#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
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
					CHEKC(q +1, l);
					i = q + 1 + l - (char*)buf;
				}
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
	char tid_return[4];
	int tid_len = 4;

	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sfd==-1){
		Error("scoket");
	}

	bzero(&claddr, sizeof(struct sockaddr_in));
	claddr.sin_family = AF_INET;
	claddr.sin_addr.s_addr = inet_addr("91.121.59.153");
	claddr.sin_port = htons(6881);
/*	
	if(sendto(sfd, con, strlen(con), 0, (struct sockaddr*)&claddr, sizeof(claddr))<0){
		Error("sendto");
	}
*/	
	if(send_find_node(sfd, (struct sockaddr*)&claddr, sizeof(claddr), "abcdefghij01234567899", "aa", 2, "abcdefghij01234567899") < 0){
		Error("sendto");
	}
	

	numBytes = recvfrom(sfd, resp, 1000, 0, NULL, NULL);
	if(numBytes==-1){
		Error("recvfrom");
	}
	parse_message(resp, (int)numBytes,
				  tid_return, &tid_len,
				  NULL, NULL, NULL, NULL,
				  NULL, NULL, NULL, NULL,
				  NULL, NULL
				  );
	printf("tid_return = %s, tid_len = %d\n", tid_return, tid_len);

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
