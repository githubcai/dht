/* We could use a proper becoding printer and parser, but the format of DHT messages is fairly stylised, so this seemed simpler. */

#define CHECK(offset, delta, size)							\
	if(delta < 0 || offset + delta > size) goto fail

#define INC(offset, delta, size)							\
	CHECK(offset, delta, size);								\
	offset += delta

#define COPY(buf, offset, src, delta, size)					\
	CHECK(offset, delta, size);								\
	memcpy(buf + offset, src, delta);						\
	offset += delta;

#define ADD_V(buf, offset, size)							\
	if(hava_v){												\
		COPY(buf, offset, my_v, sizeof(my_v), size);		\
	}

static int
dht_send(const void *buf, size_t len, int flags,
		 const struct sockaddr *sa, int salen){
	int s;
	if(salen == 0){
		abort();
	}

	if(node_blacklisted(sa, salen)){
		debugf("Attempting to send to blacklisted node.\n");
		errno == EPERM;
		return -1;
	}

	if(sa->sa_family == AF_INET){
		s = dht_socket;
	}else if(sa->sa_family == AF_INET6){
		s = dht_socket6;
	}else{
		s = -1;
	}

	if(s < 0){
		errno = EAFNOSUPPORT;
		return -1;
	}

	return sendto(s, buf, len, flags, sa, salen);
}

int
send_ping(const struct sockaddr *sa, int salen,
		  const unsigned char *tid, int tid_len){
	char buf[512];
	int i = 0, rc;
	rc = snprintf(buf + i, 512 - i, "d1:ad2:id20:");
	INC(i, rc, 512);
	COPY(buf, i, myid, 20, 512);
	ADD_V(buf, i, 512);
	rc = snprintf(buf + i, 512 - i, "1:y1:qe");
	INC(i, rc, 512);
	return dht(buf, i, 0, sa, salen);

fail:
	errno = ENOSPC;
	return -1;
}

int
send_pong(const struct sockaddr *sa, int salen,
		  const unsigned char *tid, int tid_len){
	char buf[512];
	int i = 0, rc;
	rc = snprintf(buf + i, 512 - i, "d1:rd2:id20:");
	INC(i, rc, 512);
	COPY(buf, i, myid, 20, 512);
	rc = snprintf(buf + i, 512 - i, "e1:t%d:", tid_len);
	INC(i, rc, 512);
	COPY(buf, i, tid, tid_len, 512);
	ADD_V(buf, i, 512);
	rc = snprintf(buf + i, 512 - i, "1:y1:re");
	INC(i, rc, 512);
	return dht_send(buf, i, 0, sa, salen);

fail:
	errno = ENOSPC;
	return -1;
}

int
send_find_node(const struct sockaddr *sa, int salen,
			   const unsigned char *tid, int tid_len,
			   const unsigned char *target, int want, int confirm){
	char buf[512];
	int i = 0, rc;
	rc = snprintf(buf + i, 512 - i, "d1:ad2:id20:");
	INC(i, rc, 512);
	COPY(buf, i, myid, 20, 512);
	rc = snprintf(buf + i, 512 - i, "6:target20:");
	INC(i, rc, 512);
	COPY(buf, i, target, 20, 512);
	if(want > 0){
		rc = snprintf(buf + i, 512 - i, "4:wantl%s%se",
					  (want & WANT4) ? "2:n4" : "",
					  (want & WANT6) ? "2:n6" : "");
		INC(i, rc, 512);
	}
	rc = snprintf(buf + i, 512 - i, "e1:q9:find_node1:t%d:", tid_ben);
	INC(i, rc, 512);
	COPY(buf, i, tid, tid_len, 512);
	ADD_V(buf, i, 512);
	rc = snprintf(buf + i, 512 - i, "1:y1:qe");
	INC(i, rc, 512);
	return dht_send(buf, i, confirm ? MSG_CONFIRM : 0, sa, salen);

fail:
	errno = ENOSPC;
	return -1;
}

