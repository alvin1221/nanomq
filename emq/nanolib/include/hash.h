#ifndef HASH_H
#define HASH_H

#ifdef __cplusplus
extern "C" {
#endif
	void push_val(int key, char *val);

	char *get_val(int key);

	void del_val(int key); 
#ifdef __cplusplus
}
#endif

#endif
