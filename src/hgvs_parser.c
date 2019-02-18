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
    char const* p = *ptr;
    while (is_digit(*p))
    {
        is_num = true;
        if (*num < SIZE_MAX / 10 - 10)
        {
            *num *= 10;
            *num += to_integer(**ptr);
        } // if
        p += 1;
    } // while

    if (is_num && *num < SIZE_MAX / 10 - 10)
    {
        *ptr = p;
        return true;
    } // if
    return false;
} // match_integer


static inline bool
match_IUPAC_NT(char const** const ptr, size_t* len)
{
    *len = 0;
    char const* p = *ptr;
    while (is_IUPAC_NT(*p))
    {
        p += 1;
        len  += 1;
    } // while

    if (*len > 0)
    {
        *ptr = p;
        return true;
    } // if
    return false;
} // match_IUPAC_NT


static inline bool
match_string(char const** const ptr, char const* tok)
{
    char const* p = *ptr;
    while (*tok != '\0' && *tok == *p)
    {
        p += 1;
        tok  += 1;
    } // while

    if (*tok == '\0')
    {
        *ptr = p;
        return true;
    } // if
    return false;
} // match_string


HGVS_Node*
HGVS_Node_destroy(HGVS_Node* node)
{
    if (node != NULL)
    {
        HGVS_Node_destroy(node->left);
        HGVS_Node_destroy(node->right);
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
        return NULL;
    } // if
    dbg_malloc_cnt += 1;
    fprintf(stderr, "dbg  malloc: %p  (%8zu)\n", (void*) node,
                                                 dbg_malloc_cnt);
    node->type  = type;
    node->left  = NULL;
    node->right = NULL;
    node->ptr   = NULL;
    node->data.value = 0;
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
        node->data.value = num;
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

        node->left = position(ptr);
        if (node->left == NULL)
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

        node->left = position(ptr);
        if (node->left == NULL)
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

    if (match_char(ptr, '-'))
    {
        node->type = HGVS_Node_point_upstream;
    } // if
    else if (match_char(ptr, '*'))
    {
        node->type = HGVS_Node_point_downstream;
    }

    node->left = position(ptr);
    if (node->left == NULL)
    {
        return parse_error(node, "invalid point");
    } // if

    node->right = offset(ptr);

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

        node->left = point(ptr);
        if (node->left == NULL)
        {
            return parse_error(node, "invalid uncertain");
        } // if

        if (!match_char(ptr, '_'))
        {
            return parse_error(node, "invalid uncertain");
        } // if

        node->right = point(ptr);
        if (node->right == NULL)
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
range(char const** const ptr)
{
    HGVS_Node* const node = create(HGVS_Node_range);
    if (node == NULL)
    {
        return allocation_error(NULL);
    } // if

    node->left = point_or_uncertain(ptr);
    if (node->left == NULL)
    {
        return parse_error(node, "invalid range");
    } // if

    if (!match_char(ptr, '_'))
    {
        return parse_error(node, "invalid range");
    } // if

    node->right = point_or_uncertain(ptr);
    if (node->left == NULL)
    {
        return parse_error(node, "invalid range");
    } // if

    return node;
} // range


static HGVS_Node*
location(char const** const ptr)
{
    HGVS_Node* const node = create(HGVS_Node_location);
    if (node == NULL)
    {
        return allocation_error(NULL);
    } // if

    node->left = point_or_uncertain(ptr);
    if (node->left == NULL)
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

        tmp->left = node->left;
        node->left = tmp;

        tmp->right = point_or_uncertain(ptr);
        if (tmp->right == NULL)
        {
            return parse_error(node, "invalid range");
        } // if
        return node;
    } // if

    return node;
} // location


static HGVS_Node*
inserted(char const** const ptr)
{
    HGVS_Node* const node = create(HGVS_Node_inserted);
    if (node == NULL)
    {
        return allocation_error(NULL);
    } // if

    node->left = range(ptr);
    if (node->left == NULL)
    {
        node->ptr = *ptr;
        size_t len = 0;
        if (match_IUPAC_NT(ptr, &len))
        {
            node->data.value = len;
            return node;
        } // if

        return parse_error(node, "invalid inserted");
    } // if

    if (match_string(ptr, "inv"))
    {
        node->data.value = INVERTED;
    } // if

    return node;
} // inserted


static HGVS_Node*
repeated_length(char const** const ptr)
{
    if (match_char(ptr, '['))
    {
        HGVS_Node* const node = create(HGVS_Node_repeated);
        if (node == NULL)
        {
            return allocation_error(NULL);
        } // if

        node->left = number(ptr);
        if (node->left == NULL)
        {
            return parse_error(node, "invalid repeat");
        } // if

        if (!match_char(ptr, ']'))
        {
            return parse_error(node, "invalid repeat");
        } // if

        return node;
    } // if

    return NULL;
} // repeated_length


static HGVS_Node*
repeated(char const** const ptr)
{
    if (match_char(ptr, '('))
    {
        HGVS_Node* const node = create(HGVS_Node_repeated_range);
        if (node == NULL)
        {
            return allocation_error(NULL);
        } // if

        node->left = number(ptr);
        if (node->left == NULL)
        {
            return parse_error(node, "invalid repeated range");
        } // if

        if (!match_char(ptr, '_'))
        {
            return parse_error(node, "invalid repeated range");
        } // if

        node->right = number(ptr);
        if (node->right == NULL)
        {
            return parse_error(node, "invalid repeated range");
        } // if

        return node;
    } // if

    return repeated_length(ptr);
} // repeated


