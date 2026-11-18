/* mys1_conv.c: ks. mys1_conv.h. */
#include "mys1_conv.h"
#define COUNTS_PER_DEGREE  64     /* datalehti 4.1: 64 raakalukua = 1 °C */
#define CENTI_PER_DEGREE   100    /* sadasosa-asteet */

int32_t mys1_raw_to_centi(int16_t raw)
{
    return ((int32_t)raw * CENTI_PER_DEGREE) / COUNTS_PER_DEGREE;     /* (1) CENTI_PER_DEGREE * raw / COUNTS_PER_DEGREE */
}
