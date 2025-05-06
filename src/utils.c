#include "utils.h"

void lerpi(int24_t *value, int24_t target, int24_t t_num, int24_t t_den) {
    *value += (target - (*value)) * t_num / t_den;
}

char *uint24_to_str(char *result, uint24_t value) {
    char *ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= 10;
        *ptr++ = "0123456789" [tmp_value - value * 10];
    } while ( value );

    *ptr-- = '\0';
  
    // reverse the string
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }

    return result;
}