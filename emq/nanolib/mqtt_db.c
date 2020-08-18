#include <string.h>
#include <assert.h>

#include "include/mqtt_db.h"
#include "include/zmalloc.h"
#include "include/hash.h"
#include "include/dbg.h"


/* Create a db_tree */
/* TODO */
void create_db_tree(struct db_tree **db)
{
	log_info("CREATE_DB_TREE_START");
	*db = (struct db_tree *)zmalloc(sizeof(struct db_tree)); 
	memset(*db, 0, sizeof(struct db_tree));

	struct db_node *node = new_db_node("\0");
	(*db)->root = node;
	return;
}


void destory_db_tree(struct db_tree *db)
{
	log_info("DESTORY_DB_TREE_START");
	/* TODO */

	log_info("DESTORY_DB_TREE_SUCCESSFULLY\n");
}

void print_db_tree(struct db_tree *db) 
{
	assert(db);
	struct db_nodes *tmps = NULL;
	struct db_nodes *tmps_end = NULL;
	tmps = (struct db_nodes*)zmalloc(sizeof(struct db_nodes));
	tmps->node = db->root;
	int size = 1;
	int len = size;
	tmps->next = NULL;
	tmps_end = tmps;

	puts("-------------------DB_TREE---------------------");
	puts("TOPIC | HASHTAG&PLUS | CLIENTID | FATHER_NODE");
	puts("-----------------------------------------------");

	while (tmps) {            
		size = 0;
		while (len-- && tmps) {
			struct db_node *tmp = tmps->node;
	 		while (tmp) {
	 			printf("\"%s\" ", tmp->topic);
	 			printf("%d", tmp->hashtag);
	 			printf("%d ", tmp->plus);
	 			if (tmp->sub_client) {
	 				printf("%s ", tmp->sub_client->id);
					if (tmp->sub_client->next) {
						printf("and more ");
					} else {
						printf("no more ");
					}

	 			} else {
					printf("-- ");
				}
				if (tmp->up) {
					if (strcmp("#", tmp->topic)) {
						printf("\"%s\"\t", tmp->up->topic);
					} else {
						printf("<--\t");
					}

				}



				if (tmp->down) {
					// debug("sth new");
					size++;
					tmps_end->next = (struct db_nodes*)zmalloc(sizeof(struct db_nodes)); 
					tmps_end = tmps_end->next;
					tmps_end->node = tmp->down;
					tmps_end->next = NULL;
				}
	 			if (tmp) {
					// debug("tmp next");
	 				tmp = tmp->next;
	 			}
			}
			// debug("tmps next");
			tmps = tmps->next;
		}

		printf("\n");
		if (size == 0) {
			break;
		}
		puts("----------------------------------------------");
	
		len = size;

	}

	puts("-------------------DB_TREE---------------------\n");
}

bool check_hashtag(char *topic_data) 
{
	if (topic_data == NULL) {
		return false;
	}
	return !strcmp(topic_data, "#");

}

bool check_plus(char *topic_data) 
{
	if (topic_data == NULL) {
		return false;
	}
	return !strcmp(topic_data, "+");
}

struct db_node *new_db_node(char *topic)
{
		struct db_node *node = NULL;
		node = (struct db_node*)zmalloc(sizeof(struct db_node));
        node->topic = (char*)zmalloc(strlen(topic)+1);
        memcpy(node->topic, topic, strlen(topic)+1);
        // memcpy(&node->topic, &topic, strlen(topic)+1);
        node->next = NULL;
		node->down = NULL;
		node->sub_client = NULL;
		return node;
}

void delete_db_node(struct db_node *node)
{
    zfree(node->topic);
	node->topic = NULL;
	node->up = NULL;
	node->next = NULL;
	node->down = NULL;
    zfree(node);
    node = NULL;
}

void set_db_node(struct db_node *node, char **topic_queue)
{
	if (node) {
		debug("set_db_node topic is %s", *topic_queue);
		node->down = new_db_node(*(topic_queue));
		node->down->up = node;
		node->down->next = NULL;
		node->down->down = NULL;
	} else {
		/* TODO */
	}
}

