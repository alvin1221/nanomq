#ifndef MQTT_DB_H
#define MQTT_DB_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum {UNEQUAL = -1, EQUAL = 0 } node_state;
    


struct mqtt_ctxt {
    // TODO
};

struct client {
    char				*id;
    struct mqtt_ctxt    *ctxt;
    struct client		*next;
};

struct db_node {
    char                *topic;
	bool				retain;
	void				*message;
    int					len;
    struct client		*sub_client;
    struct db_node      *up;
    struct db_node      *down;
    struct db_node      *next;
    node_state			state;
    // int					first;

    /* hash func will work if len > 3 */ 
    /* or make an array and use binary serach */
    // TODO
};

struct topic_and_node {
    char **topic;
    struct db_node *node; 

};

struct db_tree{
    struct db_node      *root;
    // TODO

};


/* Create a db_tree */
void create_db_tree(struct db_tree **db);

/* Delete a db_tree */
void delete_db_tree(struct db_tree *db);

/* Search node in db_tree*/
// struct topic_and_node *search_node(struct db_tree *db, char *topic_data);
void search_node(struct db_tree *db, char *topic_data, struct topic_and_node
		**tan);

/* Add node to db_tree */
void add_node(struct topic_and_node *input, struct client *id);

/* Delete node from db_tree when node does not have clientId */
void del_node(struct db_node *node);

/* Free node memory */
void free_node(struct db_node *node);

/* Parsing topic from char* with '/' to char** */
char **topic_parse(char *topic);

/* Delete client id. */
void del_client(struct topic_and_node *input, char *id);

/* Add client id. */
void add_client(struct topic_and_node *input, char *id);

/* A hash table, clientId or alias as key, topic as value */ 
struct topic* hash_check_topic(int alias);

void hash_add_topic(int alias, struct topic *topic_data);

void hash_del_topic(int alias);




#endif