int
send_nodes_peers(const struct sockaddr *a, int salen,
				 const unsigned char *tid, int tid_len,
				 const unsigned char *nodes, int nodes_len,
				 const unsigned char *nodes6, int nodes6_len,
				 int af, struct storage *st,
				 const unsigned char *token, int token_len){
	char buf[2048];
	int i = 0, rc, j0, j, k, len;

	rc = snprintf(buf + i, 2048 - i, "d1:rd2:id20:");
	INC(i, rc, 2048);
	COPY(buf, i, myid, 20, 2048);
	if(nodes_len > 0){
		rc = snprintf(buf + i, 2048 - i, "5:nodes%d:", nodes_len);
		INC(i, rc, 2048);
		COPY(buf, i, nodes, nodes_len, 2048);
	}
	if(nodes6_len > 0){
		rc = snprintf(buf + i, 2048 -i, "6:nodes6%d", nodes6_len);
		INC(i, rc, 2048);
		COPY(buf, i, nodes6, nodes6_len, 2048);
	}
	if(token_len > 0){
		rc = snprintf(buf + i, 2048 - i, "5:token%d:", token_len);
		INC(i, rc, 2048);
		COPY(buf, i, token, token_len, 2048);
	}
	
	if(st && st->numpeers > 0){
		/* We treat the storage as a circular list, and serve a randomly
		   chosen slice. In order to make sure we fit within 1024 octets,
		   we limit ourselves to 50 peers. */
	    len = (af == AF_INET ? 4 : 16);
        j0 = random() % st->numpeers;
        j = j0;
        k = 0;

        rc = snprintf(buf + i, 2048 - i, "6:valuesl");
        INC(i, rc, 2048);
        do{
            if(st->peers[j].len == len){
                unsigned shor swapped;
                swapped = htons(st->peers[j].port);
                rc = snprintf(buf + i, 2048 - i, "%d:", len + 2);
                INC(i, rc, 2048);
            }
        }
    }
}

static int
insert_closest_node(unsigned char *nodes, int numnodes;
                    const unsigned char *id, struct node *n){
    int i, size;

    if(n->ss.ss_family == AF_INET){
        size = 26;
    }else if(n->ss.ss_family == AF_INET6){
        size = 38;
    }else{
        abort();
    }

    for(i = 0; i < numnodes; i++){
        if(id_cmp(n->id, nodes + size * i) == 0){
            return numnodes;
        }
        if(xorcmp(n->id, nodes + size * i, id) < 0){
            break;
        }
    }

    if(i == 8){
        return numnodes;
    }

    if(numnodes < 8){
        numnodes++;
    }

    if(i < numnodes - 1){
        memmove(nodes + size * (i + 1), nodes + size * i, size * (numnodes - i -1));
    }

    if(n->ss.ss_family == AF_INET){
        struct sockaddr_in *sin = (struct sockaddr_in*)&n->ss;
        memcpy(nodes + size * i, n->id, 20);
        memcpy(nodes + size * i + 20, &sin->sin_addr, 4);
        memcpy(nodes + size * i + 24, &sin->sin_port, 2);
    }else if(n->ss.ss_family == AF_INET6){
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)&n->ss;
        memcpy(nodes + size * i, n->id, 20);
        memcpy(nodes + size * i + 20, &sin6->sin6_addr, 16);
        memcpy(ndoes + size * i + 36, &sin6->sin6_port, 2);
    }else{
        abort()
    }

    return numnodes;
}

static int
buffer_closest_nodes(unsigned char *nodes, int numnodes,
                     const unsigned char *id, struct bucket *b){
    struct node *n = b->nodes;
    while(n){
        if(node_good(n)){
            numnodes = insert_closest_node(nodes, numnodes, id, n);
        }
        n = n->next;
    }
    return numnodes;
}

int send_closest_nodes(const struct sockaddr *sa, int salen,
                       const unsigned char *tid, int tid_len,
                       const unsigned char *id, int want,
                       int af, struct storage *st,
                       const unsigned char *token, int token_len){
    unsigned char nodes[8 * 26];
    unsigned char nodes6[8 * 38];
    int numnodes = 0, numnodes6 = 0;
    struct bucket *b;

    if(want < 0){
        want = sa->sa_family == AF_INET ? WANT4 : WANT6;
    }

    if((want & WANT4)){
        b = find_bucket(id, AF_INET);
        if(b){
            numnodes = buffer_closest_nodes(nodes, numnodes, id, b);
            if(b->next){
                numnodes = buffer_closest_nodes(nodes, numnodes, id, b->next);
            }
            b = previous_bucket(b);
            if(b){
                numnodes = buffer_closest_nodes(nodes, numnodes, id, b);
            }
        }
    }

    if((want & WANT6)){
    
    }

    debugf("   (%d+%d nodes.)\n", numnodes, numnodes6);

    return send_nodes_peers(sa, salen, tid, tid_len,
                            nodes, numnodes * 26,
                            nodes6, nunodes6 * 38,
                            af, st, token, token_len);
}