void insert_db_node(struct db_node *new_node, struct db_node *old_node)
{
	if (old_node->next == new_node) {
		struct db_node *tmp_node = NULL;
		tmp_node = old_node->next; 
		old_node->next = new_node;
		new_node->next = tmp_node->next;
	}
	new_node->up = old_node->up ? old_node->up : NULL;
	return;
}


/* 
** Add node when sub do not find a node on the tree 
*/
void add_node(struct topic_and_node *input, struct client *id)
{
	/* 
	** add node with topic wildcard + and # 
	*/
	log_info("ADD_NODE_START");
    assert(input && id);
    struct db_node *tmp_node = NULL;
    struct db_node *new_node = NULL;
    char **topic_queue = input->topic;

	if (topic_queue == NULL) {
		log("Topic_queue is NULL, no topic is needed add!");
		return;
	}

    if (input->t_state == EQUAL) {
		if (input->hashtag) {/* # is the last data in topic */
			input->node->hashtag = true;
			if (input->node->next) {
				debug("next");
				new_node = new_db_node(*(++topic_queue));
				// insert_db_node(new_node, input->node);
				// tmp_node = input->node->next; 
				// input->node->next = new_node;
				// new_node->next = tmp_node->next;
				// new_node->up = input->node->up ? input->node->up : NULL;
				// new_node = new_node->up->down;
				log_info("Head insertion for hashtag add");

			} else {
				debug("no next");
				debug("input->node is %s", input->node->topic);
				input->node->next = new_db_node(*(++topic_queue));
				debug("input->node next is %s", input->node->next->topic);
				// insert_db_node(input->node->next, input->node);
				input->node->next->up = input->node->up ? input->node->up : NULL;
				input->node->next->next = NULL;
				input->node->next->down = NULL;
				new_node = input->node->next;
			}

		} else {

			set_db_node(input->node, ++topic_queue);
			if (check_plus(*(topic_queue))) {
				debug("equal, plus is true");
				input->node->plus = true;
			}
        	new_node = input->node->down;
		}

    } else {
		new_node = new_db_node(*topic_queue);
        new_node->up =  input->node;
		if (check_plus(*topic_queue)) {
			debug("unequal, plus is true");
			input->node->plus = true;
			tmp_node = input->node->down;
			input->node->down = new_node;
			new_node->next = tmp_node;
		} else {
			debug("unequal, plus is not true");
			if (input->node->down->next) {
        	    tmp_node = input->node->down->next;
        	    input->node->down->next = new_node;
        	    new_node->next = tmp_node; 
        	} else {
        	    input->node->down->next = new_node;
        	}
		}
    }

    while (*(++topic_queue)) {
		if (check_hashtag(*topic_queue)) {
			debug("set hashtag is true");
            new_node->hashtag = true;
			if (new_node->next) {
				tmp_node = new_db_node(*topic_queue);
				tmp_node->up = new_node->up ? new_node->up : NULL;
				tmp_node->down = NULL;
				tmp_node->next = new_node->next;
				new_node->next = tmp_node;

			} else {
				new_node->next = new_db_node(*topic_queue);
				new_node->next->up = new_node->up ? new_node->up : NULL;
				new_node->next->down = NULL;
				new_node->next->next = NULL;
			}
			new_node = new_node->next;
        	// new_node = new_node->up->down;
		} else {
			if (check_plus(*topic_queue)) {
				new_node->plus = true;
			}
			set_db_node(new_node, topic_queue);
        	new_node = new_node->down;
		}
    }
    new_node->sub_client = (struct client*)zmalloc(sizeof(struct client));
    memcpy(new_node->sub_client, id, sizeof(struct client));
    new_node->sub_client->next = NULL;
    return;
}
/*	For duplicate node 
	TODO*/
