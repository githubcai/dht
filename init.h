int dht_init(int s, int s6, const unsigned char *id, const unsigned char *v){
    int rc;

    if(dht_socket >= 0 || dht_socket6 >= 0 || buckets || buckets6){
        errno = EBUSY;
        return -1;
    }

    searches = NULL;
    numsearches = 0;

    storage = NULL;
    numstorage = 0;

    if(s >= 0){
        buckets = calloc(sizeof(struct bucket), 1);
        if(buckets == NULL){
            return -1;
        }
        buckets->af = AF_INET;

        rc = set_nonblocking(s, 1);
        if(rc < 0){
            goto fail;
        }
    }

    if(s6 >= 0){
        buckets6 = calloc(sizeof(struct bucket), 1);
        if(buckets6 == NULL){
            return -1;
        }
        buckets6->af = AF_INET6;

        rc = set_nonblocking(s6, 1);
        if(rc < 0){
            goto fail;
        }
    }

    memcpy(myid, id, 20);
    if(v){
    
    }else{
    
    }

    gettimeofday(&now, NULL);

    mybucket_grow_time = now.tv_sec;
    mybucket6_grow_time;
    confirm_nodes_time = now.tv_sec ;

    search_id = random() & 0xFFFF;
    search_time = 0;

    next_blacklisted = 0;

    token_bucket_time = now.tv_sec;
    token_bucket_tokens = MAX_TOKEN_BUCKET_TOKENS;

    memset(secret, 0, sizeof(secret));
    rc = rotate_secrets();
    if(rc < 0){
        goto fail;
    }

    dht_socket = s;
    dht_socket6 = s6;

    expire_buckets(buckets);
    expire_buckets(buckets6);

    return 1;

fail:
    free(buckets);
    buckets = null;
    return -1;
}
