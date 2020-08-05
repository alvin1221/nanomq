
#include <stdlib.h>
#include "core/nng_impl.h"

//int hex_to_oct(char *str);
//
//uint32_t htoi(char *str);

uint8_t put_var_integer(uint8_t *dest, uint32_t value);

uint32_t get_var_integer(const uint8_t *buf, int *pos);

int32_t get_utf8_str(char *dest, const uint8_t *src, int *pos);

int utf8_check(const char *str, size_t length);

uint16_t get_variable_binary(uint8_t *dest, const uint8_t *src);

int fixed_header_adaptor(uint8_t *packet, nni_msg *dst);