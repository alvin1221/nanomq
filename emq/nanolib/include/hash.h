#ifndef HASH_H
#define HASH_H

#ifdef __cplusplus
extern "C" {
#endif
struct topic {
	char	*topic_data;
	struct	topic *next;
};
void push_val(int key, struct topic *val) ;

struct topic *get_val(int key);

void del_val(int key); 
#ifdef __cplusplus
}
#endif

#endif
