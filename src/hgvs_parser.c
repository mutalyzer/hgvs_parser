#include "../include/hgvs_parser.h"


/*
TODO list
- multi line indent
- EBNF
- implement reference
*/


#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


static size_t const MAX_INTEGER = SIZE_MAX / 10 - 10;


static HGVS_Node allocation_error_context =
{
    .left  = NULL,
    .right = NULL,
    .type  = HGVS_Node_error_context,
    .data  = 0,
    .ptr   = "allocation error: out of memory?"
}; // allocation_error_context


static HGVS_Node allocation_error =
{
    .left  = &allocation_error_context,
    .right = NULL,
    .type  = HGVS_Node_error_allocation,
    .data  = 0,
    .ptr   = NULL
}; // allocation_error


static HGVS_Node* allele(char const** const str);


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
match_reference(char const** const str, size_t* len)
{
    *len = 0;
    char const* ptr = *str;
    while (isalnum(*ptr) || *ptr == '_' ||
                            *ptr == '(' ||
                            *ptr == ')' ||
                            *ptr == '.')
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
} // match_reference


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
error(HGVS_Node* const  node,
      HGVS_Node* const  next,
      char const* const ptr,
      char const* const msg)
{
    HGVS_Node* const err = create(HGVS_Node_error);
    if (is_error_allocation(err))
    {
        HGVS_Node_destroy(node);
        return error_allocation(next);
    } // if

    err->right = next;
    err->left = create(HGVS_Node_error_context);
    if (is_error_allocation(err->left))
    {
        HGVS_Node_destroy(node);
        return error_allocation(err);
    } // if

    err->left->left = node;
    err->left->ptr = msg;

    err->ptr = ptr;

    return err;
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
            return unmatched(NULL);
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
    if (is_error(probe))
    {
        return error(NULL, probe, *str, "while matching a position");
    } // if

    if (is_unmatched(probe))
    {
        return unmatched(NULL);
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
        if (is_error(probe))
        {
            return error(node, probe, *str, "while matching an offset (-)");
        }
        if (is_unmatched(probe))
        {
            return error(node, NULL, ptr, "expected an offset: number ...");
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
        if (is_error(probe))
        {
            return error(node, probe, *str, "while matching an offset (+)");
        }
        if (is_unmatched(probe))
        {
            return error(node, NULL, ptr, "expected an offset: number ...");
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
    if (is_error(probe))
    {
        return error(node, probe, *str, "while matching a point");
    } // if
    if (is_unmatched(probe))
    {
        if (node->data != 0)
        {
            return error(node, NULL, ptr, "expected number or unknown ('?')");
        } // if
        return unmatched(node);
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
        if (is_error(probe))
        {
            return error(node, probe, *str, "while matching an uncertain");
        } // if
        if (is_unmatched(probe))
        {
            return error(node, NULL, ptr, "expected a point: ('-' | '*') number ...");
        } // if

        node->left = probe;

        if (!match_char(&ptr, '_'))
        {
            return error(node, NULL, ptr, "expected: '_'");
        } // if

        probe = point(&ptr);
        if (is_error(probe))
        {
            return error(node, probe, *str, "while matching an uncertain");
        } // if
        if (is_unmatched(probe))
        {
            return error(node, NULL, ptr, "expected a point: ('-' | '*') number ...");
        } // if

        node->right = probe;

        if (!match_char(&ptr, ')'))
        {
            return error(node, NULL, ptr, "expected: ')'");
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
        return error(NULL, node, *str, "while matching an uncertain");
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
            return unmatched(NULL);
        } // if

        *str = ptr;
        return node;
    } // if

    *str = ptr;
    return node;
} // point_or_uncertain


static HGVS_Node*
range(char const** const str)
{
    char const* ptr = *str;
    HGVS_Node* probe = point_or_uncertain(&ptr);
    if (is_error(probe))
    {
        return error(NULL, probe, *str, "while matching a range");
    } // if

    if (is_unmatched(probe))
    {
        return unmatched(NULL);
    } // if

    if (!match_char(&ptr, '_'))
    {
        return error(probe, NULL, ptr, "expected: '_'");
    } // if

    HGVS_Node* const node = create(HGVS_Node_range);
    if (is_error_allocation(node))
    {
        return error_allocation(probe);
    } // if

    node->left = probe;

    probe = point_or_uncertain(&ptr);
    if (is_error(probe))
    {
       return error(node, probe, *str, "while matching a range");
    }
    if (is_unmatched(probe))
    {
        return error(node, NULL, ptr, "expected point or uncertain: ('-' | '*') number ... or '(' point ...");
    } // if

    node->right = probe;

    *str = ptr;
    return node;
} // range


static HGVS_Node*
location(char const** const str)
{
    char const* ptr = *str;
    HGVS_Node* probe = point_or_uncertain(&ptr);
    if (is_error(probe))
    {
        return error(NULL, probe, *str, "while matching a location");
    } // if
    if (is_unmatched(probe))
    {
        return error(NULL, NULL, ptr, "expected a point or uncertain: ('-' | '*') number ... or '(' point ...");
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
        if (is_error(probe))
        {
            return error(node, probe, *str, "while matching a range");
        } // if
        if (is_unmatched(probe))
        {
            return error(node, NULL, ptr, "expected a point or uncertain: ('-' | '*') number ... or '(' point ...");
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
length(char const** const str)
{
    char const* ptr = *str;
    if (match_char(&ptr, '('))
    {
        HGVS_Node* probe = number_or_unknown(&ptr);
        if (is_error(probe))
        {
            return error(NULL, probe, *str, "while matching an insert length");
        } // if
        if (is_unmatched(probe))
        {
            return unmatched(NULL);
        } // if

        if (match_char(&ptr, '_'))
        {
            HGVS_Node* const node = create(HGVS_Node_range_exact);
            if (is_error_allocation(node))
            {
                return error_allocation(probe);
            } // if

            node->left = probe;

            probe = number(&ptr);
            if (is_error(probe))
            {
                return error(node, probe, *str, "while matching an insert length");
            } // if
            if (is_unmatched(probe))
            {
                return unmatched(node);
            } // if

            node->right = probe;

            if (!match_char(&ptr, ')'))
            {
                return unmatched(node);
            } // if

            *str = ptr;
            return node;
        } // if

        if (!match_char(&ptr, ')'))
        {
            return unmatched(probe);
        } // if

        *str = ptr;
        return probe;
    } // if

    return unmatched(NULL);
} // length


static HGVS_Node*
sequence_or_length(char const** const str)
{
    char const* ptr = *str;
    HGVS_Node* node = length(&ptr);
    if (is_error(node))
    {
        return error(NULL, node, *str, "while matching a sequence or length");
    } // if
    if (is_unmatched(node))
    {
        node = sequence(&ptr);
        if (is_error(node))
        {
            return error(NULL, node, *str, "while matching a sequence or length");
        } // if
    } // if

    *str = ptr;
    return node;
} // sequence_or_length


static HGVS_Node*
reference(char const** const str, size_t const prefix)
{
    char const* ptr = *str;
    size_t len = 0;
    match_reference(&ptr, &len);

    if (prefix + len > 0)
    {
        if (!match_char(&ptr, ':'))
        {
            return error(NULL, NULL, ptr, "expected: ':'");
        } // if

        HGVS_Node* const node = create(HGVS_Node_reference);
        if (is_error_allocation(node))
        {
            return error_allocation(NULL);
        } // if

        node->left = create(HGVS_Node_reference_identifier);
        if (is_error_allocation(node->left))
        {
            return error_allocation(node);
        } // if

        node->left->ptr = *str - prefix;
        node->left->data = prefix + len;

        if (match_char(&ptr, 'c'))
        {
            node->data = HGVS_Node_system_c;
        } // if
        else if (match_char(&ptr, 'g'))
        {
            node->data = HGVS_Node_system_g;
        } // if
        else if (match_char(&ptr, 'm'))
        {
            node->data = HGVS_Node_system_m;
        } // if
        else if (match_char(&ptr, 'n'))
        {
            node->data = HGVS_Node_system_n;
        } // if
        else if (match_char(&ptr, 'o'))
        {
            node->data = HGVS_Node_system_o;
        } // if
        else if (match_char(&ptr, 'r'))
        {
            node->data = HGVS_Node_system_r;
        } // if
        else
        {
            HGVS_Node* const probe = allele(&ptr);
            if (is_error(probe))
            {
                return error(node, NULL, *str, "while matching a reference");
            } // if
            if (is_unmatched(probe))
            {
                return error(node, NULL, ptr, "expeced an allele");
            } // if

            node->right = probe;

            *str = ptr;
            return node;
        } // else

        if (!match_char(&ptr, '.'))
        {
            return error(node, NULL, ptr, "expected: '.'");
        } // if

        HGVS_Node* const probe = allele(&ptr);
        if (is_error(probe))
        {
            return error(node, NULL, *str, "while matching a reference");
        } // if
        if (is_unmatched(probe))
        {
            return error(node, NULL, ptr, "expeced an allele");
        } // if

        node->right = probe;

        *str = ptr;
        return node;
    } // if

    return unmatched(NULL);
} // reference


static HGVS_Node*
sequence_or_reference(char const** const str)
{
    char const* ptr = *str;
    if (isalpha(*ptr) == 0)
    {
        return unmatched(NULL);
    } // if

    size_t len = 0;
    match_IUPAC_NT(&ptr, &len);

    if (isalnum(*ptr) != 0 || *ptr == '_')
    {
        HGVS_Node* const node = reference(&ptr, len);
        if (is_error(node))
        {
            return error(NULL, node, *str, "while matching a reference");
        } // if
        if (is_unmatched(node))
        {
            return error(NULL, NULL, ptr, "expected a reference");
        } // if

        *str = ptr;
        return node;
    } // if

    HGVS_Node* const node = create(HGVS_Node_sequence);
    if (is_error_allocation(node))
    {
        return error_allocation(NULL);
    } // if

    node->data = len;
    node->ptr  = *str;

    *str = ptr;
    return node;
} // sequence_or_reference


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

        HGVS_Node* const probe = sequence_or_length(&ptr);
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

        HGVS_Node* const probe = sequence_or_length(&ptr);
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
repeated(char const** const str)
{
    char const* ptr = *str;
    if (match_char(&ptr, '['))
    {
        HGVS_Node* const node = number(&ptr);
        if (is_error(node))
        {
            return error(NULL, node, *str, "while matching a repeated (exact)");
        } // if
        if (is_unmatched(node))
        {
            return error(NULL, NULL, ptr, "expected: number ']' ...");
        } // if

        if (!match_char(&ptr, ']'))
        {
            return error(node, NULL, ptr, "expected: ']'");
        } // if

        *str = ptr;
        return node;
    } // if

    if (match_char(&ptr, '('))
    {
        HGVS_Node* probe = number(&ptr);
        if (is_error(probe))
        {
            return error(NULL, probe, *str, "while matching a repeated (uncertain)");
        } // if
        if (is_unmatched(probe))
        {
            return error(NULL, NULL, ptr, "expected: number '_' number ')' ...");
        } // if

        if (!match_char(&ptr, '_'))
        {
            return error(probe, NULL, ptr, "expected: '_'");
        } // if

        HGVS_Node* const node = create(HGVS_Node_range_exact);
        if (is_error_allocation(node))
        {
            return error_allocation(probe);
        } // if

        node->left = probe;

        probe = number(&ptr);
        if (is_error(probe))
        {
            return error(NULL, probe, *str, "while matching a repeated (uncertain)");
        } // if
        if (is_unmatched(probe))
        {
            return error(NULL, NULL, ptr, "expected: number ')' ...");
        } // if

        node->right = probe;

        if (!match_char(&ptr, ')'))
        {
            return error(node, NULL, ptr, "expected: ')'");
        } // if

        *str = ptr;
        return node;
    } // if

    return unmatched(NULL);
} // repeated


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
        if (is_error(probe))
        {
            return error(node, probe, *str, "while matching a substitution");
        } // if
        if (is_unmatched(probe))
        {
            return error(node, NULL, ptr, "expected a sequence: 'A' | 'C' | 'G' | 'T' ...");
        } // if

        node->right = probe;

        *str = ptr;
        return node;
    } // if

    HGVS_Node* const node = create(HGVS_Node_repeat);
    if (is_error_allocation(node))
    {
        return error_allocation(probe);
    } // if

    node->left = probe;

    probe = repeated(&ptr);
    if (is_error(probe))
    {
        return error(node, probe, *str, "while matching a repeat");
    } // if
    if (is_unmatched(probe))
    {
        return error(node, NULL, ptr, "expected a repeat or substitution: '[' ... or '>' ...");
    } // if

    node->right = probe;

    *str = ptr;
    return node;
} // substitution_or_repeat


static HGVS_Node*
insert(char const** const str)
{
    char const* ptr = *str;
    HGVS_Node* probe = length(&ptr);
    if (is_error(probe))
    {
        return error(NULL, probe, *str, "while matching an insert");
    } // if

    if (is_unmatched(probe))
    {
        probe = range(&ptr);
        if (is_error(probe))
        {
            return error(NULL, probe, *str, "while matching an insert");
        } // if
        if (is_unmatched(probe))
        {
            probe = sequence_or_reference(&ptr);
            if (is_error(probe))
            {
                return error(NULL, probe, *str, "while matching an insert");
            } // if
            if (is_unmatched(probe))
            {
                return error(NULL, NULL, ptr, "expected an inserted part");
            } // if
        } // if
    } // if

    HGVS_Node* const node = create(HGVS_Node_inserted);
    if (is_error_allocation(node))
    {
        return error_allocation(probe);
    } // if

    node->left = probe;

    if (match_string(&ptr, "inv"))
    {
        node->data = HGVS_Node_inverted;
    } // if

    probe = repeated(&ptr);
    if (is_error(probe))
    {
        return error(node, probe, *str, "while matching an insert");
    } // if

    node->right = probe;

    *str = ptr;
    return node;
} // insert


static HGVS_Node*
inserted(char const** const str)
{
    char const* ptr = *str;
    if (match_char(&ptr, '['))
    {
        HGVS_Node* const node = create(HGVS_Node_inserted_compound);
        if (is_error_allocation(node))
        {
            return error_allocation(NULL);
        } // if

        HGVS_Node* probe = insert(&ptr);
        if (is_error(probe))
        {
            return error(node, probe, *str, "while matching an inserted compound");
        } // if

        node->left = probe;

        HGVS_Node* tmp = node;
        while (match_char(&ptr, ';'))
        {
            HGVS_Node* const next = create(HGVS_Node_inserted_compound);
            if (is_error_allocation(node))
            {
                return error_allocation(node);
            } // if

            probe = insert(&ptr);
            if (is_error(probe))
            {
                HGVS_Node_destroy(next);
                return error(node, probe, *str, "while matching an inserted compound");
            } // if

            next->left = probe;

            tmp->right = next;
            tmp = tmp->right;
        } // while

        if (!match_char(&ptr, ']'))
        {
            return error(node, NULL, ptr, "expected: ']'");
        } // if

        *str = ptr;
        return node;
    } // if

    return insert(str);
} // inserted


static HGVS_Node*
deletion_or_deletion_insertion(char const** const str)
{
    char const* ptr = *str;
    if (match_string(&ptr, "del"))
    {
        HGVS_Node* probe = sequence_or_length(&ptr);
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
            if (is_error(probe))
            {
                return error(node, probe, *str, "while matching a deletion/insertion");
            } // if

            node->right = probe;
        } // if

        *str = ptr;
        return node;
    } // if

    return unmatched(NULL);
} // deletion_or_deletion_insertion


static HGVS_Node*
insertion(char const** const str)
{
    char const* ptr = *str;
    if (match_string(&ptr, "ins"))
    {
        HGVS_Node* const node = create(HGVS_Node_insertion);
        if (is_error_allocation(node))
        {
            return error_allocation(NULL);
        } // if

        HGVS_Node* const probe = inserted(&ptr);
        if (is_error(probe))
        {
            return error(node, probe, *str, "while matching a insertion");
        } // if

        node->left = probe;

        *str = ptr;
        return node;
    } // if

    return unmatched(NULL);
} // insertion


static HGVS_Node*
range_or_reference(char const** const str)
{
    char const* ptr = *str;
    HGVS_Node* node = range(&ptr);
    if (is_error(node))
    {
        return error(NULL, node, *str, "while matching a range or reference");
    } // if
    if (is_unmatched(node))
    {
        node = reference(&ptr, 0);
        if (is_error(node))
        {
            return error(NULL, node, *str, "while matching a range or reference");
        } // if
    } // if

    *str = ptr;
    return node;
} // range_or_reference


static HGVS_Node*
conversion(char const** const str)
{
    char const* ptr = *str;
    if (match_string(&ptr, "con"))
    {
        HGVS_Node* const node = create(HGVS_Node_conversion);
        if (is_error_allocation(node))
        {
            return error_allocation(NULL);
        } // if

        HGVS_Node* const probe = range_or_reference(&ptr);
        if (is_error(probe))
        {
            return error(node, probe, *str, "while matching a conversion");
        } // if

        node->left = probe;

        *str = ptr;
        return node;
    } // if

    return unmatched(NULL);
} // conversion


static HGVS_Node*
repeat(char const** const str)
{
    char const* ptr = *str;
    HGVS_Node* const probe = repeated(&ptr);
    if (is_error(probe))
    {
        return error(NULL, probe, *str, "while matching a repeat");
    } // if
    if (is_unmatched(probe))
    {
        return unmatched(NULL);
    } // if

    HGVS_Node* const node = create(HGVS_Node_repeat);
    if (is_error_allocation(node))
    {
        return error_allocation(NULL);
    } // if

    node->right = probe;

    *str = ptr;
    return node;
} // repeat


static HGVS_Node*
equal(char const** const str)
{
    char const* ptr = *str;
    if (match_char(&ptr, '='))
    {
        HGVS_Node* const node = create(HGVS_Node_equal);
        if (is_error_allocation(node))
        {
            return error_allocation(NULL);
        } // if

        *str = ptr;
        return node;
    } // if

    return unmatched(NULL);
} // equal


static HGVS_Node*
variant(char const** const str)
{
    char const* ptr = *str;
    HGVS_Node* probe = location(&ptr);
    if (is_error(probe))
    {
        return error(NULL, probe, *str, "while matching a variant");
    } // if
    if (is_unmatched(probe))
    {
        return error(NULL, NULL, ptr, "expected a location");
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

    probe = insertion(&ptr);
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

    probe = equal(&ptr);
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

    probe = repeat(&ptr);
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

    probe = conversion(&ptr);
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

    return error(node, NULL, ptr, "expected: '=', con', 'del', 'dup', 'ins', 'inv', substitution or repeat");
} // variant


static HGVS_Node*
allele(char const** const str)
{
    char const* ptr = *str;
    if (match_char(&ptr, '['))
    {
        if (match_char(&ptr, '='))
        {
            HGVS_Node* const node = create(HGVS_Node_equal_allele);
            if (is_error_allocation(node))
            {
                return error_allocation(NULL);
            } // if

            if (!match_char(&ptr, ']'))
            {
                return error(node, NULL, ptr, "expected: ']'");
            } // if

            *str = ptr;
            return node;
        } // if

        HGVS_Node* const node = create(HGVS_Node_variant_list);
        if (is_error_allocation(node))
        {
            return error_allocation(NULL);
        } // if

        HGVS_Node* probe = variant(&ptr);
        if (is_error(probe))
        {
            return error(node, probe, *str, "while matching an allele");
        } // if
        if (is_unmatched(probe))
        {
            return error(node, NULL, ptr, "expected a variant");
        } // if

        node->left = probe;

        HGVS_Node* tmp = node;
        while (match_char(&ptr, ';'))
        {
            HGVS_Node* const next = create(HGVS_Node_variant_list);
            if (is_error_allocation(node))
            {
                return error_allocation(node);
            } // if

            probe = variant(&ptr);
            if (is_error(probe))
            {
                HGVS_Node_destroy(next);
                return error(node, probe, *str, "while matching an allele");
            } // if
            if (is_unmatched(probe))
            {
                HGVS_Node_destroy(next);
                return error(node, NULL, ptr, "expected a variant");
            } // if

            next->left = probe;

            tmp->right = next;
            tmp = tmp->right;
        } // while

        if (!match_char(&ptr, ']'))
        {
            return error(node, NULL, ptr, "expected: ']'");
        } // if

        *str = ptr;
        return node;
    } // if

    if (match_char(&ptr, '='))
    {
        HGVS_Node* const node = create(HGVS_Node_equal_allele);
        if (is_error_allocation(node))
        {
            return error_allocation(NULL);
        } // if

        *str = ptr;
        return node;
    } // if

    HGVS_Node* const node = variant(&ptr);
    if (is_error(node))
    {
        return error(NULL, node, *str, "while matching an allele");
    } // if
    if (is_unmatched(node))
    {
        return error(NULL, NULL, ptr, "expected a variant, variant list or '='");
    } // if

    if (*ptr != '\0')
    {
        return error(node, NULL, ptr, "unmatched input");
    } // if

    *str = ptr;
    return node;
} // allele


void
error_print_indent(char const* const str, char const* const ptr)
{
    for(ptrdiff_t i = 0; i < ptr - str; ++i)
    {
        fprintf(stderr, " ");
    } // if
} // error_print_indent


void
error_print(HGVS_Node* node, char const* const str)
{
    if (is_error(node))
    {
        error_print(node->right, str);
        error_print_indent(str, node->ptr);
        fprintf(stderr, "^  (%u)  %s\n", (node->left->left != NULL ? node->left->left->type : 0), node->left->ptr);
    } // if
} // error_print


HGVS_Node*
HGVS_parse(char const* const str)
{
    char const* ptr = str;

    HGVS_Node* const node = allele(&ptr);

    if (is_error(node))
    {

        fprintf(stderr, "ERROR:\n%s\n", str);
        error_print(node, str);
        HGVS_Node_destroy(node);
        return unmatched(NULL);
    } // if

    return node;
} // HGVS_parse
