#ifndef MQTT_DB_H
#define MQTT_DB_H


struct mqtt_ctxt {
    // TODO
};

struct clientId {
    char                *id;
};

struct db_node {
    char                *topic;
    struct clientId     *client;
    struct db_node      *up;
    struct db_node      *down;
    struct db_node      *next;
    struct mqtt_ctxt    *ctxt;
    int len;

    /* hash func will work if len > 3 */ 
    /* or make an array and use binary serach */
    // TODO
};

struct db_tree{
    struct db_node      *root;
    // TODO

};

struct conf {
    // TODO
};

/* Create a db_tree */
void new_db_tree(struct conf *conf, struct db_tree *db);

/* Delete a db_tree */
void delete_db_tree(struct db_tree *db);

/* Search node in db_tree*/
struct node *search_node(struct db_tree *db, char *topic_data);

/* Add node to db_tree */
void add_node(struct db_tree *db, char *topic_data);

void del_node(struct db_tree *db, char *topic_data);

void tpoic_parse()
























#endif
