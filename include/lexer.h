#if !defined(HGVS_LEXER_H)
#define HGVS_LEXER_H


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


static size_t const MAX_NUMBER     = SIZE_MAX / 10 - 10;
static size_t const INVALID_NUMBER = -1;


static inline size_t
to_integer(char const ch)
{
    if (ch == '1') return 1;
    if (ch == '2') return 2;
    if (ch == '3') return 3;
    if (ch == '4') return 4;
    if (ch == '5') return 5;
    if (ch == '6') return 6;
    if (ch == '7') return 7;
    if (ch == '8') return 8;
    if (ch == '9') return 9;
    return 0;
} // to_integer


static inline bool
is_decimal_digit(char const ch)
{
    return ch == '0' || ch == '1' || ch == '2' || ch == '3' ||
           ch == '4' || ch == '5' || ch == '6' || ch == '7' ||
           ch == '8' || ch == '9';
} // is decimal_digit


static inline bool
is_IUPAC_DNA(char const ch)
{
    return ch == 'A' || ch == 'C' || ch == 'G' || ch == 'T' ||
           ch == 'R' || ch == 'Y' || ch == 'S' || ch == 'W' ||
           ch == 'K' || ch == 'M' || ch == 'B' || ch == 'D' ||
           ch == 'H' || ch == 'V' || ch == 'N';
} // is_IUPAC_DNA


static inline bool
is_alpha(const char ch)
{
    return ch == 'A' || ch == 'B' || ch == 'C' || ch == 'D' ||
           ch == 'E' || ch == 'F' || ch == 'G' || ch == 'H' ||
           ch == 'I' || ch == 'J' || ch == 'K' || ch == 'L' ||
           ch == 'M' || ch == 'N' || ch == 'O' || ch == 'P' ||
           ch == 'Q' || ch == 'R' || ch == 'S' || ch == 'T' ||
           ch == 'U' || ch == 'V' || ch == 'W' || ch == 'X' ||
           ch == 'Y' || ch == 'Z' ||
           ch == 'a' || ch == 'b' || ch == 'c' || ch == 'd' ||
           ch == 'e' || ch == 'f' || ch == 'g' || ch == 'h' ||
           ch == 'i' || ch == 'j' || ch == 'k' || ch == 'l' ||
           ch == 'm' || ch == 'n' || ch == 'o' || ch == 'p' ||
           ch == 'q' || ch == 'r' || ch == 's' || ch == 't' ||
           ch == 'u' || ch == 'v' || ch == 'w' || ch == 'x' ||
           ch == 'y' || ch == 'z';
} // is_alpha


static inline bool
is_alphanumeric(const char ch)
{
    return is_alpha(ch) || is_decimal_digit(ch);
} // is_alphanumeric


static inline bool
match_alpha(char const** const ptr, size_t* ch)
{
    if (is_alpha(**ptr))
    {
        *ch = **ptr;
        *ptr += 1;
        return true;
    } // if
    return false;
} // match_alpha


static inline bool
match_char(char const** const ptr, char const ch)
{
    if (**ptr == ch)
    {
        *ptr += 1;
        return true;
    } // if
    return false;
} // match_char


static inline bool
match_number(char const** const ptr, size_t* num)
{
    *num = 0;
    bool matched = false;
    while (is_decimal_digit(**ptr))
    {
        matched = true;
        if (*num < MAX_NUMBER)
        {
            *num *= 10;
            *num += to_integer(**ptr);
        } // if
        else
        {
            *num = INVALID_NUMBER;
        } // else
        *ptr += 1;
    } // while
    return matched;
} // match_number


static inline bool
match_sequence(char const** const ptr, size_t* len)
{
    *len = 0;
    bool matched = false;
    while (is_IUPAC_DNA(**ptr))
    {
        matched = true;
        *ptr += 1;
        *len += 1;
    } // while
    return matched;
} // match_sequence


static inline bool
match_identifier(char const** const ptr, size_t* len)
{
    *len = 0;
    bool matched = false;
    if (is_alpha(**ptr))
    {
        matched = true;
        while (is_alphanumeric(**ptr) || **ptr == '.' || **ptr == '_')
        {
            *ptr += 1;
            *len += 1;
        } // while
    } // if
    return matched;
} // match_identifier


static inline bool
match_string(char const** const ptr, char const* str)
{
    while (*str != '\0' && *str == **ptr)
    {
        str += 1;
        *ptr += 1;
    } // while
    return *str == '\0';
} // match_string


#endif
