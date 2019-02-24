#include "../include/hgvs_parser.h"


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#define ANSI_RESET       "\033[0m"
#define ANSI_BLACK       "\033[30m"
#define ANSI_RED         "\033[31m"
#define ANSI_GREEN       "\033[32m"
#define ANSI_YELLOW      "\033[33m"
#define ANSI_BLUE        "\033[34m"
#define ANSI_MAGENTA     "\033[35m"
#define ANSI_CYAN        "\033[36m"
#define ANSI_WHITE       "\033[37m"
#define ANSI_BOLDBLACK   "\033[1m\033[30m"
#define ANSI_BOLDRED     "\033[1m\033[31m"
#define ANSI_BOLDGREEN   "\033[1m\033[32m"
#define ANSI_BOLDYELLOW  "\033[1m\033[33m"
#define ANSI_BOLDBLUE    "\033[1m\033[34m"
#define ANSI_BOLDMAGENTA "\033[1m\033[35m"
#define ANSI_BOLDCYAN    "\033[1m\033[36m"
#define ANSI_BOLDWHITE   "\033[1m\033[37m"


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


static size_t const NODE_POSITIVE_OFFSET = 1;
static size_t const NODE_NEGATIVE_OFFSET = 2;
static size_t const NODE_DOWNSTREAM      = 1;
static size_t const NODE_UPSTREAM        = 2;
static size_t const NODE_INVERTED        = 1;


typedef struct Node
{
    enum Node_Type
    {
        NODE_ALLOCATION_ERROR,
        NODE_ERROR,
        NODE_ERROR_CONTEXT,
        NODE_UNKNOWN,
        NODE_NUMBER,
        NODE_SEQUENCE,
        NODE_IDENTIFIER,
        NODE_REFERENCE,
        NODE_DESCRIPTION,
        NODE_OFFSET,
        NODE_POINT,
        NODE_UNCERTAIN_POINT,
        NODE_RANGE,
        NODE_LENGTH,
        NODE_INSERT,
        NODE_COMPOUND_INSERT,
        NODE_SUBSTITUTION,
        NODE_REPEAT,
        NODE_COMPOUND_REPEAT,
        NODE_DELETION,
        NODE_DELETION_INSERTION,
        NODE_INSERTION,
        NODE_DUPLICATION,
        NODE_CONVERSION,
        NODE_INVERSION,
        NODE_EQUAL,
        NODE_SLICE,
        NODE_VARIANT,
        NODE_COMPOUND_VARIANT,
    } Node_Type;

    struct Node* left;
    struct Node* right;

    char const* ptr;
    size_t      data;

    enum Node_Type type;
} Node;


static Node ALLOCATION_ERROR = {
    .left  = NULL,
    .right = NULL,
    .data  = 0,
    .ptr   = "allocation error; out of memory?",
    .type  = NODE_ALLOCATION_ERROR
}; // ALLOCATION_ERROR


static inline bool
is_error(Node* const node)
{
    return node != NULL && node->type == NODE_ERROR;
} // is_error


static inline void
destroy(Node* const node)
{
    if (node != NULL)
    {
        destroy(node->left);
        destroy(node->right);
        free(node);
    } // if
} // destroy


static inline Node*
allocation_error(Node* const node)
{
    destroy(node);
    return &ALLOCATION_ERROR;
} // allocation_error


static inline Node*
unmatched(Node* const node)
{
    destroy(node);
    return NULL;
} // unmatched


static inline Node*
create(enum Node_Type const type)
{
    Node* const node = malloc(sizeof(*node));
    if (node == NULL)
    {
        return allocation_error(NULL);
    } // if

    node->left = NULL;
    node->right = NULL;

    node->data = 0;
    node->ptr = NULL;

    node->type = type;

    return node;
} // create


static inline Node*
error(Node* const       cxt,
      Node* const       err,
      char const* const ptr,
      char const* const msg)
{
    Node* const node = create(NODE_ERROR);
    if (node == &ALLOCATION_ERROR)
    {
        destroy(cxt);
        destroy(err);
        return allocation_error(NULL);
    } // if

    node->left = create(NODE_ERROR_CONTEXT);
    if (node->left == &ALLOCATION_ERROR)
    {
        destroy(cxt);
        destroy(err);
        return allocation_error(NULL);
    } // if
    node->left->left = cxt;
    node->left->ptr = msg;

    node->right = err;

    node->ptr = ptr;

    return node;
} // error


static Node* allele(char const** const ptr);


