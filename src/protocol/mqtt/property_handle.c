#include <nng/nng.h>
#include <string.h>
#include <nng/protocol/mqtt/property_handle.h>
#include <nng/protocol/mqtt/mqtt_parser.h>

// u8-1 u16-2 u32-3 varint-4(u32) str-5 bin-6 strpair-7
int type_of_variable_property(uint8_t id){
	int lens[] = {
1	,4	,5	,0	,0	,0	,
0	,5	,6	,0	,4	,0	,
0	,0	,0	,0	,4	,5	,
2	,0	,5	,6	,1	,4	,
1	,5	,0	,5	,0	,0	,
5	,0	,2	,2	,2	,1	,
1	,7	,4	,1	,1	,1	,
};
	return lens[id-1];
}

void property_list_init(struct mqtt_property * list){
	list->len = 0;
	list->count = 0;
	list->property = NULL;
	list->property_end = NULL;
}

/*
 * @Return the length of binary inserted.
 *
 * */
int property_list_insert(struct mqtt_property * list, uint8_t id, uint8_t * bin){
	int res = 0;
	int t, t1, t2, t3;
	struct mqtt_string * s;
	struct mqtt_binary * b;
	struct mqtt_str_pair * sp;
	struct property * node = nng_alloc(sizeof(struct property));
	node->id = id;
	union Property_type val;
	node->value = val;
	int type = type_of_variable_property(id);
	switch(type){
		case 1:
			val.u8 = *bin;
			res = 1;
			break;
		case 2:
			t = *(bin+1);
			val.u16 = (t<<8) + *bin;
			res = 2;
			break;
		case 3:
			t1 = *(bin+3);
			t2 = *(bin+2);
			t3 = *(bin+1);
			val.u32 = (t1<<24) + (t2<<16) + (t3<<8) +*bin;
			res = 4;
			break;
		case 4:
			val.varint = get_var_integer(bin, &res);
			break;
		case 5:
			s = nng_alloc(sizeof(mqtt_string));
			s->len = (*(bin+1) << 8) + *bin;
			if(s->len != 0){
				s->str = nng_alloc(s->len + 1);
				memcpy(s->str, bin+2, s->len);
				s->str[s->len] = '\0';
			}
			val.str = *s;
			res = 2 + s->len;
			break;
		case 6:
			b = nng_alloc(sizeof(mqtt_binary));
			b->len = (*(bin+1) << 8) + *bin;
			if(b->len != 0){
				b->str = nng_alloc(b->len + 1);
				memcpy(b->str, bin+2, b->len);
				b->str[b->len] = '\0';
			}
			val.binary = *b;
			res = 2 + b->len;
			break;
		case 7:
			sp = nng_alloc(sizeof(mqtt_str_pair));
			sp->len1 = (*(bin+1) << 8) + *bin;
			if(sp->len1 != 0){
				sp->str1 = nng_alloc(sp->len1 + 1);
				memcpy(sp->str1, bin+2, sp->len1);
				sp->str1[sp->len1] = '\0';
			}else {
				break;
			}
			sp->len2 = (*(bin+1+sp->len1+2) << 8) + *(bin+sp->len1+2);
			if(sp->len2 != 0){
				sp->str2 = nng_alloc(sp->len2 + 1);
				memcpy(sp->str2, bin+4+sp->len1, sp->len2);
				sp->str2[sp->len2] = '\0';
			}
			val.strpair = *sp;
			res = 4 + sp->len1 + sp->len2;
			break;
		default:
			res = 0;
	}
	if(list->count == 0){
		list->count++;
		list->property = node;
		list->property_end = node;
	}else {
		list->count++;
		struct property * t = list->property_end;
		t->next = node;
		list->property_end = node;
	}
	return res;
}

int property_list_free(struct mqtt_property * list){
	struct property * node, *_node;
	for(int i=0; i<list->count; i++){
		node = list->property;
		if(type_of_variable_property(node->id) > 6){
			nng_free(&node->value, sizeof(mqtt_str_pair));
		}else if(type_of_variable_property(node->id) > 4){
			nng_free(&node->value, sizeof(mqtt_string));
		}

		_node = node->next;
		nng_free(node, sizeof(struct property));
		node = _node;
	}
}

struct property * property_list_head(struct mqtt_property * list){
	return list->property;
}

struct property * property_list_end(struct mqtt_property * list){
	return list->property_end;
}

struct property * property_list_get_element(struct mqtt_property * list, int pos){
	struct property * res = list->property;
	if(pos < 0){
		return NULL;
	}
	if(pos > list->count){
		return NULL;
	}
	if(pos == list->count){
		return list->property_end;
	}
	while(pos--){
		res = res->next;
	}
	return res;
}

struct property * property_list_find_element(struct mqtt_property * list, uint8_t id){
	struct property * node;
	int pos = list->count;
	while(pos--){
		node = list->property;
		if(node->id == id){
			return node;
		}else{
			node = node->next;
		}
	}
	return node;
}

int property_list_set_len(struct mqtt_property * list, int len){
	list->len = len;
	return 0;
}

