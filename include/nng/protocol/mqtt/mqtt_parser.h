
#include <stdlib.h>

uint8_t fixed_header_adaptor(uint8_t *packet);
int hex_to_oct(char *str);
uint32_t htoi(char *str);

uint32_t get_var_integer(const uint8_t *buf, int *pos);