static Node*
unknown(char const** const ptr)
{
    Node* const node = create(NODE_UNKNOWN);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    if (!match_char(ptr, '?'))
    {
        return unmatched(node);
    } // if
    return node;
} // unknown


static Node*
number(char const** const ptr)
{
    Node* const node = create(NODE_NUMBER);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    if (!match_number(ptr, &node->data))
    {
        return unmatched(node);
    } // if
    return node;
} // number


static Node*
unknown_or_number(char const** const ptr)
{
    Node* node = unknown(ptr);
    if (node == NULL)
    {
        node = number(ptr);
        if (node == NULL)
        {
            return unmatched(NULL);
        } // if
    } // if
    return node;
} // unknown_or_number


static Node*
sequence(char const** const ptr)
{
    Node* const node = create(NODE_SEQUENCE);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;
    if (!match_sequence(ptr, &node->data))
    {
        return unmatched(node);
    } // if
    return node;
} // sequence


static Node*
identifier(char const** const ptr)
{
    Node* const node = create(NODE_IDENTIFIER);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;
    if (!match_identifier(ptr, &node->data))
    {
        return unmatched(node);
    } // if
    return node;
} // identifier


static Node*
reference(char const** const ptr)
{
    Node* const node = create(NODE_REFERENCE);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    Node* probe = identifier(ptr);
    if (probe == NULL)
    {
        return error(node, NULL, *ptr, "expected an identifier");
    } // if
    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching an identifier");
    } // if
    node->left = probe;

    if (match_char(ptr, '('))
    {
        probe = reference(ptr);
        if (probe == NULL)
        {
            return error(node, NULL, *ptr, "expected a reference");
        } // if
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching a reference");
        } // if
        node->right = probe;

        if (!match_char(ptr, ')'))
        {
            return error(node, NULL, *ptr, "expected: ')'");
        } // if
    } // if
    return node;
} // reference


static Node*
description(char const** const ptr)
{
    Node* const node = create(NODE_DESCRIPTION);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    Node* probe = reference(ptr);
    if (probe == NULL)
    {
        return error(node, NULL, *ptr, "expected a reference");
    } // if
    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching a description");
    } // if
    node->left = probe;

    if (!match_char(ptr, ':'))
    {
        return error(node, NULL, *ptr, "expected: ':'");
    } // if

    if (match_alpha(ptr, &node->data))
    {
        if (!match_char(ptr, '.'))
        {
            return error(node, NULL, *ptr, "expected a coordinate system");
        } // if
    } // if

    probe = allele(ptr);
    if (probe == NULL)
    {
        return error(node, NULL, *ptr, "expected an allele");
    } // if
    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching a description");
    } // if
    node->right = probe;

    return node;
} // description


static Node*
offset(char const** const ptr)
{
    Node* const node = create(NODE_OFFSET);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    bool matched = false;
    if (match_char(ptr, '+'))
    {
        matched = true;
        node->data = NODE_POSITIVE_OFFSET;
    } // if
    else if (match_char(ptr, '-'))
    {
        matched = true;
        node->data = NODE_NEGATIVE_OFFSET;
    } // if

    if (matched)
    {
        Node* const probe = unknown_or_number(ptr);
        if (probe == NULL)
        {
            return error(node, NULL, *ptr, "expected an unknown or number");
        } // if
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching an offset");
        } // if
        node->left = probe;

        return node;
    } // if
    return unmatched(node);
} // offset


static Node*
point(char const** const ptr)
{
    Node* const node = create(NODE_POINT);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    if (match_char(ptr, '*'))
    {
        node->data = NODE_DOWNSTREAM;
    } // if
    else if (match_char(ptr, '-'))
    {
        node->data = NODE_UPSTREAM;
    } // if

    Node* probe = unknown_or_number(ptr);
    if (probe == NULL)
    {
        return unmatched(node);
    } // if
    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching a point");
    } // if
    node->left = probe;

    probe = offset(ptr);
    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching a point");
    } // if
    node->right = probe;

    return node;
} // point


static Node*
uncertain_point(char const** const ptr)
{
    Node* const node = create(NODE_UNCERTAIN_POINT);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    if (match_char(ptr, '('))
    {
        Node* probe = point(ptr);
        if (probe == NULL)
        {
            return error(node, NULL, *ptr, "expected a point");
        } // if
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching an uncertain point");
        } // if
        node->left = probe;

        if (!match_char(ptr, '_'))
        {
            return error(node, NULL, *ptr, "expected: '_'");
        } // if

        probe = point(ptr);
        if (probe == NULL)
        {
            return error(node, NULL, *ptr, "expected a point");
        } // if
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching an uncertain point");
        } // if
        node->right = probe;

        if (!match_char(ptr, ')'))
        {
            return error(node, NULL, *ptr, "expected: ')'");
        } // if

        return node;
    } // if
    return unmatched(node);
} // uncertain_point


