#include "../include/hgvs_parser.h"


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


static size_t const MAX_INTEGER = SIZE_MAX / 10 - 10;


static HGVS_Node allocation_error_msg =
{
    .left  = NULL,
    .right = NULL,
    .type  = HGVS_Node_error_message,
    .data  = 0,
    .ptr   = "allocation error"
}; // allocation_error_msg


static HGVS_Node allocation_error =
{
    .left  = &allocation_error_msg,
    .right = NULL,
    .type  = HGVS_Node_error_allocation,
    .data  = 0,
    .ptr   = NULL
}; // allocation_error


static inline bool
is_IUPAC_NT(char const ch)
{
    return ch == 'A' || ch == 'C' || ch == 'G' || ch == 'T' ||
           ch == 'U' || ch == 'R' || ch == 'Y' || ch == 'K' ||
           ch == 'M' || ch == 'S' || ch == 'W' || ch == 'B' ||
           ch == 'D' || ch == 'H' || ch == 'V' || ch == 'N';
} // is_IUPAC_NT


static inline bool
is_decimal_digit(char const ch)
{
    return ch == '0' || ch == '1' || ch == '2' || ch == '3' ||
           ch == '4' || ch == '5' || ch == '6' || ch == '7' ||
           ch == '8' || ch == '9';
} // is_decimal_digit


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
match_char(char const** const str, char const ch)
{
    if (**str == ch)
    {
        *str += 1;
        return true;
    } // if

    return false;
} // match_char


static inline bool
match_integer(char const** const str, size_t* num)
{
    bool is_num = false;
    *num = 0;
    char const* ptr = *str;

    while (is_decimal_digit(*ptr))
    {
        is_num = true;
        if (*num < SIZE_MAX / 10 - 10)
        {
            *num *= 10;
            *num += to_integer(*ptr);
        } // if
        ptr += 1;
    } // while

    if (is_num)
    {
        *str = ptr;

        if (*num < MAX_INTEGER)
        {
            return true;
        } // if

        *num = -1;
        return true;
    } // if

    return false;
} // match_integer


static inline bool
match_IUPAC_NT(char const** const str, size_t* len)
{
    *len = 0;
    char const* ptr = *str;
    while (is_IUPAC_NT(*ptr))
    {
        ptr += 1;
        *len += 1;
    } // while

    if (*len > 0)
    {
        *str = ptr;
        return true;
    } // if
    return false;
} // match_IUPAC_NT


static inline bool
match_string(char const** const str, char const* tok)
{
    char const* ptr = *str;
    while (*tok != '\0' && *tok == *ptr)
    {
        ptr += 1;
        tok  += 1;
    } // while

    if (*tok == '\0')
    {
        *str = ptr;
        return true;
    } // if
    return false;
} // match_string


static inline bool
is_error_allocation(HGVS_Node const* const node)
{
    return node != NULL && node->type == HGVS_Node_error_allocation;
} // is_error_allocation


static inline bool
is_error(HGVS_Node const* const node)
{
    return node != NULL && (node->type == HGVS_Node_error_allocation ||
                            node->type == HGVS_Node_error);
} // is_error


static inline bool
is_unmatched(HGVS_Node const* const node)
{
    return node == NULL;
} // is_unmatched


void
HGVS_Node_destroy(HGVS_Node* node)
{
    if (node != NULL && node->type != HGVS_Node_error_allocation)
    {
        HGVS_Node_destroy(node->left);
        HGVS_Node_destroy(node->right);
        free(node);
    } // if
} // HGVS_Node_destroy


static HGVS_Node*
error_allocation(HGVS_Node* node)
{
    HGVS_Node_destroy(node);
    return &allocation_error;
} // error_allocation


static HGVS_Node*
create(enum HGVS_Node_Type const type)
{
    HGVS_Node* const node = malloc(sizeof(*node));
    if (node == NULL)
    {
        return error_allocation(NULL);
    } // if

    node->left  = NULL;
    node->right = NULL;

    node->type = type;
    node->data = 0;
    node->ptr  = NULL;

    return node;
} // create


static HGVS_Node*
error(HGVS_Node*        node,
      HGVS_Node* const  next,
      char const* const ptr,
      char const* const msg)
{
    HGVS_Node_destroy(node);

    node = create(HGVS_Node_error);
    if (is_error_allocation(node))
    {
        return error_allocation(next);
    } // if

    node->right = next;
    node->left = create(HGVS_Node_error_message);
    if (is_error_allocation(node))
    {
        return error_allocation(node);
    } // if
    node->left->ptr = msg;

    node->ptr = ptr;

    return node;
} // error


