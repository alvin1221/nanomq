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
	*db = (struct db_tree *)zmalloc(sizeof(struct db_tree)); 
	memset(*db, 0, sizeof(struct db_tree));
	struct db_node *node = (struct db_node *)zmalloc(sizeof(struct db_node));
	memset(node, 0, sizeof(struct db_node));
    node->topic = (char*)zmalloc(sizeof(char)*2);
	memcpy(&node->topic, "\0", sizeof(char)*2);
	(*db)->root = node;

}

struct db_node *new_db_node(char *topic)
{
		struct db_node *node = NULL;
		node = (struct db_node*)zmalloc(sizeof(struct db_node));
        node->len = 0;
        node->topic = (char*)zmalloc(strlen(topic)+1);
        memcpy(&node->topic, &topic, strlen(topic)+1);
        node->next = NULL;
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
/* Add node when sub do not find a node on the tree */
void add_node(struct topic_and_node *input, struct client *id)
{
    assert(input && id);
    struct db_node *tmp_node = NULL;
    struct db_node *new_node = NULL;
    char **topic_queue = input->topic;

    if (!strcmp(input->node->topic, *topic_queue)) {
        input->node->down = new_db_node(*(++topic_queue));
        input->node->down->up = input->node;
        new_node = input->node->down;

    } else {
		new_node = new_db_node(*topic_queue);
        new_node->up =  input->node->up;
        input->node->len++;

        if (input->node->next) {
            tmp_node = input->node->next;
            input->node->next = new_node;
            new_node->next = tmp_node; 
        } else {
            input->node->next = new_node;
        }
    }

    while (*(++topic_queue)) {
        new_node->down = new_db_node(*topic_queue);
        new_node->down->up = new_node;
        new_node = new_node->down;
    }
    new_node->sub_client = (struct client*)zmalloc(sizeof(struct client));
    memcpy(new_node->sub_client, id, sizeof(struct client));
    return;
}

void del_node(struct db_node *node) 
{
    assert(node);
    if (node->sub_client || node->down) {
		puts("Node can't be deleted!");
        return;
    }

    if (node->next) {
		puts("DELETE NODE NEXT!");
        struct db_node *first = node->next->up->down;
        if (first == node) {
            node->up->down = node->next;
            node->next->len = node->len-1;
        } else {
            while (first->next != node) {
                first = first->next;
            }
            first->next = first->next->next;
        }

        /* delete node */
        delete_db_node(node);
    } else {
		if (node->up == NULL) {
			return;
		}
		puts("DELETE NODE UP!");
        struct db_node *tmp_node = node->up;
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
    assert(input && id);
    // puts(id);
    // puts(input->node->sub_client->id);
    struct client *client = input->node->sub_client; 
    struct client *before_client = NULL; 
    while (client) {
        if (!strcmp(client->id, id)) {
            if (before_client) {
                before_client->next = before_client->next->next;
				delete_client(client);
            } else {
				// puts("del_client");
				// puts(input->node->sub_client->id);
				before_client = input->node->sub_client; 
				if (input->node->sub_client->next) {
					input->node->sub_client = input->node->sub_client->next;
				} else {
					input->node->sub_client = NULL;
				}

				delete_client(before_client);
				// delete_client(input->node->sub_client);
                // zfree(input->node->sub_client->id);
                // input->node->sub_client->id = NULL;
                // zfree(input->node->sub_client);
                // input->node->sub_client = NULL;
                // client->id = NULL;
                // client = NULL;
                break;
            }
        }
        before_client = client;
        client = client->next;
    }
    return;
}


/* Add client id. */
void add_client(struct topic_and_node *input, char *id)
{    
    assert(input && id);
    // puts(id);
    // puts(input->node->sub_client->id);
    struct client *cli_add = NULL;
    cli_add = (struct client*)zmalloc(sizeof(struct client));
    cli_add->id = (char*)zmalloc(strlen(id)+1);
    memcpy(cli_add->id, id, strlen(id)+1);
    puts(cli_add->id);

    if (input->node->sub_client == NULL) {
        input->node->sub_client = cli_add;
    } else {
        // input->node->len++;
        struct client* client = input->node->sub_client;
        while (client->next) { 
   // puts("2@@@@");
            client = client->next;
        }
    // puts(cli_add->id);
        client->next = cli_add;
		client->next->next = NULL;
    }
    return;
	/* fixed search client */

}


/* Search node */
void search_node(struct db_tree *db, char *topic_data, struct topic_and_node **tan)
{
    assert(db && topic_data);
    int len = 0;
    struct db_node *node = NULL;
    struct db_node *fnode = NULL;
    char **topic_queue = NULL;

    if (db->root) {
        node = db->root;
    }

    topic_queue = topic_parse(topic_data);

    // char **re = topic_queue;
    // while (*re) {
    //     printf("RES: %s\n", *re++);
    // }


    while (*topic_queue && node){
        if (strcmp(node->topic, *topic_queue)) {
		 // puts("node->topic");
            len = node->len;
            fnode = node;
            fnode->state = UNEQUAL;
            while (len > 0 && node->next) {
                node = node->next;
                if (!strcmp(node->topic, *topic_queue)) {
                    break;
                }
                len--;
            }
            if (len == 0) {
                (*tan)->node = fnode;
                (*tan)->topic = topic_queue;
				return;
            }
        }
        if (node->down && *(topic_queue+1)) {
            topic_queue++;
            node = node->down;
        } else if (*(topic_queue+1) == NULL) {
            node->state = EQUAL;
			(*tan)->topic = NULL;
 			(*tan)->node = node; 
            node->state = EQUAL;
			return;
        } else {
            node->state = UNEQUAL;
			(*tan)->topic = topic_queue;
 			(*tan)->node = node; 
			printf("node state : %d\n", (*tan)->node->state);
			// puts("2node->topic");
			return;
        }
    }
	return;
}


struct client *search_client(struct db_node *root, char **topic_queue)
{
    assert(root && topic_queue);
	size_t client_len = 1; 
	struct client *res = NULL;
    struct db_node *node = root;
	res = (struct client*)zmalloc(sizeof(struct client));
	memset(res, 0, sizeof(struct client));
	struct client *tmp_ptr = NULL;
	res->next = tmp_ptr;

    while (*topic_queue && node){
		if (strcmp(node->topic, *topic_queue)) {/* if can't ,add '+' judge here */
			int len = 0;
            len = node->len;
            while (len > 0 && node->next) {
                node = node->next;
                if (!strcmp(node->topic, *topic_queue)) {
                    break;
                }
                len--;
            }
            if (len == 0) {
				return res;
            }
        }
		// if (node->hashtag) {
		// 	struct client* sub_client = node->next->sub_client;
		// 	int len = sub_client->len;
		// 	client_len += len;
		// 	tmp_ptr = zrealloc(tmp_ptr, client_len * sizeof(struct client));
		// 	while (len > 0 && sub_client) {
		// 		tmp_ptr = sub_client;/* fixed */
		// 		tmp_ptr = tmp_ptr->next;
		// 		sub_client = sub_client->next;
		// 		len--;
		// 	}
		// }

		// if (node->plus) { 
		// 	if (node->down->down && *(topic_queue+2)) {
		// 		// search_client(node->down, topic_queue+1);
		// 		tmp_ptr = search_client(node->down->down, topic_queue+2);
		// 		while (tmp_ptr) {
		// 			client_len++;
		// 			tmp_ptr = tmp_ptr->next;
		// 		}

        // 	} else if (*(topic_queue+2) == NULL) {
		// 		struct client* sub_client = node->next->sub_client;
		// 		int len = sub_client->len;
		// 		client_len += len;
		// 		tmp_ptr = zrealloc(tmp_ptr, client_len * sizeof(struct client));
		// 		while (len > 0 && sub_client) {
		// 			tmp_ptr = sub_client;/* fixed */
		// 			tmp_ptr = tmp_ptr->next;
		// 			sub_client = sub_client->next;
		// 			len--;
		// 		}
        // 	} else if (node->down->down == NULL) { /* + is leaf */
		// 		/* fixed judge # */
		// 		int len = 0;
		// 		if (node->down->next) {
		// 			node = node->down;
        //     		len = node->len;
        //     		while (len > 0 && node->next) {
        //     		    node = node->next;
        //     		    if (!strcmp(node->topic, *(topic_queue+1))) {
        //     		        break;
        //     		    }
        //     		    len--;
        //     		}
		// 		}
		// 		if (len == 0) {
		// 			return res;
		// 		}
		// 		if (node->hashtag) {
		// 			struct client* sub_client = node->next->sub_client;
		// 			int len = sub_client->len;
		// 			client_len += len;
		// 			tmp_ptr = zrealloc(tmp_ptr, client_len * sizeof(struct client));
		// 			while (len > 0 && sub_client) {
		// 				tmp_ptr = sub_client;/* fixed */
		// 				tmp_ptr = tmp_ptr->next;
		// 				sub_client = sub_client->next;
		// 				len--;
		// 			}

		// 		}
		// 		return res;
		// 		} 
		// } else {
			if (node->down && *(topic_queue+1)) {
        	    topic_queue++;
        	    node = node->down;
        	} else if (*(topic_queue+1) == NULL) {
				struct client* sub_client = node->sub_client;
				printf("sub_client len %ld\n", sub_client->len);
				int len = sub_client->len;
				client_len += len+1;
				printf("client len %ld\n", client_len);
				tmp_ptr = (struct client*)zmalloc(client_len * sizeof(struct client));
				// tmp_ptr = zmalloc(tmp_ptr, client_len * sizeof(struct client));
				while (len+1 > 0 && sub_client) {
					puts(sub_client->id);
					tmp_ptr->id = sub_client->id;/* fixed */
					puts(tmp_ptr->id);
					tmp_ptr = tmp_ptr->next;
					sub_client = sub_client->next;
					len--;
				}

				while (res->next) { 
					res = res->next;
					puts(res->id);
				}
					
				return res;
        	} else {
				return res;
        	}
		// }
	}

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

