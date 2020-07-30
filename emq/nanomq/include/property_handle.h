#include <string.h>

#include "packet.h"
#include "nng/nng.h"

int type_of_variable_property(uint32_t id);
void property_linklist_init(struct mqtt_property * list);
int property_linklist_insert(struct mqtt_property * list, uint32_t id, uint8_t * bin);
int property_list_clear(struct mqtt_property * list);
struct property * property_list_head(struct mqtt_property * list);
struct property * property_list_end(struct mqtt_property * list);
struct property * property_list_get_element(struct mqtt_property * list, int pos);
struct property * property_list_find_element(struct mqtt_property * list, uint32_t id);