static HGVS_Node*
unmatched(HGVS_Node* node)
{
    HGVS_Node_destroy(node);
    return NULL;
} // unmatched


static HGVS_Node*
number(char const** const str)
{
    size_t num = 0;
    char const* ptr = *str;
    if (match_integer(&ptr, &num))
    {
        HGVS_Node* const node = create(HGVS_Node_number);
        if (is_error_allocation(node))
        {
            return error_allocation(NULL);
        } // if

        node->data = num;

        *str = ptr;
        return node;
    } // if

    return unmatched(NULL);
} // number


static HGVS_Node*
unknown(char const** const str)
{
    char const* ptr = *str;
    if (match_char(&ptr, '?'))
    {
        HGVS_Node* const node = create(HGVS_Node_unknown);
        if (is_error_allocation(node))
        {
            return error_allocation(NULL);
        } // if

        *str = ptr;
        return node;
    } // if

    return unmatched(NULL);
} // unknown


static HGVS_Node*
number_or_unknown(char const** const str)
{
    char const* ptr = *str;
    HGVS_Node* node = unknown(&ptr);
    if (is_error(node))
    {
        return error(NULL, node, *str, "while matching a number or unknown ('?')");
    } // if
    if (is_unmatched(node))
    {
        node = number(&ptr);
        if (is_error(node))
        {
            return error(NULL, node, *str, "while matching a number or unknown ('?')");
        } // if
        if (is_unmatched(node))
        {
            return error(NULL, NULL, *str, "expected a number or unknown ('?')");
        } // if

        *str = ptr;
        return node;
    } // if

    *str = ptr;
    return node;
} // number_or_unknown


static HGVS_Node*
position(char const** const str)
{
    char const* ptr = *str;
    HGVS_Node* const probe = number_or_unknown(&ptr);
    if (is_error(probe) ||
        is_unmatched(probe))
    {
        return error(NULL, probe, *str, "while matching a position");
    } // if

    HGVS_Node* const node = create(HGVS_Node_position);
    if (is_error_allocation(node))
    {
        return error_allocation(NULL);
    } // if

    node->left = probe;

    *str = ptr;
    return node;
} // position


static HGVS_Node*
offset(char const** const str)
{
    char const* ptr = *str;
    if (match_char(&ptr, '-'))
    {
        HGVS_Node* const node = create(HGVS_Node_offset);
        if (is_error_allocation(node))
        {
            return error_allocation(NULL);
        } // if

        node->data = HGVS_Node_negative;

        HGVS_Node* const probe = number_or_unknown(&ptr);
        if (is_error(probe) ||
            is_unmatched(probe))
        {
            return error(node, probe, *str, "while matching an offset (-)");
        } // if

        node->left = probe;

        *str = ptr;
        return node;
    } // if

    if (match_char(&ptr, '+'))
    {
        HGVS_Node* const node = create(HGVS_Node_offset);
        if (is_error(node))
        {
            return error_allocation(NULL);
        } // if

        node->data = HGVS_Node_positive;

        HGVS_Node* const probe = number_or_unknown(&ptr);
        if (is_error(probe) ||
            is_unmatched(probe))
        {
            return error(node, probe, *str, "while matching an offset (+)");
        } // if

        node->left = probe;

        *str = ptr;
        return node;
    } // if

    return unmatched(NULL);
} // offset


static HGVS_Node*
point(char const** const str)
{
    HGVS_Node* const node = create(HGVS_Node_point);
    if (is_error_allocation(node))
    {
        return error_allocation(NULL);
    } // if

    char const* ptr = *str;
    if (match_char(&ptr, '-'))
    {
        node->data = HGVS_Node_downstream;
    } // if
    else if (match_char(&ptr, '*'))
    {
        node->data = HGVS_Node_upstream;
    } // if

    HGVS_Node* probe = position(&ptr);
    if (is_error(probe) ||
        is_unmatched(probe))
    {
        return error(node, probe, *str, "while matching a point");
    } // if

    node->left = probe;

    probe = offset(&ptr);
    if (is_error(probe))
    {
        return error(node, probe, *str, "while matching a point");
    } // if

    node->right = probe;

    *str = ptr;
    return node;
} // point


