/* While a search is in progress, we don't necessarily keep the nodes being
   walked in the main bucket table. A search in progress is identified by
   a unique transaction id, a short (and hence small enough to fit in the
   transaction id of the protocol packets). */

static struct search*
find_search(unsigned short tid, int af){
    struct search *sr = searches;
    while(sr){
        if(sr->tid == tid && sr->af == af){
            return sr;
        }
        sr = sr->next;
    }
    return NULL;
}

/* A search contains a list of nodes, sorted by decreasing distance to the 
   target. We just got a new candidate, insert it at the right spot or discard it.*/
static int
insert_search_node(unsigned char *id,
                   const struct sockaddr *sa, int salen,
                   struct search *sr, int replied,
                   unsigned char *token, int token_len){
    struct search_node *n;
    int i, j;

    if(sa->family != sr->af){
        debugf("Attempted to insert node in the wrong family.\n");
        return 0;
    }

    for(i = 0; i < sr->numnodes; i++){
        if(id_cmp(id, sr->nodes[i].id) == 0){
            n = &sr->nodes[i];
            goto found;
        }
        if(xorcmp(id, sr->nodex[i].id, sr->id) < 0){
            break;
        }
    }

    if(i == SEARCH_NODES){
        return 0;
    }

    if(sr->numnodes < SEARCH_NODES){
        sr->numnodes++;
    }

    for(j=sr->numnodes - 1; j > i; j--){
        sr->nodes[j] = sr->nodes[j - 1];
    }

    n = &sr->nodes[i];

    memset(n, 0, sizeof(struct search_node));
    memcpy(n->id, id, 20);

found:
    memcpy(&n->ss, sa, salen);
    n->sslen = salen;

    if(replied){
        n->replied = 1;
        n->reply_time = now.tv_sec;
        n->request_time = 0;
        n->pinged = 0;
    }
    if(token){
        if(token_len >= 40){
            debugf("Eek! Overlong token.\n");
        }else{
            memcpy(n->token, token, token_len);
            n->token_len = token_len;
        }
    }

    return 1;
}

static void
flush_search_node(struct search_node *n, struct search *sr){
    int i = n - sr->nodes, j;
    for(j = i; j < sr->numnodes - 1; j++){
        sr->nodes[j] = sr->nodes[j + 1];
    }
    sr->numnodes--;
}

static void
expire_searches(void){
    struct search *sr = searches, *previous = NULL;

    while(sr){
        struct search *next = sr->next;
        if(sr->step_time < now.tv_sec - DHT_SEARCH_EXPIRE_TIME){
            if(previous){
                previous->next = next;
            }else{
                searches = next;
            }
            free(sr);
            numsearches--;
        }else{
            previous = sr;
        }
        sr = next;
    }
}

/* This must always return 0 or 1, never -1, not even on failure (see below). */
static int
search_send_get_peers(struct search *sr, struct search_node *n){
    struct node *node;
    unsigned char tid[4];
    
    if(n = NULL){
        int i;
        for(i = 0; i < sr->numnodes; i++){
            if(sr->nodes[i].pinged < 3 && !sr->nodes[i].replied &&
                sr->nodes[i].request_time < now.tv_sec - 15){
                n = &sr->nodes[i];
            }
        }
    }

    if(!n || n->pinged >= 3 || n->replied ||
       n->request_time >= now.tv_sec - 15){
       return 0;
    }

    debugf("Sending get_peers.\n");
    make_tid(tid, "gp", sr->tid);
    send_get_peers((struct sockaddr*)&n->ss, n->sslen, tid, 4, sr->id, -1,
                    n->reply_time >= now.tv_sec - 15);
    n->pinged++;
    n->request_time = now.tv_sec;
    /* If the node happens to be in our main routing table, mark it
       as pinged. */
    node = find_node(n->id, n->ss.ss_family);
    if(node){
        pinged(node, NULL);
    }
    return 1;
}

/* When a search is in progress, we periodically call search_step to send
   further requests. */
