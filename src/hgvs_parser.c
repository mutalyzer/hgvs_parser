#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#include "../include/hgvs_parser.h"
#include "../include/hgvs_interface.h"
#include "../include/lexer.h"


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
        return error(node, error(NULL, NULL, *ptr, "expected: ':'"), node->ptr, "while matching a description");
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
            return error(node, NULL, *ptr, "expected an offset");
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
        return error(node, probe, node->ptr, "while matching an exact point");
    } // if
    node->left = probe;

    probe = offset(ptr);
    if (is_error(probe))
    {
        return error(node, probe, node->ptr, "while matching an exact point");
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
            return error(node, error(NULL, NULL, *ptr, "expected an exact point (start)"), node->ptr, "while matching an uncertain point");
        } // if
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching an uncertain point");
        } // if
        node->left = probe;

        if (!match_char(ptr, '_'))
        {
            return error(node, error(NULL, NULL, *ptr, "expected: '_'"), node->ptr, "while matching an uncertain point");
        } // if

        probe = point(ptr);
        if (probe == NULL)
        {
            return error(node, error(NULL, NULL, *ptr, "expected an exact point (end)"), node->ptr, "while matching an uncertain point");
        } // if
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching an uncertain point");
        } // if
        node->right = probe;

        if (!match_char(ptr, ')'))
        {
            return error(node, error(NULL, NULL, *ptr, "expected: ')'"), node->ptr, "while matching an uncertain point");
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
        return node;
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
            return error(NULL, node, err, "while matching an exact point");
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
            return error(node, error(NULL, NULL, *ptr, "expected a point (exact or uncertain)"), err, "while matching a location (range)");
        } // if
        if (is_error(probe))
        {
            return error(node, probe, err, "while matching a location (range)");
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
        return error(NULL, NULL, *ptr, "a repeat number");
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
            return error(node, error(NULL, NULL, *ptr, "expected a sequence"), node->ptr, "while matching a substitution");
        } // if
        node->right = probe;

        return node;
    } // if

    probe = repeated(ptr);
    if (probe == NULL)
    {
        return error(node, NULL, *ptr, "expected a substitution or repeat number");
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
            return error(node, NULL, *ptr, "expected a length");
        } // if
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching a length");
        } // if
        node->left = probe;

        if (!match_char(ptr, ')'))
        {
            return error(node, error(NULL, NULL, *ptr, "expected: ')'"), node->ptr, "while matching a length");
        } // if
        return node;
    } // if

    return unmatched(node);
} // length


static Node*
length_or_unknown_or_number(char const** const ptr)
{
    Node* node = length(ptr);
    if (is_error(node))
    {
        return node;
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
    Node* node = sequence(ptr);
    if (node == NULL)
    {
        node = length_or_unknown_or_number(ptr);
        if (node == NULL)
        {
            return unmatched(NULL);
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

        node->data = 0;
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
        return probe;
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
        return error(node, probe, node->ptr, "while matching an inserted part");
    } // if

    if (probe == NULL)
    {
        probe = location_or_length(ptr);
        if (probe == NULL)
        {
            return unmatched(node);
        } // if
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching an inserted part");
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
        return error(node, probe, node->ptr, "while matching an inserted part");
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
        node->ptr = *ptr - 1;

        Node* probe = insert(ptr);
        if (probe == NULL)
        {
            return error(node, error(NULL, NULL, *ptr, "expected an inserted part"), node->ptr, "while matching a compound insertion");
        } // if
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching a compound insertion");
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
                return error(node, error(NULL, NULL, *ptr, "expected an inserted part"), node->ptr, "while matching a compound insertion");
            } // if
            if (is_error(probe))
            {
                return error(node, probe, node->ptr, "while matching a compound insertion");
            } // if
            tmp->left = probe;
        } // while

        if (!match_char(ptr, ']'))
        {
            return error(node, error(NULL, NULL, *ptr, "expected: ']'"), node->ptr, "while matching a compound insertion");
        } // if

        return node;
    } // if

    return insert(ptr);
} // inserted