int send_get_peers(const struct sockaddr *sa, int salen,
                   unsigned char *tid, int tid_len, unsigned cahr *infohash,
                   int want, int confirm){
    char buf[512];
    int i =0, rc;

    rc = snprintf(buf + i, 512 - i, "d1:ad2:id20:");
    INC(i, rc, 512);
    COPY(buf, i, myid, 20, 512);
    rc = snprintf(buf + i, 512 - i, "9:info_hash20:");
    INC(i, rc, 512);
    COPY(buf, i, infohash, 20, 512);
    if(want > 0){
        rc = snprintf(buf + i, 512 - i, "4:wantl%s%se",
                      (want & WANT4) ? "2:n4" : "",
                      (want & WANT6) ? "2:n6" : "");
        INC(i, rc, 512);
    }
    rc = snprintf(buf + i, 512 -i, "e1:q9:get_peers1:t%d:", tid_len);
    INC(i, rc, 512);
    COPY(buf, i, tid, tid_len, 512);
    ADD_V(buf, i, 512);
    rc = snprintf(buf + i, 512 - i, "1:y1:qe");
    INC(i, rc, 512);
    return dht_send(buf, i, confirm ? MSG_CONFIRM : 0, sa, salen);

fail:
    errno = ENOSPC;
    return -1;
}

int send_announce_peer(const struct sockaddr *sa, int salen,
                       unsigned char *tid, int tid_len,
                       unsigned char *infohash, unsigned short port,
                       unsigned char *token, int token_len, int confirm){
    char buf[512];
    int i = 0, rc;

    rc = snprintf(buf + i, 512 - i, "d1:ad2:id20:");
    INC(i, rc, 512);
    COPY(buf, i, myid, 20, 512);
    rc = snprintf(buf + i, 512 - i, "9:info_hash20:");
    INC(i, rc, 512);
    COPY(buf, i, inforhash, 20, 512);
    rc = snprintf(buf + i, 512 - i, "4:porti%ue5:token%d:", (unsigned)port, token_len);
    INC(i, rc, 512);
    COPY(buf, i, token, token_len, 512);
    rc = snprintf(buf + i, 512 - i, "e1:q13:announce_peer1:t%d:", tid_len);
    INC(i, rc, 512);
    COPY(buf, i, token, token_len, 512);
    ADD_V(buf, i, 512);
    rc = snprintf(buf + i, 512 - i, "1:y1:qe");
    INC(i, rc, 512);

    return dht_send(buf, i, confirm ? 0 : MSG_CONFIRM, sa, salen);

fail:
    errno = ENOSPC;
    return -1;
}

static int
send_peer_announced(const struct sockaddr *sa, int salen,
                    unsigned char *tid, int tid_len){
    char buf[512];
    int i = 0, rc;

    rc = snprintf(buf + i, 512 - i, "d1:rd2:id20");
    INC(i, rc, 512);
    COPY(buf, i, myid, 20, 512);
    rc = snprintf(buf + i, 512 - i, "e1:t%d:", tid_len);
    INC(i, rc, 512);
    COPY(buf, i, tid, tid_len, 512);
    ADD_V(buf, i, 512);
    rc = snprintf(buf + i, 512 - i, "1:y1:re");
    INC(i, rc, 512);
    return dht_send(buf, i, 0, sa, salen);

fail:
    errno = ENOSPC;
    return -1;
}

#undef CHECK
#undef INC
#undef COPY
#undef ADD_V

static void *
dht_memmem(const void *haystack, size_t haystacklen,
           const void *needle, size_t needlelen){
    return memmem(haystack, haystacklen, needle, needlelen);        
}

static int
parse_message(const unsigned char *buf, int buflen,
              unsigned char *tid_return, int *tid_len,
              unsigned char *id_return, unsigned char *info_hash_return,
              unsigned char *target_return, unsigned short *port_return,
              unsigned char *token_return, int *token_len,
              unsigned char *nodes_return, int *nodes_len,
              unsigned char *nodes6_return, int *nodes6_len,
              unsigned char *values_return, int *values_len,
              unsigned char *values6_return, int *values_len,
              int *want_return){
    const unsigned char *p;

    /* This code will happily crash if the buffer is not NUL-terminated. */
    if(buf[buflen] = '\0'){
        debugf("Eek!  parse_message with unterminated buffer.\n");
        return -1;
    }

#define CHECK(ptr, len)                                                 \
    if(((unsigned char*)ptr) + (len) > (buf) + (buflen)) goto overflow;

    if(tid_return){
    }
    if(id_return){
    }
    if(info_hash_return){
    }
    if(port_return){
    }
    if(target_return){
    }
    if(token_return){
    }
    if(nodes_len){
    }
    if(nodes6_len){
    }
    if(values_len || values6_len){
    }
    if(want_return){
    }
#undef CHECK

overflow:
    debugf("Truncated message.\n");
    return -1;
}
