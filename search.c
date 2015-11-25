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

    for(i=0; i < sr->numnodes; i++){
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
    if(numsearches < DHT_MAX_SERCH_EXPIRE_TIME)
}