static Node*
substitution(char const** const ptr)
{
    Node* const node = create(NODE_SUBSTITUTION);
    if (node == &ALLOCATION_ERROR)
    {
        return allocation_error(NULL);
    } // if
    node->ptr = *ptr;

    if (match_char(ptr, '>'))
    {
        Node* const probe = inserted(ptr);
        if (probe == NULL)
        {
            return error(node, NULL, *ptr, "expected an inserted part");
        } // if
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching a substitution");
        } // if
        node->right = probe;

        return node;
    } // if

    return unmatched(node);
} // substitution


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
        if (probe == NULL)
        {
            return error(node, NULL, *ptr, "expected an inserted part");
        } // if
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
        Node* probe = NULL;
        if (**ptr == '[')
        {
            probe = inserted(ptr);
        } // if
        else
        {
            probe = sequence_or_length(ptr);
        } // else
        if (is_error(probe))
        {
            return error(node, probe, node->ptr, "while matching a deletion");
        } // if
        node->left = probe;

        if (match_string(ptr, "ins"))
        {
            node->type = NODE_DELETION_INSERTION;

            probe = inserted(ptr);
            if (probe == NULL)
            {
                return error(node, NULL, *ptr, "expected an inserted part");
            } // if
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
        Node* const probe = inserted(ptr);
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
        Node* probe = inserted(ptr);
        if (probe == NULL)
        {
            return error(node, NULL, *ptr, "expected an inserted part");
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
        Node* const probe = inserted(ptr);
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
        Node* const probe = inserted(ptr);
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
        return error(node, error(NULL, NULL, *ptr, "expected a location"), node->ptr, "while matching a variant");
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

    probe = substitution(ptr);
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
                return error(node, probe, node->ptr, "while matching an allele");
            } // if
            tmp->left = probe;
        } // while

        if (!match_char(ptr, ']'))
        {
            return error(node, error(NULL, NULL, *ptr, "expected: ']'"), node->ptr, "while matching an allele");
        } // if

        return node;
    } // if

    return variant(ptr);
} // allele