static Node*
uncertain_point_or_point(char const** const ptr)
{
    char const* const err = *ptr;
    Node* node = uncertain_point(ptr);
    if (is_error(node))
    {
        return error(NULL, node, err, "while matching an uncertain point");
    } // if

    if (node == NULL)
    {
        node = point(ptr);
        if (node == NULL)
        {
            return unmatched(NULL);
        } // if
        if (is_error(node))
        {
            return error(NULL, node, err, "while matching a point");
        } // if
    } // if
    return node;
} // uncertain_point_or_point


static Node*
location(char const** const ptr)
{
    char const* const err = *ptr;
    Node* probe = uncertain_point_or_point(ptr);
    if (probe == NULL)
    {
        return unmatched(NULL);
    } // if
    if (is_error(probe))
    {
        return error(NULL, probe, err, "while matching a location");
    } // if

    if (match_char(ptr, '_'))
    {
        Node* const node = create(NODE_RANGE);
        if (node == &ALLOCATION_ERROR)
        {
            return allocation_error(NULL);
        } // if
        node->left = probe;

        probe = uncertain_point_or_point(ptr);
        if (probe == NULL)
        {
            return error(node, NULL, *ptr, "expected a uncertain point or point");
        } // if
        if (is_error(probe))
        {
            return error(node, probe, err, "while matching a range");
        } // if
        node->right = probe;

        return node;
    } // if
    return probe;
} // location


static Node*
sequence_or_location(char const** const ptr)
{
    char const* const err = *ptr;
    Node* node = sequence(ptr);
    if (node == NULL)
    {
        node = location(ptr);
        if (node == NULL)
        {
            return unmatched(NULL);
        } // if
        if (is_error(node))
        {
            return error(NULL, node, err, "while matching a location");
        } // if
    } // if
    return node;
} // sequence_or_location


static Node*
unknown_or_number_or_exact_range(char const** const ptr)
{
    char const* const err = *ptr;
    Node* probe = unknown_or_number(ptr);
    if (probe == NULL)
    {
        return unmatched(NULL);
    } // if
    if (is_error(probe))
    {
        return error(NULL, probe, err, "while matching an unknown, number or exact range");
    } // if

    if (match_char(ptr, '_'))
    {
        Node* const node = create(NODE_RANGE);
        if (node == &ALLOCATION_ERROR)
        {
            return allocation_error(probe);
        } // if
        node->left = probe;

        probe = unknown_or_number(ptr);
        if (probe == NULL)
        {
            return error(node, NULL, *ptr, "expected an unknown or number");
        } // if
        if (is_error(probe))
        {
            return error(node, probe, err, "while matching an exact range");
        } // if
        node->right = probe;

        return node;
    } // if
    return probe;
} // unknown_or_number_or_exact_range


static Node*
repeated(char const** const ptr)
{
    char const* const err = *ptr;
    if (!match_char(ptr, '['))
    {
        return unmatched(NULL);
    } // if

    Node* const node = unknown_or_number_or_exact_range(ptr);
    if (node == NULL)
    {
        return error(NULL, NULL, *ptr, "expected an unknown, number or exact range");
    } // if
    if (is_error(node))
    {
        return error(NULL, node, err, "while matching a repeat number");
    } // if

    if (!match_char(ptr, ']'))
    {
        return error(node, NULL, *ptr, "expected: ']'");
    } // if

    return node;
} // repeated


static Node*
repeat(char const** const ptr)
{
    Node* const node = create(NODE_REPEAT);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    Node* probe = sequence_or_location(ptr);
    if (probe == NULL)
    {
        return unmatched(node);
    } // if
    if (is_error(probe))
    {
        return error(NULL, probe, node->ptr, "while matching a repeat");
    } // if
    node->left = probe;

    probe = repeated(ptr);
    if (probe == NULL)
    {
        return error(node, NULL, *ptr, "expected repeat number");
    } // if
    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching a repeat");
    } // if
    node->right = probe;

    return node;
} // repeat


