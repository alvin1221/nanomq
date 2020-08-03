#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "mqtt_db.h"
#include "zmalloc.h"
#include "hash.h"


/* Create a db_tree */
/* TODO */


/* Add node when sub do not find a node on the tree */
void add_node(struct topic_and_node *input, struct clientId *id)
{
    assert(input && id);
    struct db_node *tmp_node = NULL;
    struct db_node *new_node = NULL;
    char **topic_queue = input->topic;


    if (!strcmp(input->node->topic, *topic_queue)) {
        input->node->down = (struct db_node*)zmalloc(sizeof(struct db_node));
        input->node->down->topic = (char*)zmalloc(strlen(*(++topic_queue))+1);
        memcpy(&input->node->down->topic, topic_queue, strlen(*topic_queue)+1);
        input->node->down->up = input->node;
        new_node = input->node->down;
        input->node->down->len = 0;

    } else {
        new_node = (struct db_node*)zmalloc(sizeof(struct db_node));
        new_node->topic = (char*)zmalloc(strlen(*topic_queue)+1);
        memcpy(&new_node->topic, topic_queue, strlen(*topic_queue)+1);
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
        new_node->down = (struct db_node*)zmalloc(sizeof(struct db_node));
        new_node->down->topic = (char*)zmalloc(strlen(*topic_queue)+1);
        memcpy(&new_node->down->topic, topic_queue, strlen(*topic_queue)+1);
        new_node->down->len = 0;
        new_node->down->up = new_node;
        new_node->next = NULL;
        new_node = new_node->down;
    }
    new_node->client = (struct clientId*)zmalloc(sizeof(struct clientId));
    memcpy(new_node->client, id, sizeof(struct clientId));

    return;
}

void del_node(struct db_node *node) 
{
    assert(node);
    if (node->client || node->down) {
        return;
    }

    if (node->next) {
        struct db_node *first = node->next->up->down;
        if (first == node) {
            /* TODO change some args and free node*/
            node->up->down = node->next;
            node->next->len = node->len-1;
        } else {
            /* change some args */
            while (first->next != node) {
                first = first->next;
            }
            first->next = first->next->next;
        }

        /* delete node */
        free_node(node);
    } else {
        struct db_node *tmp_node = node->up;
        free_node(node);
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
        
/* Free node memory */
void free_node(struct db_node *node)
{
    // TODO
    zfree(node->topic);
    zfree(node->client);
    zfree(node);
    node = NULL;
}

/* Delete client id. */
void del_client(struct topic_and_node *input, char *id)
{
    assert(input && id);
    puts(id);
    puts(input->node->client->id);
    struct clientId *client = input->node->client; 
    struct clientId *before_client = NULL; 
    while (client) {
        if (!strcmp(client->id, id)) {
            if (before_client) {
                before_client->next = before_client->next->next;
            } else {
                input->node->client = NULL;
                client->id = NULL;
                client = NULL;
                break;
            }
        }
        before_client = client;
        client = client->next;
    puts("del_client");
    }
    return;
}



/* Add client id. */
void add_client(struct topic_and_node *input, char *id)
{    
    assert(input && id);
    puts(id);
    puts(input->node->client->id);
    struct clientId *cli_add = NULL;
    cli_add = (struct clientId*)zmalloc(sizeof(struct clientId));
    cli_add->id = (char*)zmalloc(strlen(id)+1);
    memcpy(&cli_add->id, id, strlen(id)+1);
    puts(cli_add->id);

    if (input->node->client == NULL) {
        input->node->client = cli_add;
    } else {
        input->node->len++;
        struct clientId* client = input->node->client;
        while (client->next) { 
            client = client->next;
        }
        client->next = cli_add;
    }
    return;

}


/* Search node */
struct topic_and_node *search_node(struct db_tree *db, char *topic_data)
{
    assert(db && topic_data);
    int len = 0;
    struct db_node *node = NULL;
    struct db_node *fnode = NULL;
    struct topic_and_node *res = NULL; 
    char **topic_queue = NULL;

    if (db->root) {
        node = db->root;
    }
    res = (struct topic_and_node*)zmalloc(sizeof(struct topic_and_node));
    // TODO memset ?
    topic_queue = topic_parse(topic_data);

    // char **re = topic_queue;
    // while (*re) {
    //     printf("RES: %s\n", *re++);
    // }


    while (*topic_queue && node){
        if (strcmp(node->topic, *topic_queue)) {
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
                res->node = fnode;
                res->topic = topic_queue;
                return res;
            }
        }
        if (node->down && *(topic_queue+1)) {
            topic_queue++;
            node = node->down;
        } else if (*(topic_queue+1) == NULL) {
            res->topic = NULL;
            node->state = EQUAL;
            res->node = node;
            return res;
        } else {
            node->state = EQUAL;
            res->topic = topic_queue;
            res->node = node;
            return res;
        }
    }

    return NULL;
}


/* topic parsing */
char **topic_parse(char *topic)
{
    assert(topic != NULL);

    int row = 1;
    int len = 0;
    char **topic_queue = NULL;
    char *before_pos = topic;
    char *pos = NULL;

    while ((pos = strchr(before_pos, '/')) != NULL) {

        if (topic_queue != NULL) {
            topic_queue = (char**)zrealloc(topic_queue, sizeof(char*)*(++row));
        } else {
            topic_queue = (char**)zmalloc(sizeof(char*)*row);
        }

        len = pos-before_pos+1;
        topic_queue[row-1] = (char*)zmalloc(sizeof(char)*len);
        memcpy(topic_queue[row-1], before_pos, (len-1));
        topic_queue[row-1][len] = '\0';
        before_pos = pos+1;
    }

    len = strlen(before_pos);

    if (topic_queue != NULL) {
        topic_queue = (char**)zrealloc(topic_queue, sizeof(char*)*(++row));
    } else {
        topic_queue = (char**)zmalloc(sizeof(char*)*row);
    }

    topic_queue[row-1] = (char*)zmalloc(sizeof(char)*(len+1));
    memcpy(topic_queue[row-1], before_pos, (len));
    topic_queue[row-1][len] = '\0';

    return topic_queue;
}


void hash_add_topic(int alias, struct topic *topic_data) 
{
	assert(topic_data);
	push_val(alias, topic_data);
}

struct topic *hash_check_topic(int alias)
{
	return get_val(alias);
}

void hash_del_topic(int alias)
{
	del_val(alias);
}


