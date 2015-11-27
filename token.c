static int
rotate_secrets(void){
	int rc;
	rotate_secrets_time = now.tv_sec + 900 + random() % 1800;

	memcpy(oldsecret, secret, sizeof(secret));
	rc = dht_random_bytes(secret, sizeof(secret));

	if(rc < 0){
		return -1;
	}

	return 1;
}

#ifdef TOKEN_SIZE
#define TOKEN_SIZE 8
#endif // TOKEN_SIZE

static void
make_token(const struct sockaddr *sa, int old, unsigned char *token_return){
	void *ip;
	int iplen;
	unsigned short port;

	if(sa->sa_family == AF_INET){
		struct sockaddr_in *sin = (struct sockaddr_in*)sa;
		ip = &sin->sin_addr;
		iplen = 4;
		port = htons(sin->sin_port);
	}else if(sa->sa_family == AF_INET6){
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)sa;
		ip = &sin6->sin6_addr;
		iplen = 16;
		port = htons(sin6->sin6_port);
	}else{
		abort();
	}

	dht_hash(token_return, TOKEN_SIZE,
			 old ? oldsecret : secret, sizeof(secret),
			 ip, iplen, (unsigned char*)&port, 2);
}

static int
token_match(const unsigned char *token, int token_len,
			const struct sockaddr *sa){
	unsigned char t[TOKEN_SIZE];
	if(token_len != TOKEN_SIZE){
		return 0;
	}
	make_token(sa, 0, t);
	if(memcmp(t, token, TOKEN_SIZE) == 0){
		return 1;
	}
	make_token(sa, 1, t);
	if(memcmp(t, token, TOKEN_SIZE) == 0){
		return 1;
	}
	return 0
}