static Node*
compound_repeat(char const** const ptr)
{
    Node* const node = create(NODE_COMPOUND_REPEAT);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    Node* probe = repeat(ptr);
    if (probe == NULL)
    {
        return unmatched(node);
    } // if
    if (is_error(probe))
    {
        return probe;
    } // if
    node->left = probe;
    node->data = 1;

    probe = repeat(ptr);
    if (is_error(probe))
    {
        return probe;
    } // if

    Node* tmp = node;
    while (probe != NULL)
    {
        node->data += 1;
        tmp->right = create(NODE_COMPOUND_REPEAT);
        if (tmp->right == &ALLOCATION_ERROR)
        {
            return allocation_error(node);
        } // if
        tmp = tmp->right;
        tmp->left = probe;
        probe = repeat(ptr);
        if (is_error(probe))
        {
            return probe;
        } // if
    } // while
    return node;
} // compound_repeat


static Node*
substitution_or_repeat(char const** const ptr)
{
    Node* const node = create(NODE_SUBSTITUTION);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    Node* probe = sequence(ptr);
    if (probe == NULL)
    {
        return unmatched(probe);
    } // if
    node->left = probe;

    if (match_char(ptr, '>'))
    {
        probe = sequence(ptr);
        if (probe == NULL)
        {
            return error(node, NULL, *ptr, "expected a sequence");
        } // if
        node->right = probe;

        return node;
    } // if

    probe = repeated(ptr);
    if (probe == NULL)
    {
        return error(node, NULL, *ptr, "expected a repeat number");
    } // if
    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching a repeat");
    } // if
    node->right = probe;
    node->type = NODE_REPEAT;

    probe = compound_repeat(ptr);
    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching a repeat");
    } // if

    if (probe != NULL)
    {
        Node* const new = create(NODE_COMPOUND_REPEAT);
        if (new == &ALLOCATION_ERROR)
        {
            destroy(probe);
            return allocation_error(node);
        } // if
        new->ptr = *ptr;
        new->left = node;
        new->right = probe;

        return new;
    } // if

    return node;
} // substitution_or_repeat


static Node*
length(char const** const ptr)
{
    Node* const node = create(NODE_LENGTH);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    if (match_char(ptr, '('))
    {
        Node* const probe = unknown_or_number_or_exact_range(ptr);
        if (probe == NULL)
        {
            return error(node, NULL, *ptr, "expected unknown, number or exact range");
        } // if
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching a length");
        } // if
        node->left = probe;

        if (!match_char(ptr, ')'))
        {
            return error(node, NULL, *ptr, "expected: ')'");
        } // if
        return node;
    } // if

    return unmatched(node);
} // length


static Node*
length_or_unknown_or_number(char const** const ptr)
{
    char const* const err = *ptr;
    Node* node = length(ptr);
    if (is_error(node))
    {
        return error(NULL, node, err, "while matching a length, unknown or number");
    } // if
    if (node == NULL)
    {
        return unknown_or_number(ptr);
    } // if
    return node;
} // length_or_unknown_or_number


static Node*
sequence_or_length(char const** const ptr)
{
    char const* const err = *ptr;
    Node* node = sequence(ptr);
    if (node == NULL)
    {
        node = length_or_unknown_or_number(ptr);
        if (node == NULL)
        {
            return unmatched(NULL);
        } // if
        if (is_error(node))
        {
            return error(NULL, node, err, "while matching a length, unknown or number");
        } // if
    } // if
    return node;
} // sequence_or_length


