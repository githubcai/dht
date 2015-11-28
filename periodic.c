int
dht_periodic(const void *buf, size_t buflen,
			 const struct sockaddr *from, int fromlen,
			 time_t *tosleep,
			 dht_callback *callback, void *closure){
	gettimeofday(&now, NULL);

	if(bufflen > 0){
		int message;
		unsigned char tid[16], id[20], info_hash[20], target[20];
		unsigned char nodes[256], nodes6[1024], token[128];
		int tid_len = 16, token_len = 128;
		int nodes_len = 256, nodes6_len = 1024;
		unsigned short port;
		unsigned char values[2048], values6[2048];
		int values_len = 2048, values6_len = 2048;
		int want;
		unsigned short ttid;

		if(is_martian(from)){
			goto dontread;
		}

		if(node_blacklisted(from, fromlen)){
			debugf("Unterminated message.\n");
			errno = EINVAL;
			return -1;
		}

		message = parse_message(buf, buflen, tid, &tid_len, id, info_hash,
								target, &port, token, &token_len,
								nodes, &nodes_len, nodes6, &nodes6_len,
								values, &values_len, values6, &values6_len,
								&want);

		if(message < 0 || message == ERROR || id_cmp(id, zeroes) == 0){
			debugf("Unparseable message: ");
			debug_printable(buf, buflen);
			debugf("\n");
			goto dontread;
		}

		if(id_cmp(id, myid) == 0){
			debugf("Received message from self.\n");
			goto dontread;
		}

		if(message > REPLY){
			/* Rate Limit request. */
			if(!token_bucket()){
				debugf("Dropping request due to rate limiting.\n");
				goto dontread;
			}
		}

		switch(message){
		case REPLY:
			if(tid_len != 4){
				debugf("Broken node truncates transaction ids: ");
				debuf_printable(buf, buflen);
				debugf("\n");
				/* This is really annoying, as it means that we will
				   time-out all our searches that go through this node. */
				backlist_node(id, from, fromlen);
				goto dontread;
			}
			if(tid_match(tid, "pn", NULL)){
				debugf("Pong!\n");
				new_node(id, from fromlen, 2);
			}else if(tid_match(tid, "fn", NULL) ||
					 tid_match(tid, "gp", NULL)){
				int gp = 0;
				struct search *sr = NULL;
				if(tid_match(tid, "gp", &ttid)){
					gp = 1;
					sr = find_search(ttid, from->sa_family);
				}
				debugf("Nodes found (%d+%d)%s!\n", nodes_len / 26, nodes6_len / 38,
					   gp ? " for get_peers" : "");
				if(nodes_len % 26 != 0 || nodes6_len % 38 != 0){
					debugf("Unexpected length for node info!\n");
					blacklist_node(id, from, fromlen);
				}else if(gp && sr == NULL){
					debugf("Unknown search!\n");
					new_node(id, from, fromlen, 1);
				}else{
					int i;
					new_node(id, from, fromlen, 2);
					for(i = 0; i < nodes_len / 26; i++){
						unsigned char *ni = nodes + i * 26;
						struct sockaddr_in sin;
						if(id_cmp(ni, myid) == 0){
							continue;
						}
						memset(&sin, 0, sizeof(sin));
						sin.sin_family = AF_INET;
						memcpy(&sin.sin_addr, ni + 20, 4);
						memcpy(&sin.sin_port, ni + 20, 2);
						new_node(ni, (struct sockaddr*)&sin, sizeof(sin), 0);
						if(sr && sr->af == AF_INET){
							insert_search_node(ni,
											   (struct sockaddr*)&sin,
											   sizeof(sin),
											   sr, 0, NULL, 0);
						}
					}
					for(i = 0; i < nodes6_len / 38; i++){
						unsigned char *ni = nodes6 + i * 38;
						struct sockaddr_in6 sin6;
						if(id_cmp(ni, myid) == 0){
							continue;
						}
						memset(&sin6, 0, sizeof(sin6));
						sin6.sin6_family = AF_INET6;
						memcpy(&sin6.sin6_addr, ni + 20, 16);
						memcpy(&sin6.sin6_port, ni + 36, 2);
						new_node(ni, (struct sockaddr*)&sin6, sizeof(sin6), 0);
						if(sr && sr->af == AF_INET6){
							insert_search_node(ni,
											   (struct sockaddr*)&sin6,
											   sizeof(sin6),
											   sr, 0, NULL, 0);
						}
					}
					if(sr){
						/* Since we received a reply, the number of
						   requests in flight has decreased. Let's push
						   another request. */
						search_send_get_peer(sr, NULL);
					}
					if(sr){
						insert_search_node(id, from, fromlen, sr,
										   1, token, token_len);
						if(values_len > 0 || values6_len > 0){
							debugf("Got values (%d+%d)!\n",
									values_len / 6, values6_len / 18);
							if(callback){
								if(values_len > 0){
									(*callback)(closure, DHT_EVENT_VALUES, sr->id,
												(void*)values, values_len);
								}
								if(values6_len > 0){
									(*callback)(closure, DHT_EVENT_VALUES6, sr->id,
												(void*)values6, values6_len);
								}
							}
						}
					}
				}
			}else if(tid_match(tid, "ap", &ttid)){
				struct search *sr;
				debugf("Got reply to announce_peer.\n");
				sr = find_search(ttid, from->sa_family);
				if(!sr){
					debugf(id, from, fromlen, 2);
					new_node(id, from, fromlen, 1);
				}else{
					int i;
					new_node(id, from, fromlen, 2);
					for(i = 0; i < sr->numnodes; i++){
						if(id_cmp(sr->nodes[i].id, id) == 0){
							sr->nodes[i].request_time = 0;
							sr->nodes[i].reply_time = now.tv_sec;
							sr->nodes[i].acked = 1;
							sr->nodes[i].pinged = 0;
							break;
						}
					}
					/* See comment for gp above. */
					search_send_get_peers(sr, NULL);
				}
			}else{
				debugf("Unexpected reply: ");
				debug_printable(buf, buflen);
				debugf("\n");
			}
			break;
		case PING:
			debugf("Ping (%d)!\n", tid_len);
			new_node(id, from, fromlen, 1);
			debugf("Sending pong.\n");
			send_pong(from, fromlen, tid, tid_len);
			break;
		case FIND_NODE:
			debugf("Find node!\n");
			new_node(id, from, fromlen, 1);
			debugf("Sending closest nodes (%d).\n", want);
			send_closest_nodes(from, fromlen,
							   tid, tid_len, target, want,
							   0, NULL, NULL, 0);
			break;
		case GET_PEERS:
			debugf("Get_peers!\n");
			new_node(id, from, fromlen, 1);
			if(id_cmp(info_hash, zeroes) == 0){
				debugf("Eek! Got get_peers with no info_hash.\n");
				send_error(from, fromlen, tid, tid_len,
						   "203", "Get_peers with no info_hash");
				break;
			}else{
				struct storage *st = find_storage(info_hash);		
				unsigned char token[TOKEN_SIZE];
				make_token(from, 0, token);
				if(st && st->numpeers > 0){
					debugf("Sending found%s peers.\n",
							from->sa_family == AF_INET6 ? " IPv6" : "");
					send_closest_nodes(from, fromlen,
									   tid, tid_len,
									   info_hash, want,
									   from->sa_family, st,
									   token, TOKEN_SIZE);
				}else{
					debugf("Sending nodes for get_peers.\n");
					send_closest_ndoes(from, fromlen,
									   tid, tid_len, info_hash, want,
									   0, NULL, token, TOKEN_SIZE);
				}
			}
			break;
		case ANNOUNCE_PEER:
			debugf("Announce peer!\n");
			new_node(id, from, fromlen, 1);
			if(id_cmp(info_hash, zeroes) == 0){
				debugf("Announce_peer with no info_hash.\n");
				send_error(from, fromlen, tid, tid_len,
						   203, "Announce_peer with wrong info_hash");
				break;
			}
			if(!token_match(token, token_len, from)){
				debugf("Incorrect token for announce_peer.\n");
				send_error(from, fromlen, tid, tid_len,
						   203, "Announce_peer with wrong token");
				break;
			}
			if(port == 0){
				debugf("Announce_peer with forbidden port %d.\n", port);
				send_error(from, fromlen, tid, tid_len,
						   203, "Announce_peer with forbidden port number");
				break;
			}
			storage_store(info_hash, from, port);
			/* Note that if storage_store failed, we lie to the requestor.
			   This is to prevent them from backtracking, and hence 
			   polluting the DHT. */
			debugf("Sending peer announced.\n");
			send_peer_announced(from, fromlen, tid, tidlen);
		}
	}

dontread:
	if(now.tv_sec >= rotate_secrets_time){
		rotate_secrets();
	}

	if(now.tv_sec >= expire_stuff_time){
		expire_bucket(buckets);
		expire_buckets(buckets6);
		expire_storage();
		expire_searches();
	}

	if(search_time > 0 && now.tv_sec >= search_time){
		struct search *sr;
		sr = searches;
		while(sr){
			if(!sr->done && sr->step_time + 5 <= now.tv_sec){
				search_step(sr, callback, closure);
			}
			sr = sr->next;
		}

		search_time = 0;

		sr = searches;
		while(sr){
			if(!s->done){
				time_t tm = s->step_time + 15 + random() % 10;
				if(search_time == 0 || search_time > tm){
					search_time = tm;
				}
			}
			sr = sr->next;
		}
	}

	if(now.tv_sec >= confirm_nodes_time){
		int soon = 0;

		soon |= bucket_maintenance(AF_INET);
		soon |= bucket_maintenance(AF_INET6);

		if(!soon){
			if(mybucket_grow_time >= now.tv_sec - 150){
				soon |= neighbourhood_maintenance(AF_INET);
			}
			if(mybucket6_grow_time >= now.tv_sec - 150){
				soon |= neighbourhood_maintenance(AF_INET6);
			}
		}

		/* In order to maintain all buckets' age within 600 seconds, worst
		   case is roughtly 27 seconds, assuming the table is 22 bits deep.
		   We want to keep a margin for neighborhood maintenance, so keep
		   this within 25 second.*/
		if(soon){
			confirm_nodes_time = now.tv_sec + 5 + random() % 20;
		}else{
			confirm_nodes_time = now.tv_sec + 60 + random() % 120;
		}
	}

	if(confirm_nodes_time > now.tv_sec){
		*tosleep = confirm_nodes_time - now.tv_sec;
	}else{
		*tosleep = 0;
	}

	if(search_time > 0){
		if(search_time <= now.tv_sec){
			*tosleep = 0;
		}else if(*tosleep > search_time - now.tv_sec){
			*tosleep = search_time - now.tv_sec;
		}
	}

	return 1;
}
