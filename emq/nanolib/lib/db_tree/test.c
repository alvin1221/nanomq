#include <stdio.h>
#include "mqtt_db.h"
#include "zmalloc.h"

struct clientId ID = {"150410"};
struct db_node node = {"lee", &ID, NULL, NULL, NULL, NULL, 1, -1};  
struct db_tree db = {&node};
struct clientId ID1 = {"150429"};

static void Test_topic_parse(void)
{
    puts(">>>>>>>>>> TEST_TOPIC_PARSE <<<<<<<<");
    // char *data = NULL;
    char *data = "lee";
    // char *data = "lee/hom/jian";
    printf("INPUT:%s\n", data);

    char **res = topic_parse(data);
    char *t = NULL;
    char **tt = res;

    while (*res) {
        t = *res;
        printf("RES: %s\n", *(res++));
        zfree(t);
        t = NULL;
    }

    zfree(tt);
    res = NULL;
}

static void Test_search_node(void)
{
    puts(">>>>>>>>>> TEST_SEARCH_NODE <<<<<<<<");
    char *data = "lee";

    printf("INPUT: %s\n", db.root->topic);
    printf("INPUT: %s\n", db.root->client->id);

    struct topic_and_node *res = search_node(&db, data);
    printf("RES_NODE_UP_ID: %s\n", res->node->client->id);
    printf("RES_NODE_STATE: %d\n", res->node->state);
    if (res->topic) {
        printf("RES_TOPIC: %s\n", *(res->topic));
    }
    zfree(res);
}

static void Test_add_node(void)
{
    puts(">>>>>>>>>>> TEST_ADD_NODE <<<<<<<<<");
    char *data = "lee/hom/jian";

    struct topic_and_node *res = search_node(&db, data);
    printf("RES_NODE_ID: %s\n", res->node->client->id);
    printf("RES_NODE_STATE: %d\n", res->node->state);
    printf("RES_TOPIC: %s\n", *(res->topic));

    add_node(res, &ID1);
    printf("NODE_NEW_ID: %s\n", db.root->down->down->client->id);
}

static void Test_del_node(void)
{
    puts(">>>>>>>>>> TEST_DEL_NODE <<<<<<<<");
    char *data = "lee/hom/jian";
    struct topic_and_node *res = search_node(&db, data);
    del_client(res, ID1.id);
    del_node(db.root->down->down);
}

void test() 
{
    puts("---------------TEST----------------");
    Test_topic_parse();
    Test_search_node();
    Test_add_node();
    Test_del_node();
    puts("---------------TEST----------------");
}
