static Node*
sequence_or_description(char const** const ptr)
{
    Node* const node = create(NODE_SEQUENCE);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    if (!is_alpha(**ptr))
    {
        return unmatched(NULL);
    } // if

    match_sequence(ptr, &node->data);
    size_t len = 0;
    while (is_alphanumeric(**ptr) || **ptr == '.' || **ptr == '_')
    {
        *ptr += 1;
        len += 1;
    } // while
    if (len > 0)
    {
        node->type = NODE_DESCRIPTION;
        node->left = create(NODE_REFERENCE);
        if (node->left == &ALLOCATION_ERROR)
        {
            return allocation_error(node);
        } // if
        node->left->ptr = node->ptr;

        node->left->left = create(NODE_IDENTIFIER);
        if (node->left->left == &ALLOCATION_ERROR)
        {
            return allocation_error(node);
        } // if
        node->left->left->ptr = node->ptr;
        node->left->left->data = node->data + len;

        if (match_char(ptr, '('))
        {
            Node* const probe = reference(ptr);
            if (probe == NULL)
            {
                return error(node, NULL, *ptr, "expected a reference");
            } // if
            if (is_error(probe))
            {
                return error(node, probe, node->ptr, "while matching a description");
            } // if
            node->right = probe;

            if (!match_char(ptr, ')'))
            {
                return error(node, NULL, *ptr, "expected: ')'");
            } // if
        } // if

        if (!match_char(ptr, ':'))
        {
            return error(node, NULL, *ptr, "expected: ':'");
        } // if

        if (match_alpha(ptr, &node->data))
        {
            if (!match_char(ptr, '.'))
            {
                return error(node, NULL, *ptr, "expected a coordinate system");
            } // if
        } // if

        Node* const probe = allele(ptr);
        if (probe == NULL)
        {
            return error(node, NULL, *ptr, "expected an allele");
        } // if
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching a description");
        } // if
        node->right = probe;

        return node;
    } // if

    if (node->data == 0)
    {
        return error(node, NULL, node->ptr, "expected a sequence or description");
    } // if

    return node;
} // sequence_or_description


static Node*
location_or_length(char const** const ptr)
{
    char const* const err = *ptr;
    Node* probe = length(ptr);
    if (is_error(probe))
    {
        return error(NULL, probe, err, "while matching a length, unknown or number");
    } // if

    if (probe == NULL)
    {
        probe = location(ptr);
        if (is_error(probe))
        {
            return error(NULL, probe, err, "while matching a location");
        } // if
    } // if
    return probe;
} // location_or_length


static Node*
insert(char const** const ptr)
{
    Node* const node = create(NODE_INSERT);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    Node* probe = sequence_or_description(ptr);
    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching an insert");
    } // if

    if (probe == NULL)
    {
        probe = location_or_length(ptr);
        if (probe == NULL)
        {
            return error(node, NULL, *ptr, "expected a sequence, description, location or length");
        } // if
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching an insert");
        } // if
    } // if
    node->left = probe;

    if (match_string(ptr, "inv"))
    {
        node->data = NODE_INVERTED;
    } // if

    probe = repeated(ptr);

    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching an insert");
    } // if
    node->right = probe;

    if (match_string(ptr, "inv"))
    {
        node->data = NODE_INVERTED;
    } // if

    return node;
} // insert


static Node*
inserted(char const** const ptr)
{
    if (match_char(ptr, '['))
    {
        Node* const node = create(NODE_COMPOUND_INSERT);
        if (node == &ALLOCATION_ERROR)
        {
            return allocation_error(NULL);
        } // if
        node->ptr = *ptr;

        Node* probe = insert(ptr);
        if (probe == NULL)
        {
            return error(node, NULL, *ptr, "expected an insert");
        } // if
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching a compound insert");
        } // if
        node->left = probe;
        node->data = 1;

        Node* tmp = node;
        while (match_char(ptr, ';'))
        {
            node->data += 1;
            tmp->right = create(NODE_COMPOUND_INSERT);
            if (tmp->right == &ALLOCATION_ERROR)
            {
                return allocation_error(node);
            } // if
            tmp = tmp->right;

            probe = insert(ptr);
            if (probe == NULL)
            {
                return error(node, NULL, *ptr, "expected an insert");
            } // if
            if (is_error(probe))
            {
                return error(node, probe, node->ptr, "while matching a compound insert");
            } // if
            tmp->left = probe;
        } // while

        if (!match_char(ptr, ']'))
        {
            return error(node, NULL, *ptr, "expected: ']'");
        } // if

        return node;
    } // if

    return insert(ptr);
} // inserted


static Node*
insertion(char const** const ptr)
{
    Node* const node = create(NODE_INSERTION);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    if (match_string(ptr, "ins"))
    {
        Node* const probe = inserted(ptr);
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching an insertion");
        } // if
        node->left = probe;

        return node;
    } // if

    *ptr = node->ptr;
    return unmatched(node);
} // insertion


static Node*
deletion_or_deletion_insertion(char const** const ptr)
{
    Node* const node = create(NODE_DELETION);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    if (match_string(ptr, "del"))
    {
        Node* probe = sequence_or_length(ptr);
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching a deletion or deletion/insertion");
        } // if
        node->left = probe;

        if (match_string(ptr, "ins"))
        {
            node->type = NODE_DELETION_INSERTION;

            probe = inserted(ptr);
            if (is_error(probe))
            {
                return error(node, probe, node->ptr, "while matching a deletion/insertion");
            } // if
            node->right = probe;
        } // if

        return node;
    } // if

    *ptr = node->ptr;
    return unmatched(node);
} // deletion_or_deletion_insertion