void del_node(struct db_node *node) 
{
    assert(node);
	log_info("DEL_NODE_START");
    if (node->sub_client || node->down || node->hashtag) {
		puts("Node can't be deleted!");
        return;
    }

    if (node->next) {
		log("DELETE NODE NEXT!");
		// if (strcmp
        struct db_node *first = node->up->down;
        if (first == node) {
            node->up->down = node->next ? node->next : NULL;
            // node->next->len = node->len-1;
        } else {
            while (first->next != node) {
                first = first->next;
            }
			if (first->hashtag) {
				first->hashtag = false;
			}
            first->next = first->next->next;
        }

        /* delete node */
        delete_db_node(node);
    } else {
		if (node->up == NULL) {
			return;
		}
		log("DELETE NODE UP!");
        struct db_node *tmp_node = node->up;
		if (tmp_node->plus) {
			tmp_node->plus = false;
		}
        delete_db_node(node);
        if (tmp_node->down == node) {
            tmp_node->down = NULL;
        }
        /* delete node */
        del_node(tmp_node);
        
        /* iter */
        /*
        while (1) {
        // TODO 

        }
        */
    }
    return;
}
        
void delete_client(struct client *client)
{
	if (client) {
		// if (client->id) {
		// 	zfree(client->id);
		// } 
		// puts("ttt");
		client->id = NULL;
		client->next = NULL;
		zfree(client);
		client = NULL;
	}
}

/* Delete client id. */
void del_client(struct topic_and_node *input, char *id)
{
	log_info("DEL_CLIENT_START");
    assert(input && id);
    struct client *client = input->node->sub_client; 
    struct client *before_client = NULL; 
    while (client) {
		debug("delete id is %s, client id is %s", id, client->id);
        if (!strcmp(client->id, id)) {
			log("delete client %s", id);
            if (before_client) {
                before_client->next = before_client->next->next;
				delete_client(client);
            } else {
				before_client = input->node->sub_client; 
				if (input->node->sub_client->next) {
					input->node->sub_client = input->node->sub_client->next;
				} else {
					input->node->sub_client = NULL;
				}

				delete_client(before_client);
                break;
            }
        }
        before_client = client;
        client = client->next;
    }
	if (client == NULL) {
		log("no client is deleted!");
	}
    return;
}

struct client *set_client(const char *id, void *ctxt) 
{
    assert(id);
    // assert(ctxt);
    struct client *sub_client = NULL;
    sub_client = (struct client*)zmalloc(sizeof(struct client));
    sub_client->id = (char*)zmalloc(strlen(id)+1);
    memcpy(sub_client->id, id, strlen(id)+1);
	sub_client->ctxt = ctxt;
	sub_client->next = NULL;
	return sub_client;

}

/* Add client id. */
void add_client(struct topic_and_node *input, struct client *sub_client)
{    
	log_info("ADD_CLIENT_START");
    assert(input && sub_client);

    // sub_client = (struct client*)zmalloc(sizeof(struct client));
    // memcpy(new_node->sub_client, sub_client, sizeof(struct client));
    // sub_client->next = NULL;

    if (input->node->sub_client == NULL) {
        input->node->sub_client = sub_client;
		log("add first client in this node");
    } else {
        struct client* client = input->node->sub_client;
        while (client->next) {/* TODO fixed clientID duplicated */ 
			if (strcmp(client->id, sub_client->id)) {
				client = client->next;
			} else {
				log("clientID you find is in the tree node");
				return;
			}
        }
		log("add client %s", sub_client->id);
        client->next = sub_client;
    }
	log_info("ADD_CLIENT_SUCCESSFULLY");
    return;
	/* fixed search client */
}


void set_topic_and_node(char **topic_queue, bool hashtag, state t_state, 
		struct db_node *node, struct topic_and_node *tan) 
{
	tan->t_state = t_state;
	tan->topic = topic_queue;
	tan->hashtag = hashtag; 
	tan->node = node;
	return;
}

/* 
** Return the last node equal topic
*/

