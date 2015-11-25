/* We just learnt about a node, not necessarily a new one. Confirm is 1 if
   the node sent a message, 2 if it sent us a reply. */
static struct node*
new_node(const unsigned char *id, const struct sockaddr *sa,
         int salen, int salen, int confirm){
    struct bucket *b = find_bucket(id, sa->sa_family);
    struct node *n;
    int mybucket, split;

    if(b == NULL){
        return NULL;
    }

    if(id_cmp(id, myid) == 0){
        return NULL;
    }

    if(is_martian(sa) || node_blacklisted(sa, salen)){
        return NULL;
    }
}