static HGVS_Node*
uncertain(char const** const str)
{
    char const* ptr = *str;
    if (match_char(&ptr, '('))
    {
        HGVS_Node* const node = create(HGVS_Node_uncertain);
        if (is_error_allocation(node))
        {
            return error_allocation(NULL);
        } // if

        HGVS_Node* probe = point(&ptr);
        if (is_error(probe) ||
            is_unmatched(probe))
        {
            return error(node, probe, *str, "while matching an uncertain");
        } // if

        node->left = probe;

        if (!match_char(&ptr, '_'))
        {
            return error(node, NULL, ptr, "invalid uncertain: expected '_'");
        } // if

        probe = point(&ptr);
        if (is_error(probe) ||
            is_unmatched(probe))
        {
            return error(node, probe, *str, "while matching an uncertain");
        } // if

        node->right = probe;

        if (!match_char(&ptr, ')'))
        {
            return error(node, NULL, ptr, "invalid uncertain: expected ')'");
        } // if

        *str = ptr;
        return node;
    } // if

    return unmatched(NULL);
} // uncertain


static HGVS_Node*
point_or_uncertain(char const** const str)
{
    char const* ptr = *str;
    HGVS_Node* node = uncertain(&ptr);
    if (is_error(node))
    {
        return error(NULL, node, *str, "while matching a point or uncertain");
    } // if
    if (is_unmatched(node))
    {
        node = point(&ptr);
        if (is_error(node))
        {
            return error(NULL, node, *str, "while matching a point or uncertain");
        } // if
        if (is_unmatched(node))
        {
            return error(NULL, NULL, ptr, "expected point or uncertain");
        } // if

        *str = ptr;
        return node;
    } // if

    *str = ptr;
    return node;
} // point_or_uncertain


static HGVS_Node*
location(char const** const str)
{
    char const* ptr = *str;
    HGVS_Node* probe = point_or_uncertain(&ptr);
    if (is_error(probe) ||
        is_unmatched(probe))
    {
        return error(NULL, probe, *str, "while matching a location");
    } // if

    if (match_char(&ptr, '_'))
    {
        HGVS_Node* const node = create(HGVS_Node_range);
        if (is_error_allocation(node))
        {
            return error_allocation(probe);
        } // if

        node->left = probe;

        probe = point_or_uncertain(&ptr);
        if (is_error(probe) ||
            is_unmatched(probe))
        {
            return error(node, probe, *str, "while matching a range location");
        } // if

        node->right = probe;

        *str = ptr;
        return node;
    } // if

    *str = ptr;
    return probe;
} // location


static HGVS_Node*
sequence(char const** const str)
{
    char const* ptr = *str;
    size_t len = 0;
    if (match_IUPAC_NT(&ptr, &len))
    {
        HGVS_Node* const node = create(HGVS_Node_sequence);
        if (is_error_allocation(node))
        {
            return error_allocation(NULL);
        } // if

        node->data = len;
        node->ptr  = *str;

        *str = ptr;
        return node;
    } // if

    return unmatched(NULL);
} // sequence


static HGVS_Node*
sequence_or_number(char const** const str)
{
    char const* ptr = *str;
    if (is_decimal_digit(*ptr))
    {
        HGVS_Node* const node = number(&ptr);
        if (is_error(node))
        {
            return error(NULL, node, *str, "while matching a sequence or number");
        } // if

        *str = ptr;
        return node;
    } // if

    HGVS_Node* const node = sequence(&ptr);
    if (is_error(node))
    {
        return error(NULL, node, *str, "while matching a sequence or number");
    } // if

    *str = ptr;
    return node;
} // sequence_or_number


static HGVS_Node*
duplication(char const** const str)
{
    char const* ptr = *str;
    if (match_string(&ptr, "dup"))
    {
        HGVS_Node* const node = create(HGVS_Node_duplication);
        if (is_error_allocation(node))
        {
            return error_allocation(NULL);
        } // if

        HGVS_Node* const probe = sequence_or_number(&ptr);
        if (is_error(probe))
        {
            return error(node, probe, *str, "while matching a duplication");
        } // if

        node->left = probe;

        *str = ptr;
        return node;
    } // if

    return unmatched(NULL);
} // duplication


static HGVS_Node*
inversion(char const** const str)
{
    char const* ptr = *str;
    if (match_string(&ptr, "inv"))
    {
        HGVS_Node* const node = create(HGVS_Node_inversion);
        if (is_error_allocation(node))
        {
            return error_allocation(NULL);
        } // if

        HGVS_Node* const probe = sequence_or_number(&ptr);
        if (is_error(probe))
        {
            return error(node, probe, *str, "while matching an inversion");
        } // if

        node->left = probe;

        *str = ptr;
        return node;
    } // if

    return unmatched(NULL);
} // inversion