void search_node(struct db_tree *db, char **topic_queue, struct topic_and_node *tan)
{
	log_info("SEARCH_NODE_START");
    assert(db->root && topic_queue);
    struct db_node *node = db->root;

    while (*topic_queue && node){
		debug("topic is: %s", *topic_queue);
        if (strcmp(node->topic, *topic_queue)) {
			bool equal;
            while (node->next) {
				equal = false;
                node = node->next;
                if (!strcmp(node->topic, *topic_queue)) {
					equal = true;
                    break;
                }
            }
            if (equal == false) {
				log("searching unqual");
				set_topic_and_node(topic_queue, false, UNEQUAL, node->up, tan);
				break;
            }
        }
		
		if (node->hashtag && check_hashtag(*(topic_queue+1))) {
			log("searching # with hashtag");
			set_topic_and_node(NULL, true, EQUAL, node->next, tan);
			debug("%s", node->next->sub_client->id);
			break;
		} else if (check_hashtag(*(topic_queue+1))) {
			log("searching # no hashtag");
			set_topic_and_node(topic_queue, true, EQUAL, node, tan);
			break;
		}

		log("searching no hashtag");
        if (node->down && *(topic_queue+1)) {
            topic_queue++;
            node = node->down;
        } else if (*(topic_queue+1) == NULL) {
			set_topic_and_node(NULL, false, EQUAL, node, tan);
			break;
        } else {
			set_topic_and_node(topic_queue, false, EQUAL, node, tan);
			break;
        }
    }
	return;
}

void *get_client_info(struct db_node *node) 
{
	/* TODO */
	return NULL;

}

void iterate_client(struct clients * sub_clients /*, void func*/) 
{
	/* TODO */
	/* iterator and do */

	/* func(client); */
}

struct clients *new_clients(struct client *sub_client)
{
	struct clients *sub_clients = NULL; 
	if (sub_client) {
		sub_clients = (struct clients*)zmalloc(sizeof(struct clients));
		sub_clients->sub_client = sub_client;
		sub_clients->down = NULL;
		debug("%s", sub_clients->sub_client->id);
	}
	return sub_clients;
}



struct clients *search_client(struct db_node *root, char **topic_queue)
{
    assert(root && topic_queue);
	log_info("SEARCH_CLIENT_START");
	struct clients *res = NULL;
	struct clients *tmp = NULL;
	tmp = (struct clients*)zmalloc(sizeof(struct clients));

	res = tmp;

	debug("%p, %p", res, tmp);
    struct db_node *node = root;

    debug("entry search");
    while (*topic_queue && node) {

        if (strcmp(node->topic, *topic_queue)) {
			bool equal;
            while (node->next) {
				equal = false;
                node = node->next;
                if (!strcmp(node->topic, *topic_queue)) {
					equal = true;
                    break;
                }
            }
            if (equal == false) {
				log("searching unqual");
				return res;
            }
        }

		if (node->hashtag) {
			log("find hashtag");
			tmp->down = new_clients(node->next->sub_client);
			debug("%p, %p", res, tmp);
			tmp = tmp->down;
		}

		if (node->plus) { 
			log("find plus");
        	if (*(topic_queue+1) == NULL) {
				tmp->down = new_clients(node->sub_client);
				tmp = tmp->down;
				return res;
        	}

        	if (*(topic_queue+2) == NULL) {
				debug("When plus is the last one");

				if (node->down->hashtag) {
					log("Find hashtag");
					tmp->down = new_clients(node->down->next->sub_client);
					tmp = tmp->down;
				}

				tmp->down = new_clients(node->down->sub_client);
				tmp = tmp->down;

				bool equal;
				struct db_node  *t = NULL;
				t = node->down->next;
				equal = false;
            	while (t->next) {
            	    t = t->next;
            	    if (!strcmp(t->topic, *(++topic_queue))) {
						equal = true;
            	        break;
            	    }
            	}
            	if (equal == false) {
					log("searching unqual");
					return res;
            	}

				if (t->hashtag) {
					log("Find hashtag");
					tmp->down = new_clients(t->next->sub_client);
					tmp = tmp->down;
				}

				tmp->down = new_clients(t->sub_client);
				tmp = tmp->down;
				return res;

			} else if (node->down->down == NULL) { 
				/* fixed judge # */
				debug("topic is longer than tree, check hashtag");

				if (node->down->hashtag) {
					log("Find hashtag");
					tmp->down = new_clients(node->down->next->sub_client);
					tmp = tmp->down;
				}

				bool equal;
				struct db_node  *t = NULL;
				t = node->down->next;
				equal = false;
            	while (t->next) {
            	    t = t->next;
            	    if (!strcmp(t->topic, *(++topic_queue))) {
						equal = true;
            	        break;
            	    }
            	}
            	if (equal == false) {
					log("searching unqual");
					return res;
            	}

				if (t->hashtag) {
					log("Find hashtag");
					tmp->down = new_clients(t->next->sub_client);
					tmp = tmp->down;
				}

				debug("t->topic, %s, topic_queue ,%s", t->down->topic,
						*(topic_queue+1));
				tmp->down = search_client(t->down, topic_queue+1);
				return res;

			} else if (node->down->down && *(topic_queue+2)) {
				debug("continue");
				char ** tmp_topic = topic_queue;
				tmp = search_client(node->down, topic_queue+1);
				if (tmp->down) {
					tmp = tmp->down;
				}
				tmp = search_client(node->down->down, tmp_topic+2);
			}

		} else {
			log("Find no plus & hashtag");
			if (node->down && *(topic_queue+1)) {
				debug("continue");
        	    topic_queue++;
        	    node = node->down;

        	} else if (*(topic_queue+1) == NULL) {
				tmp->down = new_clients(node->sub_client);
				debug("%s", tmp->down->sub_client->id);
				tmp = tmp->down;
				return res;
        	} else {
				return res;
        	}
		}
	}