static Node*
duplication(char const** const ptr)
{
    Node* const node = create(NODE_DUPLICATION);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    if (match_string(ptr, "dup"))
    {
        Node* const probe = sequence_or_length(ptr);
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching an duplication");
        } // if
        node->left = probe;

        return node;
    } // if

    *ptr = node->ptr;
    return unmatched(node);
} // duplication


static Node*
conversion(char const** const ptr)
{
    Node* const node = create(NODE_CONVERSION);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    if (match_string(ptr, "con"))
    {
        Node* probe = location(ptr);
        if (probe == NULL)
        {
            probe = description(ptr);
            if (probe == NULL)
            {
                return error(node, NULL, *ptr, "expected a location or description");
            } // if
        } // if

        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching an conversion");
        } // if
        node->left = probe;

        return node;
    } // if
    return unmatched(node);
} // conversion


static Node*
inversion(char const** const ptr)
{
    Node* const node = create(NODE_INVERSION);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    if (match_string(ptr, "inv"))
    {
        Node* const probe = sequence_or_length(ptr);
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching an inversion");
        } // if
        node->left = probe;

        return node;
    } // if

    *ptr = node->ptr;
    return unmatched(node);
} // inversion


static Node*
equal(char const** const ptr)
{
    Node* const node = create(NODE_EQUAL);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    if (match_char(ptr, '='))
    {
        Node* const probe = sequence_or_length(ptr);
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching an equal");
        } // if
        node->left = probe;

        return node;
    } // if

    *ptr = node->ptr;
    return unmatched(node);
} // equal


static Node*
variant(char const** const ptr)
{
    Node* const node = create(NODE_VARIANT);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    Node* probe = location(ptr);
    if (probe == NULL)
    {
        return error(node, NULL, *ptr, "expected a location");
    } // if
    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching a variant");
    } // if
    node->left = probe;

    probe = substitution_or_repeat(ptr);
    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching a variant");
    } // if
    if (probe != NULL)
    {
        node->right = probe;
        return node;
    } // if

    probe = deletion_or_deletion_insertion(ptr);
    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching a variant");
    } // if
    if (probe != NULL)
    {
        node->right = probe;
        return node;
    } // if

    probe = insertion(ptr);
    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching a variant");
    } // if
    if (probe != NULL)
    {
        node->right = probe;
        return node;
    } // if

    probe = duplication(ptr);
    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching a variant");
    } // if
    if (probe != NULL)
    {
        node->right = probe;
        return node;
    } // if

    probe = inversion(ptr);
    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching a variant");
    } // if
    if (probe != NULL)
    {
        node->right = probe;
        return node;
    } // if

    probe = conversion(ptr);
    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching a variant");
    } // if
    if (probe != NULL)
    {
        node->right = probe;
        return node;
    } // if

    probe = equal(ptr);
    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching a variant");
    } // if
    if (probe != NULL)
    {
        node->right = probe;
        return node;
    } // if

    probe = repeated(ptr);
    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching a variant");
    } // if
    if (probe != NULL)
    {
        node->type = NODE_REPEAT;
        node->right = probe;

        probe = compound_repeat(ptr);
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching a variant");
        } // if

        if (probe != NULL)
        {
            Node* const new = create(NODE_COMPOUND_REPEAT);
            if (new == &ALLOCATION_ERROR)
            {
                destroy(probe);
                return allocation_error(node);
            } // if
            node->ptr = *ptr;
            new->left = node;
            new->right = probe;

            return new;
        } // if
        return node;
    } // if

    node->right = create(NODE_SLICE);
    if (node->right == &ALLOCATION_ERROR)
    {
        return allocation_error(node);
    } // if
    node->right->ptr = node->ptr;

    return node;
} // variant


