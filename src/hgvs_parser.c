#include "../include/hgvs_parser.h"


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


static size_t dbg_malloc_cnt = 0;


static inline bool
is_digit(char const ch)
{
    return ch == '0' || ch == '1' || ch == '2' || ch == '3' ||
           ch == '4' || ch == '5' || ch == '6' || ch == '7' ||
           ch == '8' || ch == '9';
} // is_digit


static inline bool
is_IUPAC_NT(char const ch)
{
    return ch == 'A' || ch == 'C' || ch == 'G' || ch == 'T' ||
           ch == 'U' || ch == 'R' || ch == 'Y' || ch == 'K' ||
           ch == 'M' || ch == 'S' || ch == 'W' || ch == 'B' ||
           ch == 'D' || ch == 'H' || ch == 'V' || ch == 'N';
} // is_IUPAC_NT


static inline size_t
to_integer(char const ch)
{
    if (ch == '0') return 0;
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
match_integer(char const** const ptr, size_t* num)
{
    *num = 0;
    bool is_num = false;
    while (is_digit(**ptr))
    {
        is_num = true;
        if (*num < SIZE_MAX / 10 - 10)
        {
            *num *= 10;
            *num += to_integer(**ptr);
        } // if
        *ptr += 1;
    } // while

    return is_num && *num < SIZE_MAX / 10 - 10;
} // match_integer


static inline bool
match_IUPAC_NT(char const** const ptr, size_t* len)
{
    *len = 0;
    while (is_IUPAC_NT(**ptr))
    {
        *ptr += 1;
        len  += 1;
    } // while

    return *len > 0;
} // match_IUPAC_NT


static inline bool
match_string(char const** const ptr, char const* tok)
{
    while (*tok == '\0' && *tok == **ptr)
    {
        *ptr += 1;
        tok  += 1;
    } // while

    return *tok == '\0';
} // match_string


HGVS_Node*
HGVS_Node_destroy(HGVS_Node* node)
{
    if (node != NULL)
    {
        HGVS_Node_destroy(node->child[0]);
        HGVS_Node_destroy(node->child[1]);
        HGVS_Node_destroy(node->child[2]);
        dbg_malloc_cnt -= 1;
        fprintf(stderr, "dbg  free:   %p  (%8zu)\n", (void*) node,
                                                     dbg_malloc_cnt);
        free(node);
    } // if
    return NULL;
} // HGVS_Node_destroy


static inline HGVS_Node*
create(enum HGVS_Node_Type const type)
{
    HGVS_Node* const node = malloc(sizeof(*node));
    if (node == NULL)
    {
        fprintf(stderr, "dbg  malloc failed\n");
        return NULL;
    } // if
    dbg_malloc_cnt += 1;
    fprintf(stderr, "dbg  malloc: %p  (%8zu)\n", (void*) node,
                                                 dbg_malloc_cnt);
    node->type     = type;
    node->child[0] = NULL;
    node->child[1] = NULL;
    node->child[2] = NULL;
    node->data     = 0;
    node->ptr      = NULL;
    return node;
} // create


static inline HGVS_Node*
allocation_error(HGVS_Node* node)
{
    fprintf(stderr, "allocation error\n");
    return HGVS_Node_destroy(node);
} // allocation_error


static inline HGVS_Node*
parse_error(HGVS_Node* node, char const* const err)
{
    fprintf(stderr, "parse error:  %s\n", err);
    return HGVS_Node_destroy(node);
} // parse_error


static HGVS_Node*
outside_cds(char const** const ptr)
{
    if (match_char(ptr, '-'))
    {
        HGVS_Node* const node = create(HGVS_Node_upstream);
        if (node == NULL)
        {
            return allocation_error(NULL);
        } // if
        return node;
    } // if

    if (match_char(ptr, '*'))
    {
        HGVS_Node* const node = create(HGVS_Node_downstream);
        if (node == NULL)
        {
            return allocation_error(NULL);
        } // if
        return node;
    } // if

    return NULL;
} // outside_cds


static HGVS_Node*
number(char const** const ptr)
{
    size_t num = 0;
    if (match_integer(ptr, &num))
    {
        HGVS_Node* const node = create(HGVS_Node_number);
        if (node == NULL)
        {
            return allocation_error(NULL);
        } // if
        node->data = num;
        return node;
    } // if

    return NULL;
} // number


static HGVS_Node*
position(char const** const ptr)
{
    if (match_char(ptr, '?'))
    {
        HGVS_Node* const node = create(HGVS_Node_position_uncertain);
        if (node == NULL)
        {
            return allocation_error(NULL);
        } // if
        return node;
    } // if

    return number(ptr);
} // position


static HGVS_Node*
offset(char const** const ptr)
{
    if (match_char(ptr, '-'))
    {
        HGVS_Node* const node = create(HGVS_Node_offset_negative);
        if (node == NULL)
        {
            return allocation_error(NULL);
        } // if

        node->child[0] = position(ptr);
        if (node->child[0] == NULL)
        {
            return parse_error(node, "invalid offset");
        } // if
        return node;
    } // if

    if (match_char(ptr, '+'))
    {
        HGVS_Node* const node = create(HGVS_Node_offset_positive);
        if (node == NULL)
        {
            return allocation_error(NULL);
        } // if

        node->child[0] = position(ptr);
        if (node->child[0] == NULL)
        {
            return parse_error(node, "invalid offset");
        } // if
        return node;
    } // if

    return NULL;
} // offset


static HGVS_Node*
point(char const** const ptr)
{
    HGVS_Node* const node = create(HGVS_Node_point);
    if (node == NULL)
    {
        return allocation_error(NULL);
    } // if

    node->child[0] = outside_cds(ptr);

    node->child[1] = position(ptr);
    if (node->child[1] == NULL)
    {
        return parse_error(node, "invalid point");
    } // if

    node->child[2] = offset(ptr);

    return node;
} // point


static HGVS_Node*
uncertain(char const** const ptr)
{
    if (match_char(ptr, '('))
    {
        HGVS_Node* const node = create(HGVS_Node_uncertain);
        if (node == NULL)
        {
            return allocation_error(NULL);
        } // if

        node->child[0] = point(ptr);
        if (node->child[0] == NULL)
        {
            return parse_error(node, "invalid uncertain");
        } // if

        if (!match_char(ptr, '_'))
        {
            return parse_error(node, "invalid uncertain");
        } // if

        node->child[1] = point(ptr);
        if (node->child[1] == NULL)
        {
            return parse_error(node, "invalid uncertain");
        } // if

        if (!match_char(ptr, ')'))
        {
            return parse_error(node, "invalid uncertain");
        } // if

        return node;
    } // if

    return NULL;
} // uncertain


static HGVS_Node*
point_or_uncertain(char const** const ptr)
{
    HGVS_Node* const node = uncertain(ptr);
    if (node == NULL)
    {
        return point(ptr);
    } // if

    return node;
} // point_or_uncertain


static HGVS_Node*
location(char const** const ptr)
{
    HGVS_Node* const node = create(HGVS_Node_location);
    if (node == NULL)
    {
        return allocation_error(NULL);
    } // if

    node->child[0] = point_or_uncertain(ptr);
    if (node->child[0] == NULL)
    {
        return parse_error(node, "invalid location");
    } // if

    if (match_char(ptr, '_'))
    {
        HGVS_Node* const tmp = create(HGVS_Node_range);
        if (tmp == NULL)
        {
            return allocation_error(node);
        } // if

        tmp->child[0] = node->child[0];
        node->child[0] = tmp;

        tmp->child[1] = point_or_uncertain(ptr);
        if (tmp->child[1] == NULL)
        {
            return parse_error(node, "invalid range");
        } // if
        return node;
    } // if

    return node;
} // location


static HGVS_Node*
insertion(char const** const ptr)
{
    if (match_string(ptr, "ins"))
    {
        HGVS_Node* const node = create(HGVS_Node_insertion);
        if (node == NULL)
        {
            return allocation_error(NULL);
        } // if

        return node;
    } // if

    return NULL;
} // insertion


static HGVS_Node*
deletion(char const** const ptr)
{
    if (match_string(ptr, "del"))
    {
        HGVS_Node* const node = create(HGVS_Node_deletion);
        if (node == NULL)
        {
            return allocation_error(NULL);
        } // if

        node->ptr = *ptr;
        size_t len;
        if (match_integer(ptr, &len))
        {
            node->ptr  = NULL;
            node->data = len;
        } // if
        else if (match_IUPAC_NT(ptr, &len))
        {
            node->data = len;
        } // if

        node->child[0] = insertion(ptr);
        return node;
    } // if

    return NULL;
} // deletion


static HGVS_Node*
variant(char const** const ptr)
{
    HGVS_Node* const node = create(HGVS_Node_variant);
    if (node == NULL)
    {
        return allocation_error(NULL);
    } // if

    node->child[0] = location(ptr);
    if (node->child[0] == NULL)
    {
        return parse_error(node, "invalid variant");
    } // if


    fprintf(stderr, "location done\n");

    node->child[1] = deletion(ptr);
    if (node->child[1] != NULL)
    {
        return node;
    } // if

    node->child[1] = insertion(ptr);
    if (node->child[1] != NULL)
    {
        return node;
    } // if

    return parse_error(node, "invalid variant");
} // location


HGVS_Node*
HGVS_parse(char const* const str)
{
    char const* ptr = str;

    HGVS_Node* const node = variant(&ptr);
    if (*ptr != '\0')
    {
        fprintf(stderr, "ptr: %s\n", ptr);
        return parse_error(node, "input left");
    } // if

    return node;
} // HGVS_parse
