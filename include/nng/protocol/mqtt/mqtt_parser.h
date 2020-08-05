
#include <stdlib.h>
#include "core/nng_impl.h"

int hex_to_oct(char *str);
uint32_t htoi(char *str);

uint32_t get_var_integer(const uint8_t *buf, int *pos);
static int32_t get_utf8_str(char *dest, const uint8_t *src, int *pos);
static int utf8_check(const char *str, size_t length);

int fixed_header_adaptor(uint8_t *packet, nni_msg *dst);