	log_info("SEARCH_CLIENT_FINISHED");
	return res;
}

/* topic parsing */
char **topic_parse(char *topic)
{
    assert(topic != NULL);

    int row = 1;
    int len = 2;
    char **topic_queue = NULL;
    char *before_pos = topic;
    char *pos = NULL;

	if ((strncmp("$share", before_pos, 6) != 0 && strncmp("$SYS", before_pos, 4)
				!= 0)) {
        topic_queue = (char**)zmalloc(sizeof(char*)*row);
		topic_queue[row-1] = (char*)zmalloc(sizeof(char)*len);
        memcpy(topic_queue[row-1], "\0", (len));
        // strcpy(topic_queue[row-1], "");
		//	topic_queue[0][0] = '';
		//	topic_queue[row-1][len-1] = '\0';

	}

    while ((pos = strchr(before_pos, '/')) != NULL) {

        if (topic_queue != NULL) {
            topic_queue = (char**)zrealloc(topic_queue, sizeof(char*)*(++row));
        } else {
            topic_queue = (char**)zmalloc(sizeof(char*)*row);
        }

		
        len = pos-before_pos+1;
        topic_queue[row-1] = (char*)zmalloc(sizeof(char)*len);
        memcpy(topic_queue[row-1], before_pos, (len-1));
        topic_queue[row-1][len-1] = '\0';
        before_pos = pos+1;
    }

    len = strlen(before_pos);
    
    if (topic_queue != NULL) {
        topic_queue = (char**)zrealloc(topic_queue, sizeof(char*)*(++row));
    } else {
        topic_queue = (char**)zmalloc(sizeof(char*)*row);
    }

    topic_queue[row-1] = (char*)zmalloc(sizeof(char)*(len+1));
    // strcpy(topic_queue[row-1], before_pos);
    memcpy(topic_queue[row-1], before_pos, (len));
    topic_queue[row-1][len] = '\0';
    topic_queue = (char**)zrealloc(topic_queue, sizeof(char*)*(++row));
	topic_queue[row-1] = NULL;

    return topic_queue;
}


void hash_add_topic(int alias, char *topic_data) 
{
	assert(topic_data);
	push_val(alias, topic_data);
}

char *hash_check_topic(int alias)
{
	return get_val(alias);
}

void hash_del_topic(int alias)
{
	del_val(alias);
}

