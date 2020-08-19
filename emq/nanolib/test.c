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

	create_db_tree(&db);
	print_db_tree(db);
    struct topic_and_node *res = NULL;
	res = (struct topic_and_node*)zmalloc(sizeof(struct topic_and_node));
    char **topic_queue = topic_parse(data);
    search_node(db, topic_queue, res);

    if (res->topic) {
        printf("RES_TOPIC: %s\n", *(res)->topic);
    }
	if (res->node) {
		printf("RES_NODE_STATE: %d\n", res->t_state);
	}
	if (res->node->sub_client) {
		printf("RES_NODE_UP_ID: %s\n", res->node->sub_client->id);
	}
    zfree(res);
}

static void Test_add_node(void)
{
    puts(">>>>>>>>>>> TEST_ADD_NODE <<<<<<<<<");
    char *data = "a/bv/cv";
    char *data1 = "a/b/c";
    char *data2 = "lee/+/#";
    char *data3 = "lee/hom/jian";
	struct client ID3 = {"150410", NULL, NULL};
	struct client ID4 = {"150422", NULL, NULL};
	struct client ID5 = {"150418", NULL, NULL};

	create_db_tree(&db);

    struct topic_and_node *res = NULL;
	res = (struct topic_and_node*)zmalloc(sizeof(struct topic_and_node));
    char **topic_queue = topic_parse(data);

    search_node(db, topic_queue, res);
    add_node(res, &ID1);
	print_db_tree(db);

    search_node(db, topic_queue, res);
    add_node(res, &ID1);
    // add_client(res, &ID1);
    // printf("RES_NODE_ID: %s\n", res->node->sub_client->id);
    // printf("RES_NODE_STATE: %d\n", res->t_state);
	// if (res->topic) {
	// 	printf("RES_TOPIC: %s\n", *(res->topic));
	// }

    topic_queue = topic_parse(data1);
    search_node(db, topic_queue, res);
    add_node(res, &ID3);
	print_db_tree(db);
    search_node(db, topic_queue, res);
    printf("RES_NODE_ID: %s\n", res->node->sub_client->id);
    printf("RES_NODE_STATE: %d\n", res->t_state);
	if (res->topic) {
		printf("RES_TOPIC: %s\n", *(res->topic));
	}

    topic_queue = topic_parse(data2);
    search_node(db, topic_queue, res);
    add_node(res, &ID4);
	print_db_tree(db);
    search_node(db, topic_queue, res);

    // printf("RES_NODE_ID: %s\n", res->node->sub_client->id);
    // printf("RES_NODE_STATE: %d\n", res->t_state);
	// if (res->topic) {
	// 	printf("RES_TOPIC: %s\n", *(res->topic));
	// }

    // topic_queue = topic_parse(data3);
    // search_node(db, topic_queue, res);
    // add_node(res, &ID5);
	// print_db_tree(db);
    // search_node(db, topic_queue, res);

    // printf("RES_NODE_ID: %s\n", res->node->sub_client->id);
    // printf("RES_NODE_STATE: %d\n", res->t_state);
	// if (res->topic) {
	// 	printf("RES_TOPIC: %s\n", *(res->topic));
	// }
}

static void Test_del_node(void)
{
    puts(">>>>>>>>>> TEST_DEL_NODE <<<<<<<<");
    char *data = "lee/hom/jian/lihai";
    struct topic_and_node *res = NULL;
	char **topic_queue = topic_parse(data);
	res = (struct topic_and_node*)zmalloc(sizeof(struct topic_and_node));
    search_node(db, topic_queue, res);

	struct client ID2 = {"150410", NULL, NULL};
    add_node(res, &ID2);
	print_db_tree(db);
   	struct clients* res_clients = NULL;
   	res_clients = search_client(db->root, topic_queue);

	while (res_clients) {
		struct client *sub_client = res_clients->sub_client;
		while (sub_client) {
   			printf("RES: sub_client is:%s\n", sub_client->id);
   		    sub_client = sub_client->next;
   		}
		res_clients = res_clients->down;
	}
    search_node(db, topic_queue, res); 
    del_client(res, ID2.id);
    del_node(res->node);
    del_client(res, ID1.id);
    del_node(res->node);
    del_client(res, ID1.id);
    del_node(res->node);
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
	printf("INPUT: %d --> %s\n", i, topic1);
	printf("INPUT: %d --> %s\n", j, topic2);
	printf("INPUT: %d --> %s\n", k, topic3);


	hash_add_topic(i, topic1);
	hash_add_topic(i, topic2);
	hash_add_topic(i, topic3);
	hash_add_topic(j, topic2);
	hash_add_topic(k, topic3);
	char* t1 = hash_check_topic(i);
	char* t2 = hash_check_topic(j);
	char* t3 = hash_check_topic(k);

	printf("RES: %s\n", t1);
	printf("RES: %s\n", t2);
	printf("RES: %s\n", t3);
	hash_del_topic(i);
	hash_del_topic(j);
	hash_del_topic(k);
	t1 = hash_check_topic(i);
	t2 = hash_check_topic(j);
	t3 = hash_check_topic(k);
	if (t1) {
		printf("RES: %s\n", t1);
	}

	if (t2) {
		printf("RES: %s\n", t2);
	}
	
	if (t3) {
		printf("RES: %s\n", t3);
	}

}


void test() 
{
    puts("\n----------------TEST START------------------");
    // Test_topic_parse();
    // Test_search_node();
    Test_add_node();
    // Test_del_node();
	// Test_hash_table();
    puts("---------------TEST FINISHED----------------\n");
}


int main(int argc, char *argv[]) 
{
    test();

    return 0;
}