size_t
print(FILE*                  stream,
      enum HGVS_Format const fmt,
      char const* const      str,
      Node const* const      node)
{
    if (node != NULL)
    {
        Node const* tmp = NULL;
        size_t res = 0;
        switch (node->type)
        {
            case NODE_ALLOCATION_ERROR:
                 return HGVS_fprintf_error(stream, fmt, 0, node->ptr);
            case NODE_ERROR:
                 return print(stream, fmt, str, node->right) +
                        HGVS_fprintf_error(stream, fmt, node->ptr - str, node->left->ptr);
            case NODE_ERROR_CONTEXT:
                return 0;
            case NODE_UNKNOWN:
                return HGVS_fprintf_unknown(stream, fmt, '?');
            case NODE_NUMBER:
                return HGVS_fprintf_number(stream, fmt, node->data);
            case NODE_SEQUENCE:
            case NODE_IDENTIFIER:
                return HGVS_fprintf_string(stream, fmt, node->ptr, node->data);
            case NODE_REFERENCE:
                if (node->right != NULL)
                {
                    return print(stream, fmt, str, node->left) +
                           HGVS_fprintf_operator(stream, fmt, '(') +
                           print(stream, fmt, str, node->right) +
                           HGVS_fprintf_operator(stream, fmt, ')');
                } // if
                return print(stream, fmt, str, node->left);
            case NODE_DESCRIPTION:
                if (node->data != 0)
                {
                    return print(stream, fmt, str, node->left) +
                           HGVS_fprintf_operator(stream, fmt, ':') +
                           HGVS_fprintf_char(stream, fmt, node->data) +
                           HGVS_fprintf_operator(stream, fmt, '.') +
                           print(stream, fmt, str, node->right);
                } // if
                return print(stream, fmt, str, node->left) +
                       HGVS_fprintf_operator(stream, fmt, ':') +
                       print(stream, fmt, str, node->right);
            case NODE_OFFSET:
                if (node->data == NODE_POSITIVE_OFFSET)
                {
                    return HGVS_fprintf_operator(stream, fmt, '+') +
                           print(stream, fmt, str, node->left);
                } // if
                return HGVS_fprintf_operator(stream, fmt, '-') +
                       print(stream, fmt, str, node->left);
            case NODE_POINT:
                if (node->data == NODE_DOWNSTREAM)
                {
                    return HGVS_fprintf_operator(stream, fmt, '*') +
                           print(stream, fmt, str, node->left) +
                           print(stream, fmt, str, node->right);
                } // if
                if (node->data == NODE_UPSTREAM)
                {
                    return HGVS_fprintf_operator(stream, fmt, '-') +
                           print(stream, fmt, str, node->left) +
                           print(stream, fmt, str, node->right);
                } // if
                return print(stream, fmt, str, node->left) +
                       print(stream, fmt, str, node->right);
            case NODE_UNCERTAIN_POINT:
                return HGVS_fprintf_operator(stream, fmt, '(') +
                       print(stream, fmt, str, node->left) +
                       HGVS_fprintf_operator(stream,fmt, '_') +
                       print(stream, fmt, str, node->right) +
                       HGVS_fprintf_operator(stream, fmt, ')');
            case NODE_RANGE:
                return print(stream, fmt, str, node->left) +
                       HGVS_fprintf_operator(stream, fmt, '_') +
                       print(stream, fmt, str, node->right);
            case NODE_LENGTH:
                return HGVS_fprintf_operator(stream, fmt, '(') +
                       print(stream, fmt, str, node->left) +
                       HGVS_fprintf_operator(stream, fmt, ')');
            case NODE_INSERT:
                res = print(stream, fmt, str, node->left);
                if (node->right != NULL)
                {
                    res += HGVS_fprintf_operator(stream, fmt, '[') +
                           print(stream, fmt, str, node->right) +
                           HGVS_fprintf_operator(stream, fmt, ']');
                } // if
                if (node->data == NODE_INVERTED)
                {
                    res += HGVS_fprintf_variant(stream, fmt, "inv");
                } // if
                return res;
            case NODE_COMPOUND_INSERT:
            case NODE_COMPOUND_VARIANT:
                res = HGVS_fprintf_operator(stream, fmt, '[') +
                      print(stream, fmt, str, node->left);
                tmp = node->right;
                while (tmp != NULL)
                {
                    res += HGVS_fprintf_operator(stream, fmt, ';') +
                           print(stream, fmt, str, tmp->left);
                    tmp = tmp->right;
                } // while
                return res + HGVS_fprintf_operator(stream, fmt, ']');
            case NODE_SUBSTITUTION:
                return print(stream, fmt, str, node->left) +
                       HGVS_fprintf_variant(stream, fmt, ">") +
                       print(stream, fmt, str, node->right);
            case NODE_REPEAT:
                return print(stream, fmt, str, node->left) +
                       HGVS_fprintf_operator(stream, fmt, '[') +
                       print(stream, fmt, str, node->right) +
                       HGVS_fprintf_operator(stream, fmt, ']');
            case NODE_COMPOUND_REPEAT:
                tmp = node;
                res = 0;
                while (tmp != NULL)
                {
                    res += print(stream, fmt, str, tmp->left);
                    tmp = tmp->right;
                } // while
                return res;
            case NODE_DELETION:
                return HGVS_fprintf_variant(stream, fmt, "del") +
                       print(stream, fmt, str, node->left);
            case NODE_DELETION_INSERTION:
                return HGVS_fprintf_variant(stream, fmt, "del") +
                       print(stream, fmt, str, node->left) +
                       HGVS_fprintf_variant(stream, fmt, "ins") +
                       print(stream, fmt, str, node->right);
            case NODE_INSERTION:
                return HGVS_fprintf_variant(stream, fmt, "ins") +
                       print(stream, fmt, str, node->left);
            case NODE_DUPLICATION:
                return HGVS_fprintf_variant(stream, fmt, "dup") +
                       print(stream, fmt, str, node->left);
            case NODE_CONVERSION:
                return HGVS_fprintf_variant(stream, fmt, "con") +
                       print(stream, fmt, str, node->left);
            case NODE_INVERSION:
                return HGVS_fprintf_variant(stream, fmt, "inv") +
                       print(stream, fmt, str, node->left);
            case NODE_EQUAL:
                return HGVS_fprintf_variant(stream, fmt, "=") +
                       print(stream, fmt, str, node->left);
            case NODE_SLICE:
                return 0;
            case NODE_VARIANT:
                return print(stream, fmt, str, node->left) +
                       print(stream, fmt, str, node->right);
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
        node = error(node, error(NULL, NULL, ptr, "unmatched input"), str, "while matching a description");
    } // if

    fprintf(stdout, "%s\n", str);
    print(stdout, HGVS_Format_console, str, node);
    fprintf(stdout, "\n");

    if (node == NULL || is_error(node))
    {
        HGVS_fprintf_failed(stdout);
        destroy(node);
        return 1;
    } // if

    HGVS_fprintf_accept(stdout);
    destroy(node);
    return 0;
} // HGVS_parse
