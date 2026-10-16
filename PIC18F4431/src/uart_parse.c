// Minimal parsing helpers for small embedded systems (no stdlib heavy functions).

#include <stdint.h>
#include "../include/uart_parse.h"

/* parse_long:
 *  - Parses an optional leading '+' or '-' and decimal digits.
 *  - Stops at first non-digit.
 *  - Returns signed long (no errno, no overflow check).
 */
long parse_long(const char *s)
{
    long val = 0;
    int sign = 1;

    if (!s) return 0;
    if (*s == '+') { s++; }
    else if (*s == '-') { sign = -1; s++; }

    while (*s >= '0' && *s <= '9') {
        val = val * 10 + (*s - '0');
        s++;
    }
    return sign * val;
}

/* parse_fixed:
 *  - Parses an optional sign, an integer part and optional fractional part.
 *  - decimals parameter: number of fractional digits to keep (scale = 10^decimals).
 *  - Example: parse_fixed("12.345", 2) -> returns 1234 (12.34 with 2 decimals truncated).
 *  - It truncates extra fractional digits (no rounding).
 *  - Stops at first character that is not digit or '.'.
 *  - Returns signed long (scaled value).
 */
long parse_fixed(const char *s, uint8_t decimals)
{
    long int_part = 0;
    long frac_part = 0;
    uint8_t frac_digits = 0;
    int sign = 1;
    uint32_t scale = 1;
    uint8_t i;

    if (!s) return 0;

    for (i = 0; i < decimals; i++) scale *= 10;

    if (*s == '+') { s++; }
    else if (*s == '-') { sign = -1; s++; }

    // integer part
    while (*s >= '0' && *s <= '9') {
        int_part = int_part * 10 + (*s - '0');
        s++;
    }

    // fractional part (if any)
    if (*s == '.') {
        s++;
        while ((*s >= '0' && *s <= '9') && (frac_digits < decimals)) {
            frac_part = frac_part * 10 + (*s - '0');
            frac_digits++;
            s++;
        }
        // if fewer fractional digits than decimals, scale up (e.g., "1.2" with decimals=3 -> 200)
        while (frac_digits < decimals) {
            frac_part = frac_part * 10;
            frac_digits++;
        }
        // If there are extra fractional digits beyond `decimals`, skip them (truncate)
        while (*s >= '0' && *s <= '9') s++;
    } else {
        // no fractional digits: scale frac_part to match decimals (all zeros)
        for (i = 0; i < decimals; i++) frac_part *= 10; // frac_part already zero, this keeps logic symmetrical
    }

    return sign * (int_part * (long)scale + frac_part);
}

/* parse_uint16_clamped:
 *  - Parses an unsigned integer and clamps to uint16_t if necessary.
 */
uint16_t parse_uint16_clamped(const char *s)
{
    unsigned long v = 0;
    if (!s) return 0;
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (unsigned)(*s - '0');
        s++;
    }
    if (v > 0xFFFFUL) v = 0xFFFFUL;
    return (uint16_t)v;
}

/* parse_percent:
 *  - Parses a number and returns -1 if it's out of 0..100 range.
 */
int parse_percent(const char *s)
{
    int v = 0;
    if (!s) return -1;
    if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (*s - '0');
        s++;
        if (v > 100) return -1; // early out
    }
    return v;
}