static void
search_step(struct search *sr, dht_callback *callback, void *closure){
    int i, j;
    int all_done = 1;

    /* Check if the first 8 live nodes have replied. */
    j = 0;
    for(i = 0; i < sr->numnodes && j < 8; i++){
        struct search_node *n = &sr->nodes[i];
        if(n->pinged >= 3){
            continue;
        }
        if(!n->replied){
            all_done = 0;
            break;
        }
        j++;
    }

	if(all_done){
		if(sr->port == 0){
			goto done;
		}else{
			int all_acked = 1;
			j = 0;
			for(i = 0; i < sr->numnodes && j < 8; i++){
				struct search_node *n = &sr->nodes[i];
				struct node *node;
				unsigned char tid[4];
				if(n->pinged >= 3){
					continue;
				}
				/* A proposed extension to the protocol consists in
				   omitting the token when storage tables are full. While
				   I don't think this makes a lot of sense -- just sending
				   a positive reply is just as good --, let's deal with it.*/
				if(n->token_len == 0){
					n->acked = 1;
				}
				if(!n->acked){
					all_acked = 0;
					debugf("Sending announce_peer\n");
					make_tid(tid, "ap", sr->tid);
					send_announce_peer((struct sockaddr*)&n->ss,
										sizeof(struct sockaddr_storage),
										tid, 4, sr->id, sr->port,
										n->token, n->token_len,
										n->reply_time >= now.tv_sec - 15);
					n->pinged++;
					n->request_time = now.tv_sec;
					node = find_node(n->id, n->ss.ss_family);
					if(node){
						pinged(node, NULL);
					}
				}
				j++;
			}
			if(all_acked){
				goto done;
			}
		}
		sr->step_time = now.tv_sec;
		return;
	}

	if(sr->step_time + 15 >= now.tv_sec){
		return;
	}

	j = 0;
	for(i = 0; i < sr->numnodes; i++){
		j += search_send_get_peers(sr, &sr->nodes[i]);
		if(j >= 3){
			break;
		}
	}
	sr->step_time = now.tv_sec;
	return;

done:
	sr->done = 1;
	if(callback){
		(*callback)(closure,
					sr->af == AF_INET ? 
					DHT_ENENT_SEARCH_DONE : DHT_EVENT_SEARCH_DONE6,
					sr->id, NULL, 0);
	}
	st->step_time = now.tv_sec;
}

static struct search*
new_search(void){
    struct search *sr, *oldest = NULL;
    
    /* Find the oldest done search */
    sr = searches;
    while(sr){
        if(sr->done && (oldest == NULL || oldest->step_time > sr->step_time)){
            oldest = sr;
        }
        sr = sr->next;
    }

    /* The oldest slot is pexired. */
    if(oldest && oldest->step_time < now.tv_sec - DHT_SEARCH_EXPIRE_TIME){
        return oldest;
    }

    /* Allocate a new slot. */
    if(numsearches < DHT_MAX_SEARCHES){
		sr = calloc(1, sizeof(search));
		if(sr != NULL){
			sr->next = searches;
			searches = sr;
			numsearches++;
			return sr;
		}
	}

	/* oh, well, never mind. Reuse the oldest slot. */
	return oldest;
}

/* Insert the contents of a bucket into a search structure. */
static void insert_search_bucket(struct bucket *b, struct search *sr){
	struct node *n;
	n = b->nodes;
	while(n){
		insert_search_node(n->id, (struct sockaddr*)&n->ss, n->sslen,
						   sr, 0, NULL, 0);
		n = n->next;
	}
}

/* Start a search. If port is non-zero, perform an announce when the
   search is complete. */
int dht_search(const unsigned char *id, int port, int af,
			   dht_callback *callback, void *closure){
	struct search *sr;
	struct storage *sr;
	struct bucket *b = find_bucket(id, af);

	if(b == NULL){
		errno = EAFNOSUPPORT;
		return -1;
	}

	/* Try to answer this search locally. In a fully grown DHT this
	   is very unlikely, but people are running modified versions of
	   this code in private DHTs with very few nodes. What's wrong
	   with flooding? */
	if(callback){
		st = find_storage(id);
		if(st){
			unsigned short swapped;
			unsigned char buf[18];
			int i;

			debugf("Found local data (%d peer).\n", st->numpeers);

			for(i = 0; i < st->numpeers; i++){
				swapped = htons(st->peers[i].port);
				if(st->peers[i].len == 4){
					memcpy(buf, st->peers[i].ip, 4);
					memcpy(buf + 4, &swapped, 2);
					(*callback)(closure, DHT_EVENT_VALUES, id,
								(void*)buf, 6);
				}else if(st->peers[i].len == 16){
					memcpy(buf, st->peers[i].ip, 16);
					memcpy(buf + 16, &swqpped, 2);
					(*callback)(blosure, DHT_EVENT_VALUES6, id,
								(void*)buf, 18);
				}
			}
		}
	}

	sr = searches;
	while(sr){
		if(sr->af == af && id_cmp(sr->id, id) == 0){
			break;
		}
		sr = sr->next;
	}

	if(sr){
	
	}else{
	
	}

	sr->port = port;

	insert_search_bucket(b, sr);

	if(sr->numnodes < SEARCH_NODES){
		struct bucket *p = previous_bucket(b);
		if(b->next){
		
		}
		if(p){
		}
	}


	return 1;
}
