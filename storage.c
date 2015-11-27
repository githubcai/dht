/* A struct storage stores all the stored peer address for a given infohash. */
static struct storage*
find_storage(const unsigned char *id){
	struct storage *st = storage;

	while(st){
		if(id_cmp(id, st->id) == 0){
			break;
		}
		st = st->next;
	}

	return st;
}

static int
storage_store(const unsigned char *id,
			  const struct sockaddr *sa, unsigned short port){
	int i, len;
	struct storage *st;
	unsigned char *ip;

	if(sa->sa_family == AF_INET){
		struct sockaddr_in *sin = (struct sockaddr_in*)sa;
		ip = (unsigned char*)&sin->sin_addr;
		len = 4;
	}else if(sa->sa_family == AF_INET6){
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)sa;
		ip = (unsigned char*)&sin6->sin6_addr;
		len = 16;
	}else{
		return -1;
	}

	st = find_storage(id);

	if(st == NULL){
		if(numstorage >= DHT_MAX_HASHES){
			return -1;
		}
		st = calloc(1, sizeof(struct storage));
		if(st == NULL){
			return -1;
		}
		memcpy(st->id, id, 20);
		st->next = storage;
		storage = st;
		numstorage++;
	}

	for(i = 0; i < st->numpeers; i++){
		if(st->peers[i].port == port && st->peers[i].len == len &&
			memcmp(st->peers[i].ip, ip, len) == 0){
			break;
		}
	}

	if(i < st->numpeers){
		st->peers[i].time = now.tv_sec;
		return 0;
	}else{
		struct peer *p;
		if(i >= st->maxpeers){
			/* Need to expand the array. */
			struct peer *new_peers;
			int n;
			if(st->maxpeers >= DHT_MAX_PEERS){
				return 0;
			}
			n = st->maxpeers == 0 ? 2 : 2 * st->maxpeers;
			n = MIN(n, DHT_MAX_PEERS);
			new_peers = reaclloc(st->peers, n * sizeof(struct peer););
			if(new_peers == NULL){
				return -1;
			}
			st->peers = new_peers;
			st->maxpeers = n;
		}
		p = &st->peers[st->numpeers++];
		p->time = now.tv_sec;
		p->len = len;
		memcpy(p->ip, ip, len);
		p->port = port;
		return 1;
	}
}

static int
expire_storage(void){
	struct storage *st = storage, *previous = NULL;
	while(st){
		int i =0;
		while(i < st->numpeers){
			if(st->peers[i].time < now.tv_sec - 32 * 60){
				if(i != st->numpeers - 1){
					st->peer[i] = st->peers[st->numpeers - 1];
				}
				st->numpeers--;
			}else{
				i++;
			}
		}

		if(st->numpeers == 0){
			free(st->peers);
			if(previous){
				previous->next = st->next;
			}else{
				storage = st->next;
			}
			free(st);
			if(previous){
				st = previous->next;
			}else{
				st = storage;
			}
			numstorage--;
			if(numstorage < 0){
				debugf("Eek... numstorage became negative.\n");
				numstorage = 0;
			}
		}else{
			previous = st;
			st = st->next;
		}
	}

	return 1;
}