static HGVS_Node*
substitution_or_repeat(char const** const str)
{
    char const* ptr = *str;
    HGVS_Node* probe = sequence(&ptr);
    if (is_error(probe))
    {
        return error(NULL, probe, *str, "while matching a substitution or repeat");
    } // if
    if (is_unmatched(probe))
    {
        return unmatched(NULL);
    } // if

    if (match_char(&ptr, '>'))
    {
        HGVS_Node* const node = create(HGVS_Node_substitution);
        if (is_error_allocation(node))
        {
            return error_allocation(probe);
        } // if

        node->left = probe;

        probe = sequence(&ptr);
        if (is_error(probe) ||
            is_unmatched(probe))
        {
            return error(node, probe, *str, "while matching a substitution");
        } // if

        node->right = probe;

        *str = ptr;
        return node;
    } // if

    return unmatched(NULL); // ToDo: add repeat part
} // substitution_or_repeat


static HGVS_Node*
inserted(char const** const str)
{
    return unmatched(NULL);
} // inserted


static HGVS_Node*
deletion_or_deletion_insertion(char const** const str)
{
    char const* ptr = *str;
    if (match_string(&ptr, "del"))
    {
        HGVS_Node* probe = sequence_or_number(&ptr);
        if (is_error(probe))
        {
            return error(NULL, probe, *str, "while matching a deletion or deletion/insertion");
        } // if

        HGVS_Node* const node = create(HGVS_Node_deletion);
        if (is_error_allocation(node))
        {
            return error_allocation(NULL);
        } // if

        node->left = probe;

        if (match_string(&ptr, "ins"))
        {
            node->type = HGVS_Node_deletion_insertion;

            probe = inserted(&ptr);
            if (is_error(probe) ||
                is_unmatched(probe))
            {
                return error(node, probe, *str, "while matching a deletion/insertion");
            } // if

            node->right = probe;

            *str = ptr;
            return node;
        } // if

        *str = ptr;
        return node;
    } // if

    return unmatched(NULL);
} // deletion_or_deletion_insertion


static HGVS_Node*
variant(char const** const str)
{
    char const* ptr = *str;
    HGVS_Node* probe = location(&ptr);
    if (is_error(probe) ||
        is_unmatched(probe))
    {
        return error(NULL, probe, *str, "while matching a variant");
    } // if

    HGVS_Node* const node = create(HGVS_Node_variant);
    if (is_error_allocation(node))
    {
        return error_allocation(NULL);
    } // if

    node->left = probe;

    probe = substitution_or_repeat(&ptr);
    if (is_error(probe))
    {
        return error(node, probe, *str, "while matching a variant");
    } // if
    if (!is_unmatched(probe))
    {
        node->right = probe;
        *str = ptr;
        return node;
    } // if

    probe = deletion_or_deletion_insertion(&ptr);
    if (is_error(probe))
    {
        return error(node, probe, *str, "while matching a variant");
    } // if
    if (!is_unmatched(probe))
    {
        node->right = probe;
        *str = ptr;
        return node;
    } // if

    probe = duplication(&ptr);
    if (is_error(probe))
    {
        return error(node, probe, *str, "while matching a variant");
    } // if
    if (!is_unmatched(probe))
    {
        node->right = probe;
        *str = ptr;
        return node;
    } // if

    probe = inversion(&ptr);
    if (is_error(probe))
    {
        return error(node, probe, *str, "while matching a variant");
    } // if
    if (!is_unmatched(probe))
    {
        node->right = probe;
        *str = ptr;
        return node;
    } // if

    return error(node, NULL, ptr, "invalid variant");
} // variant


void
error_print(HGVS_Node* node, char const* const str)
{
    if (is_error(node))
    {
        fprintf(stderr, "error:  %s\n", node->left->ptr);

        error_print(node->right, str);
    } // if
} // error_print


HGVS_Node*
HGVS_parse(char const* const str)
{
    char const* ptr = str;

    HGVS_Node* const node = variant(&ptr);

    if (is_unmatched(node))
    {
        fprintf(stderr, "unmatched\n");
    } // if

    if (*ptr != '\0')
    {
        fprintf(stderr, "input left:  %s\n", ptr);
    } // if

    if (is_error(node))
    {
        fprintf(stderr, "error\n");
        error_print(node, str);
    } // if


    return node;
} // HGVS_parse
