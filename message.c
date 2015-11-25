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
		   chosen slice. In orger to make sure we fit within 1024 octets,
		   we limit ourselves to 50 peers. */
	}
}
