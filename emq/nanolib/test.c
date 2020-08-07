#include <stdio.h>
#include <string.h>
#include "include/mqtt_db.h"
#include "include/zmalloc.h"
#include "include/hash.h"


// struct clientId ID = {"150410"};
// struct db_node node = {"", &ID, NULL, NULL, NULL, NULL, 1, -1};  
// struct db_tree db = {&node};

struct db_tree *db = NULL;
// struct db_tree *db = (struct db_tree*)zmalloc(sizeof(struct db_tree));

struct client ID1 = {"150429", NULL, NULL};

static void Test_topic_parse(void)
{
    puts(">>>>>>>>>> TEST_TOPIC_PARSE <<<<<<<<");
    // char *data = NULL;
    // char *data = "lee";
    // char *data = "$share/hom/jian";
    char *data = "$shar/hom/jian";
    printf("INPUT:%s\n", data);

    char **res = topic_parse(data);
    char *t = NULL;
    char **tt = res;


    while (*res) {
        t = *res;
        printf("RES: %s\n", *res);
		res++;
        zfree(t);
        t = NULL;
    }

    zfree(tt);
    res = NULL;
}

static void Test_search_node(void)
{
    puts(">>>>>>>>>> TEST_SEARCH_NODE <<<<<<<<");
    // char *data = "lee";
	char *data = "lee/hom/jian";


	db = (struct db_tree *)zmalloc(sizeof(struct db_tree)); 
	memset(db, 0, sizeof(struct db_tree));
	create_db_tree(&db);
    printf("INPUT: %d\n", db->root->len);
    printf("INPUT: %s\n", db->root->topic);
    struct topic_and_node *res = (struct topic_and_node*)zmalloc(sizeof(struct topic_and_node));
    search_node(db, data, &res);

    if (res->topic) {
        printf("RES_TOPIC: %s\n", *(res)->topic);
    }
	if (res->node) {
		printf("RES_NODE_STATE: %d\n", res->node->state);
	}
	if (res->node->sub_client) {
		printf("RES_NODE_UP_ID: %s\n", res->node->sub_client->id);
	}
    zfree(res);
}

static void Test_add_node(void)
{
    puts(">>>>>>>>>>> TEST_ADD_NODE <<<<<<<<<");
    char *data = "lee/hom/jian";

    struct topic_and_node *res = (struct topic_and_node*)zmalloc(sizeof(struct topic_and_node));
    search_node(db, data, &res);
    add_node(res, &ID1);
    search_node(db, data, &res);
    printf("RES_NODE_ID: %s\n", res->node->sub_client->id);
    printf("RES_NODE_STATE: %d\n", res->node->state);
	if (res->topic) {
		printf("RES_TOPIC: %s\n", *(res->topic));
	}
    printf("NODE_NEW_ID: %s\n", db->root->down->down->down->sub_client->id);
}

static void Test_del_node(void)
{
    puts(">>>>>>>>>> TEST_DEL_NODE <<<<<<<<");
    char *data = "lee/hom/jian";
    struct topic_and_node *res = (struct topic_and_node*)zmalloc(sizeof(struct topic_and_node));
    search_node(db, data, &res);
    del_client(res, ID1.id);
    del_node(res->node);
    // del_node(db->root->down->down->down);
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
    puts("\n----------------TEST START------------------");
    Test_topic_parse();
    Test_search_node();
    Test_add_node();
    Test_del_node();
	Test_hash_table();
    puts("---------------TEST FINISHED----------------\n");
}
