static Node*
allele(char const** const ptr)
{
    char const* const err = *ptr;
    if (match_char(ptr, '['))
    {
        Node* const node = create(NODE_COMPOUND_VARIANT);
        if (node == &ALLOCATION_ERROR)
        {
            return allocation_error(NULL);
        } // if
        node->ptr = *ptr - 1;

        Node* probe = variant(ptr);
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching an allele");
        } // if
        node->left = probe;
        node->data = 1;

        Node* tmp = node;
        while (match_char(ptr, ';'))
        {
            node->data += 1;
            tmp->right = create(NODE_COMPOUND_VARIANT);
            if (tmp->right == &ALLOCATION_ERROR)
            {
                return allocation_error(node);
            } // if
            tmp = tmp->right;
            tmp->ptr = *ptr;

            probe = variant(ptr);
            if (is_error(probe))
            {
                return error(node, probe, node->ptr, "while matching a compound variant");
            } // if
            tmp->left = probe;
        } // while

        if (!match_char(ptr, ']'))
        {
            return error(node, NULL, *ptr, "expected: ']'");
        } // if

        return node;
    } // if

    Node* const node = variant(ptr);
    if (is_error(node))
    {
        return error(NULL, node, err, "while matching an allele");
    } // if
    return node;
} // allele


