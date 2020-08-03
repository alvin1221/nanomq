#include <stdio.h>
#include "mqtt_db.h"
#include "zmalloc.h"
#include "hash.h"


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

static void Test_hash_table(void) 
{
    puts(">>>>>>>>>>TEST_HASH_TABLE<<<<<<<<");
	int i = 1;
	int j = 2;
	int k = 3;
	char *topic1 = "topic1";
	char *topic2 = "topic2";
	char *topic3 = "topic3";
	struct topic topic_data1 = {topic1, NULL};
	struct topic topic_data2 = {topic2, NULL};
	struct topic topic_data3 = {topic3, NULL};
	printf("INPUT: %d --> %s\n", i, topic_data1.topic_data);
	printf("INPUT: %d --> %s\n", j, topic_data2.topic_data);
	printf("INPUT: %d --> %s\n", k, topic_data3.topic_data);


	hash_add_topic(i, &topic_data1);
	hash_add_topic(i, &topic_data2);
	hash_add_topic(i, &topic_data3);
	hash_add_topic(j, &topic_data2);
	hash_add_topic(k, &topic_data3);
	struct topic* t1 = hash_check_topic(i);
	struct topic* t2 = hash_check_topic(j);
	struct topic* t3 = hash_check_topic(k);
	while (t1) {
		printf("RES: t1 %s\n", t1->topic_data);
		t1 = t1->next;
	}

	printf("RES: %s\n", t2->topic_data);
	printf("RES: %s\n", t3->topic_data);
	hash_del_topic(i);
	hash_del_topic(j);
	hash_del_topic(k);
	t1 = hash_check_topic(i);
	t2 = hash_check_topic(j);
	t3 = hash_check_topic(k);
	if (t1) {
		printf("RES: %s\n", t1->topic_data);
	}

	if (t2) {
		printf("RES: %s\n", t2->topic_data);
	}
	
	if (t3) {
		printf("RES: %s\n", t3->topic_data);
	}

}


void test() 
{
    puts("---------------TEST----------------");
    Test_topic_parse();
    Test_search_node();
    Test_add_node();
    Test_del_node();
	Test_hash_table();
    puts("---------------TEST----------------");
}
















