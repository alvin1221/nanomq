#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include "hash.h"
#include <iostream>

using namespace std;
 
template<typename K, typename V>
class mqtt_hash {
public:
	typedef typename unordered_map<K, V>::iterator iterator;

	V &operator [](const K &_key)
	{
		lock_guard<mutex> lk(_mtx);
		return hash_map[_key];
	}

	V get(const K &_key)
	{
		lock_guard<mutex> lk(_mtx);
		return hash_map[_key];
	}

	void set(const K &_key, const V &_val) 
	{
		lock_guard<mutex> lk(_mtx);
		hash_map[_key] = _val;
	}

	void del(const K &_key) 
	{
		lock_guard<mutex> lk(_mtx);
		mqtt_hash<K, V>::iterator iter = hash_map.find(_key);

		if (iter != hash_map.end()) {
			hash_map.erase(iter);
		}
	}

private:
	unordered_map<K, V> hash_map;
	mutex _mtx;

};



mqtt_hash<int, struct topic*> _mqtt_hash;
 
 
void push_val(int key, struct topic *val)
{
	struct topic *tmp = _mqtt_hash[key];
	if (tmp == NULL) {
		_mqtt_hash[key] = val;
	} else {
		while (tmp->next) {
			tmp = tmp->next;
		}
		tmp->next = val;
	}

}
 
 
struct topic *get_val(int key)
{
	return _mqtt_hash.get(key);
}

void del_val(int key) 
{
	_mqtt_hash.del(key);

}