size_t
print(FILE*             stream,
      char const* const str,
      Node const* const node)
{
    if (node != NULL)
    {
        Node const* tmp = NULL;
        size_t res = 0;
        switch (node->type)
        {
            case NODE_ALLOCATION_ERROR:
                return fprintf(stream, "%*s" ANSI_BOLDGREEN "^ " ANSI_BOLDMAGENTA "error: " ANSI_BOLDWHITE "%s\n" ANSI_RESET, (int) (node->ptr - str), "", node->ptr);
            case NODE_ERROR:
                return print(stream, str, node->right) +
                       fprintf(stream, "%*s" ANSI_BOLDGREEN "^ " ANSI_BOLDMAGENTA "error: " ANSI_BOLDWHITE "%s\n" ANSI_RESET, (int) (node->ptr - str), "", node->left->ptr);
            case NODE_ERROR_CONTEXT:
                return 0;
            case NODE_UNKNOWN:
                return fprintf(stream, ANSI_CYAN "?" ANSI_RESET);
            case NODE_NUMBER:
                return fprintf(stream, ANSI_CYAN "%zu" ANSI_RESET, node->data);
            case NODE_SEQUENCE:
                return fprintf(stream, ANSI_BOLDWHITE "%.*s" ANSI_RESET, (int) node->data, node->ptr);
            case NODE_IDENTIFIER:
                return fprintf(stream, ANSI_BOLDWHITE "%.*s" ANSI_RESET, (int) node->data, node->ptr);
            case NODE_REFERENCE:
                res = print(stream, str, node->left);
                if (node->right != NULL)
                {
                    res += fprintf(stream, ANSI_MAGENTA "(" ANSI_RESET) +
                           print(stream, str, node->right) +
                           fprintf(stream, ANSI_MAGENTA ")" ANSI_RESET);
                } // if
                return res;
            case NODE_DESCRIPTION:
                res = print(stream, str, node->left) +
                      fprintf(stream, ANSI_MAGENTA ":" ANSI_RESET);
                if (node->data != 0)
                {
                    res += fprintf(stream, ANSI_BOLDWHITE "%c" ANSI_MAGENTA "." ANSI_RESET, (char) node->data);
                } // if
                return res + print(stream, str, node->right);
            case NODE_OFFSET:
                if (node->data == NODE_POSITIVE_OFFSET)
                {
                    return fprintf(stream, ANSI_MAGENTA "+" ANSI_RESET) +
                           print(stream, str, node->left);
                } // if
                return fprintf(stream, ANSI_MAGENTA "-" ANSI_RESET) +
                       print(stream, str, node->left);
            case NODE_POINT:
                if (node->data == NODE_DOWNSTREAM)
                {
                    return fprintf(stream, ANSI_MAGENTA "*" ANSI_RESET) +
                           print(stream, str, node->left) +
                           print(stream, str, node->right);
                } // if
                if (node->data == NODE_UPSTREAM)
                {
                    return fprintf(stream, ANSI_MAGENTA "-" ANSI_RESET) +
                           print(stream, str, node->left) +
                           print(stream, str, node->right);
                } // if
                return print(stream, str, node->left) + 
                       print(stream, str, node->right);
            case NODE_UNCERTAIN_POINT:
                return fprintf(stream, ANSI_MAGENTA "(" ANSI_RESET) +
                       print(stream, str, node->left) +
                       fprintf(stream, ANSI_MAGENTA "_" ANSI_RESET) +
                       print(stream, str, node->right) +
                       fprintf(stream, ANSI_MAGENTA ")" ANSI_RESET);
            case NODE_RANGE:
                return print(stream, str, node->left) +
                       fprintf(stream, ANSI_MAGENTA "_" ANSI_RESET) +
                       print(stream, str, node->right);
            case NODE_LENGTH:
                return fprintf(stream, ANSI_MAGENTA "(") +
                       print(stream, str, node->left) +
                       fprintf(stream, ANSI_MAGENTA ")");
            case NODE_INSERT:
                res = print(stream, str, node->left);
                if (node->data == NODE_INVERTED)
                {
                    res += fprintf(stream, ANSI_GREEN "inv" ANSI_RESET);
                } // if
                if (node->right != NULL)
                {
                    res += fprintf(stream, ANSI_MAGENTA "[" ANSI_RESET) +
                           print(stream, str, node->right) +
                           fprintf(stream, ANSI_MAGENTA "]" ANSI_RESET);
                } // if
                return res;
            case NODE_COMPOUND_INSERT:
                res = fprintf(stream, ANSI_MAGENTA "[" ANSI_RESET) +
                      print(stream, str, node->left);
                tmp = node->right;
                while (tmp != NULL)
                {
                    res += fprintf(stream, ANSI_MAGENTA ";") +
                           print(stream, str, tmp->left);
                    tmp = tmp->right;
                } // while
                return res + fprintf(stream, ANSI_MAGENTA "]");
            case NODE_SUBSTITUTION:
                return print(stream, str, node->left) +
                       fprintf(stream, ANSI_GREEN ">" ANSI_RESET) +
                       print(stream, str, node->right);
            case NODE_REPEAT:
                return print(stream, str, node->left) +
                       fprintf(stream, ANSI_MAGENTA "[" ANSI_RESET) +
                       print(stream, str, node->right) + 
                       fprintf(stream, ANSI_MAGENTA "]" ANSI_RESET);
            case NODE_COMPOUND_REPEAT:
                tmp = node;
                res = 0;
                while (tmp != NULL)
                {
                    res += print(stream, str, tmp->left);
                    tmp = tmp->right;
                } // while
                return res;
            case NODE_DELETION:
                return fprintf(stream, ANSI_GREEN "del") +
                       print(stream, str, node->left);
            case NODE_DELETION_INSERTION:
                return fprintf(stream, ANSI_GREEN "del" ANSI_RESET) +
                       print(stream, str, node->left) +
                       fprintf(stream, ANSI_GREEN "ins" ANSI_RESET) +
                       print(stream, str, node->right);
            case NODE_INSERTION:
                return fprintf(stream, ANSI_GREEN "ins" ANSI_RESET) +
                       print(stream, str, node->left);
            case NODE_DUPLICATION:
                return fprintf(stream, ANSI_GREEN "dup" ANSI_RESET) +
                       print(stream, str, node->left);
            case NODE_CONVERSION:
                return fprintf(stream, ANSI_GREEN "con" ANSI_RESET) +
                       print(stream, str, node->left);
            case NODE_INVERSION:
                return fprintf(stream, ANSI_GREEN "inv" ANSI_RESET) +
                       print(stream, str, node->left);
            case NODE_EQUAL:
                return fprintf(stream, ANSI_GREEN "=" ANSI_RESET) +
                       print(stream, str, node->left);
            case NODE_SLICE:
                return 0;
            case NODE_VARIANT:
                return print(stream, str, node->left) +
                       print(stream, str, node->right);
            case NODE_COMPOUND_VARIANT:
                res = fprintf(stream, ANSI_MAGENTA "[" ANSI_RESET) +
                      print(stream, str, node->left);
                tmp = node->right;
                while (tmp != NULL)
                {
                    res += fprintf(stream, ANSI_MAGENTA ";") +
                           print(stream, str, tmp->left);
                    tmp = tmp->right;
                } // while
                return res + fprintf(stream, ANSI_MAGENTA "]");
        } // switch
    } // if
    return 0;
} // print


int
HGVS_parse(char const* const str)
{
    char const* ptr = str;

    Node* node = description(&ptr);
    if (*ptr != '\0' && !is_error(node))
    {
        node = error(node, NULL, ptr, "unmatched input");
    } // if

    fprintf(stderr, "%s\n", str);
    print(stderr, str, node);
    fprintf(stderr, "\n");

    if (node == NULL || is_error(node))
    {
        fprintf(stderr, ANSI_BOLDRED "failed.\n" ANSI_RESET);
        destroy(node);
        return 1;
    } // if

    fprintf(stderr, ANSI_BOLDGREEN "accepted.\n" ANSI_RESET);
    destroy(node);
    return 0;
} // HGVS_parse