static HGVS_Node*
substitution(char const** const ptr)
{
    char const deleted = **ptr;
    size_t len = 0;
    if (match_IUPAC_NT(ptr, &len))
    {
        if (match_char(ptr, '>'))
        {
            if (len != 1)
            {
                return parse_error(NULL, "invalid substution");
            } // if

            HGVS_Node* const node = create(HGVS_Node_substitution);
            if (node == NULL)
            {
                return allocation_error(NULL);
            } // if

            node->data.substitution.deleted = deleted;
            node->data.substitution.inserted = **ptr;

            len = 0;
            if (!match_IUPAC_NT(ptr, &len))
            {
                return parse_error(node, "invalid substitution");
            } // if

            if (len != 1)
            {
                return parse_error(node, "invalid substitution");
            } // if

            return node;
        } // if

        return repeated(ptr);
    } // if

    return NULL;
} // substitution


static HGVS_Node*
inserted_compound(char const** const ptr)
{
    if (match_char(ptr, '['))
    {
        HGVS_Node* const node = inserted(ptr);
        if (node == NULL)
        {
            return parse_error(NULL, "invalid inserted compound");
        } // if

        HGVS_Node* tmp = node;
        while (match_char(ptr, ';'))
        {
            tmp->right = inserted(ptr);
            if (tmp->right == NULL)
            {
                return parse_error(node, "invalid inserted compound");
            } // if
            tmp = tmp->right;
        } // while

        return node;
    } // if

    return NULL;
} // inserted_compound


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

        node->left = inserted_compound(ptr);
        if (node->left == NULL)
        {
            node->left = inserted(ptr);
            if (node->left == NULL)
            {
                return parse_error(node, "invalid insertion");
            } // if

            return node;
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
            node->ptr = NULL;
            node->data.value = len;
        } // if
        else if (match_IUPAC_NT(ptr, &len))
        {
            node->data.value = len;
        } // if

        node->left = insertion(ptr);
        if (node->left != NULL)
        {
            node->type = HGVS_Node_deletion_insertion;
        } // if

        return node;
    } // if

    return NULL;
} // deletion


static HGVS_Node*
duplication(char const** const ptr)
{
    if (match_string(ptr, "dup"))
    {
        HGVS_Node* const node = create(HGVS_Node_duplication);
        if (node == NULL)
        {
            return allocation_error(NULL);
        } // if

        node->ptr = *ptr;
        size_t len;
        if (match_integer(ptr, &len))
        {
            node->ptr = NULL;
            node->data.value = len;
        } // if
        else if (match_IUPAC_NT(ptr, &len))
        {
            node->data.value = len;
        } // if

        return node;
    } // if

    return NULL;
} // duplication


static HGVS_Node*
inversion(char const** const ptr)
{
    if (match_string(ptr, "inv"))
    {
        HGVS_Node* const node = create(HGVS_Node_inversion);
        if (node == NULL)
        {
            return allocation_error(NULL);
        } // if

        node->ptr = *ptr;
        size_t len;
        if (match_integer(ptr, &len))
        {
            node->ptr = NULL;
            node->data.value = len;
        } // if
        else if (match_IUPAC_NT(ptr, &len))
        {
            node->data.value = len;
        } // if

        return node;
    } // if

    return NULL;
} // inversion


static HGVS_Node*
conversion(char const** const ptr)
{
    if (match_string(ptr, "con"))
    {
        HGVS_Node* const node = create(HGVS_Node_conversion);
        if (node == NULL)
        {
            return allocation_error(NULL);
        } // if

        node->left = range(ptr);
        if (node->left == NULL)
        {
            return parse_error(node, "invalid conversion");
        } // if

        return node;
    } // if

    return NULL;
} // conversion


static HGVS_Node*
equal(char const** const ptr)
{
    if (match_char(ptr, '='))
    {
        HGVS_Node* const node = create(HGVS_Node_equal);
        if (node == NULL)
        {
            return allocation_error(NULL);
        } // if

        return node;
    } // if

    return NULL;
} // equal


static HGVS_Node*
variant(char const** const ptr)
{
    HGVS_Node* const node = create(HGVS_Node_variant);
    if (node == NULL)
    {
        return allocation_error(NULL);
    } // if

    node->left = location(ptr);
    if (node->left == NULL)
    {
        return parse_error(node, "invalid variant");
    } // if

    node->right = substitution(ptr);
    if (node->right != NULL)
    {
        if (node->right->type == HGVS_Node_substitution &&
            (node->left->type != HGVS_Node_point ||
             node->left->type != HGVS_Node_uncertain))
        {
            return parse_error(node, "invalid substitution");
        } // if

        if (node->right->type == HGVS_Node_repeated &&
            node->left->type != HGVS_Node_point)
        {
            return parse_error(node, "invalid repeat");
        } // if

        return node;
    } // if

    node->right = deletion(ptr);
    if (node->right != NULL)
    {
        return node;
    } // if

    if (node->left->type == HGVS_Node_range)
    {
        node->right = insertion(ptr);
        if (node->right != NULL)
        {
            return node;
        } // if
    } // if

    node->right = duplication(ptr);
    if (node->right != NULL)
    {
        return node;
    } // if

    if (node->left->type == HGVS_Node_range)
    {
        node->right = inversion(ptr);
        if (node->right != NULL)
        {
            return node;
        } // if
    } // if

    if (node->left->type == HGVS_Node_range)
    {
        node->right = conversion(ptr);
        if (node->right != NULL)
        {
            return node;
        } // if
    } // if

    if (node->left->type == HGVS_Node_point ||
        node->left->type == HGVS_Node_range)
    {
        node->right = equal(ptr);
        if (node->right != NULL)
        {
            return node;
        } // if
    } // if

    if (node->left->type == HGVS_Node_range)
    {
        node->right = repeated_length(ptr);
        if (node->right != NULL)
        {
            return node;
        } // if
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
