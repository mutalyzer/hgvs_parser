#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct
HGVS_Variant
{
    size_t reference_start;
    size_t reference_end;

    size_t observed_start;
    size_t observed_end;

    size_t reference_index : 31;
    size_t observed_index  : 31;
    bool   field_inverted  :  1;
    bool   field_unused    :  1;
}; // HGVS_Variant


static inline bool
match_string(char const** const ptr, char const* const str)
{
    if (strncmp(*ptr, str, strlen(str)) == 0)
    {
        *ptr += strlen(str);
        return true;
    } // if
    return false;
} // match_string


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
is_IUPAC_NT(char const ch)
{
    return ch == 'A' || ch == 'C' || ch == 'G' || ch == 'T' ||
           ch == 'U' || ch == 'N'; // TODO: more IUPAC
} // is_IUPAC_NT


static inline char
match_IUPAC_NT(char const** const ptr)
{
    if (is_IUPAC_NT(**ptr))
    {
        char const ch = **ptr;
        *ptr += 1;
        return ch;
    } // if
    return -1;
} // match_IUPAC_NT


static inline size_t
match_IUPAC_NT_string(char const** const ptr)
{
    size_t len = 0;
    while (is_IUPAC_NT(**ptr))
    {
        *ptr += 1;
        len  += 1;
    } // while
    return len;
} // match_IUPAC_NT_string


static inline bool
is_digit(char const ch)
{
    return (ch == '0' || ch == '1' || ch == '2' || ch == '3' ||
            ch == '4' || ch == '5' || ch == '6' || ch == '7' ||
            ch == '8' || ch == '9');
} // is_digit


static inline size_t
to_size_t(char const ch)
{
    if (ch == '1')
    {
        return 1;
    } // if
    if (ch == '2')
    {
        return 2;
    } // if
    if (ch == '3')
    {
        return 3;
    } // if
    if (ch == '4')
    {
        return 4;
    } // if
    if (ch == '5')
    {
        return 5;
    } // if
    if (ch == '6')
    {
        return 6;
    } // if
    if (ch == '7')
    {
        return 7;
    } // if
    if (ch == '8')
    {
        return 8;
    } // if
    if (ch == '9')
    {
        return 9;
    } // if
    return 0;
} // to_size_t


static inline size_t
match_number(char const** const ptr)
{
    size_t num = 0;
    bool is_number = false;
    while (is_digit(**ptr))
    {
        is_number = true;
        if (num < SIZE_MAX / 10 - 10)
        {
            num *= 10;
            num += to_size_t(**ptr);
        } // if
        *ptr += 1;
    } // while
    if (!is_number || num >= SIZE_MAX / 10 - 10)
    {
        return -1;
    } // if
    return num;
} // match_number


struct Node
{
    enum Type
    {
        Node_position_uncertain,
        Node_potision_exact,
        Node_point,
        Node_point_upstream,
        Node_point_downstream,
        Node_offset_negative,
        Node_offset_positive,
        Node_uncertain,
        Node_range,
        Node_variant,
        Node_equal,
        Node_substitution,
        Node_deletion,
        Node_duplication,
        Node_insertion,
        Node_inserted_range,
        Node_inserted_sequence,
        Node_insertion_compound,
        Node_inversion,
        Node_conversion,
        Node_repeated,
        Node_repeated_range,
        Node_repeated_sequence,
        Node_repeated_sequence_uncertain
    } Type;

    enum Type    type;
    struct Node* left;
    struct Node* right;

    union
    {
        size_t value;
        struct
        {
            char deleted;
            char inserted;
        } substitution;
        struct
        {
            char const* start;
            size_t      length;
        } sequence;
        struct
        {
            size_t start;
            size_t end;
        } range;
        bool inverted;
    } data;
}; // Node


static struct Node*
Node_destroy(struct Node* const node)
{
    if (node != NULL)
    {
        Node_destroy(node->left);
        Node_destroy(node->right);
        fprintf(stderr, "free: %p\n", (void*) node);
        free(node);
    } // if
    return NULL;
} // Node_destroy


static struct Node*
Node_allocation_error(struct Node* const node)
{
    fprintf(stderr, "allocation error\n");
    Node_destroy(node);
    return NULL;
} // Node_allocation_error


static struct Node*
Node_create(enum Type const type)
{
    struct Node* const node = malloc(sizeof(*node));
    fprintf(stderr, "malloc: %p\n", (void*) node);
    if (node == NULL)
    {
        return Node_allocation_error(node);
    } // if
    node->type = type;
    node->left = NULL;
    node->right = NULL;
    return node;
} // Node_create


static struct Node*
position(char const** const ptr)
{
    fprintf(stderr, "position()\n");
    if (match_char(ptr, '?'))
    {
        return Node_create(Node_position_uncertain);
    } // if

    struct Node* const node = Node_create(Node_potision_exact);
    if (node == NULL)
    {
        return Node_allocation_error(node);
    } // if

    node->data.value = match_number(ptr);
    if (node->data.value == (size_t) -1)
    {
        fprintf(stderr, "    invalid number\n");
        return Node_destroy(node);
    } // if
    return node;
} // position


static struct Node*
point(char const** const ptr)
{
    fprintf(stderr, "point()\n");

    enum Type type = Node_point;
    if (match_char(ptr, '-'))
    {
        type = Node_point_upstream;
    } // if
    else if (match_char(ptr, '*'))
    {
        type = Node_point_downstream;
    } // if

    struct Node* const node = Node_create(type);
    if (node == NULL)
    {
        return Node_allocation_error(node);
    } // if

    node->left = position(ptr);
    if (node->left == NULL)
    {
        fprintf(stderr, "    failed\n");
        return Node_destroy(node);
    } // if

    if (match_char(ptr, '-'))
    {
        node->right = position(ptr);
        if (node->right == NULL)
        {
            fprintf(stderr, "    failed\n");
            return Node_destroy(node);
        } // if
        node->right->type = Node_offset_negative;
    } // if
    else if (match_char(ptr, '+'))
    {
        node->right = position(ptr);
        if (node->right == NULL)
        {
            fprintf(stderr, "    failed\n");
            return Node_destroy(node);
        } // if
        node->right->type = Node_offset_positive;
    }// if
    return node;
} // point


static struct Node*
uncertain(char const** const ptr)
{
    fprintf(stderr, "uncertain()\n");
    if (match_char(ptr, '('))
    {
        struct Node* const node = Node_create(Node_uncertain);
        if (node == NULL)
        {
            return Node_allocation_error(node);
        } // if

        node->left = point(ptr);
        if (node->left == NULL)
        {
            fprintf(stderr, "    failed\n");
            return Node_destroy(node);
        } // if

        if (!match_char(ptr, '_'))
        {
            fprintf(stderr, "    failed\n");
            return Node_destroy(node);
        } // if

        node->right = point(ptr);
        if (node->right == NULL)
        {
            fprintf(stderr, "    failed\n");
            return Node_destroy(node);
        } // if

        if (!match_char(ptr, ')'))
        {
            fprintf(stderr, "    failed\n");
            return Node_destroy(node);
        } // if

        return node;
    } // if
    return NULL;
} // uncertain


static struct Node*
point_or_uncertain(char const** const ptr)
{
    fprintf(stderr, "point_or_uncertain()\n");
    struct Node* const node = uncertain(ptr);
    if (node == NULL)
    {
        return point(ptr);
    } // if
    return node;
} // point_or_uncertain


static struct Node*
range(char const** const ptr)
{
    fprintf(stderr, "range()\n");
    struct Node* const node = Node_create(Node_range);
    if (node == NULL)
    {
        return Node_allocation_error(node);
    } // if

    node->left = point_or_uncertain(ptr);
    if (node->left == NULL)
    {
        fprintf(stderr, "    invalid range\n");
        return Node_destroy(node);
    } // if

    if (!match_char(ptr, '_'))
    {
        fprintf(stderr, "    invalid range\n");
        return Node_destroy(node);
    } // if

    node->right = point_or_uncertain(ptr);
    if (node->right == NULL)
    {
        fprintf(stderr, "    invalid range\n");
        return Node_destroy(node);
    }
    return node;
} // range


static struct Node*
location(char const** const ptr)
{
    fprintf(stderr, "location()\n");
    struct Node* probe = uncertain(ptr);
    if (probe != NULL)
    {
        if (match_char(ptr, '_'))
        {
            struct Node* const node = Node_create(Node_range);
            if (node == NULL)
            {
                fprintf(stderr, "    failed\n");
                return Node_destroy(probe);
            } // if
            node->left = probe;
            node->right = point_or_uncertain(ptr);
            if (node->right == NULL)
            {
                fprintf(stderr, "    failed\n");
                return Node_destroy(node);
            } // if
            return node;
        } // if
        return probe;
    } // if

    probe = point(ptr);
    if (probe == NULL)
    {
        return Node_allocation_error(probe);
    } // if

    if (match_char(ptr, '_'))
    {
        struct Node* const node = Node_create(Node_range);
        if (node == NULL)
        {
            fprintf(stderr, "    failed\n");
            return Node_destroy(probe);
        } // if
        node->left = probe;
        node->right = point_or_uncertain(ptr);
        if (node->right == NULL)
        {
            fprintf(stderr, "    failed\n");
            return Node_destroy(node);
        } // if
        return node;
    } // if
    return probe;
} // location


static struct Node*
repeated(char const** const ptr)
{
    if (match_char(ptr, '['))
    {
        struct Node* const node = Node_create(Node_repeated_range);
        if (node == NULL)
        {
            return Node_allocation_error(node);
        } // if

        node->data.value = match_number(ptr);
        if (node->data.value == (size_t) -1)
        {
            fprintf(stderr, "    invalid repeat\n");
            return Node_destroy(node);
        } // if

        if (!match_char(ptr, ']'))
        {
            fprintf(stderr, "invalid repeat\n");
            return Node_destroy(node);
        } // if
        return node;
    } // if

    struct Node* const node = Node_create(Node_repeated_sequence);
    if (node == NULL)
    {
        return Node_allocation_error(node);
    } // if

    node->data.sequence.start  = *ptr;
    node->data.sequence.length = match_IUPAC_NT_string(ptr);
    if (node->data.sequence.length <= 0)
    {
        fprintf(stderr, "    invalid sequence\n");
        return Node_destroy(node);
    } // if

    if (match_char(ptr, '('))
    {
        node->type = Node_repeated_sequence_uncertain;
        node->data.range.start = match_number(ptr);
        if (node->data.range.start == (size_t) -1)
        {
            fprintf(stderr, "    invalid repeat\n");
            return Node_destroy(node);
        } // if
        if (!match_char(ptr, '_'))
        {
            fprintf(stderr, "    invalid repeat\n");
            return Node_destroy(node);
        } // if
        node->data.range.end = match_number(ptr);
        if (node->data.range.end == (size_t) -1)
        {
            fprintf(stderr, "    invalid repeat\n");
            return Node_destroy(node);
        } // if
        if (!match_char(ptr, ')'))
        {
            fprintf(stderr, "    invalid repeat\n");
            return Node_destroy(node);
        } // if
        return node;
    } // if

    if (match_char(ptr, '['))
    {
        node->data.value = match_number(ptr);
        if (node->data.value == (size_t) -1)
        {
            fprintf(stderr, "    invalid repeat\n");
            return Node_destroy(node);
        } // if
        if (!match_char(ptr, ']'))
        {
            fprintf(stderr, "    invalid repeat\n");
            return Node_destroy(node);
        }
        return node;
    } // if

    return NULL;
} // repeated


static struct Node*
substitution(char const** const ptr)
{
    fprintf(stderr, "substitution()\n");
    char const* start =  *ptr;
    size_t const len  = match_IUPAC_NT_string(ptr);

    if (len <= 0)
    {
        fprintf(stderr, "    invalid deleted sequence\n");
        return NULL;
    } // if

    if (match_char(ptr, '>'))
    {
        if (len > 1)
        {
            fprintf(stderr, "    invalid substitution\n");
            return NULL;
        } // if

        struct Node* const node = Node_create(Node_substitution);
        if (node == NULL)
        {
            return Node_allocation_error(node);
        } // if
        node->data.substitution.deleted  = *start;
        node->data.substitution.inserted = match_IUPAC_NT(ptr);
        if (node->data.substitution.inserted == (char) -1)
        {
            fprintf(stderr, "    invalid substituion\n");
            return Node_destroy(node);
        } // if
        return node;
    } // if

    return repeated(ptr);
} // substitution


static struct Node*
insert(char const** const ptr)
{
    fprintf(stderr, "insert()\n");
    struct Node* const probe = range(ptr);
    if (probe != NULL)
    {
        struct Node* const node = Node_create(Node_inserted_range);
        if (node == NULL)
        {
            return Node_allocation_error(probe);
        } // if

        node->left = probe;
        node->data.inverted = match_string(ptr, "inv");
        return node;
    } // if

    struct Node* const node = Node_create(Node_inserted_sequence);
    if (node == NULL)
    {
        return Node_allocation_error(node);
    } // if

    node->data.sequence.start  = *ptr;
    node->data.sequence.length = match_IUPAC_NT_string(ptr);
    if (node->data.sequence.length <= 0)
    {
        fprintf(stderr, "    invalid insertion\n");
        return Node_destroy(node);
    } // if
    return node;
} // insert


static struct Node*
inserted(char const** const ptr)
{
    if (match_char(ptr, '['))
    {
        struct Node* const node = Node_create(Node_insertion_compound);
        if (node == NULL)
        {
            return Node_allocation_error(node);
        } // if

        node->left = insert(ptr);
        if (node->left == NULL)
        {
            fprintf(stderr, "    empty compound insertion\n");
            return Node_destroy(node);
        } // if

        struct Node* tmp = node;
        while (match_char(ptr, ';'))
        {
            tmp->right = Node_create(Node_insertion_compound);
            if (tmp->right == NULL)
            {
                return Node_allocation_error(node);
            } // if
            tmp = tmp->right;
            tmp->left = insert(ptr);
            if (tmp->left == NULL)
            {
                fprintf(stderr, "    invalid inserted\n");
                return Node_destroy(node);
            } // if
        } // while

        if (!match_char(ptr, ']'))
        {
            fprintf(stderr, "    invalid insertion\n");
            return Node_destroy(node);
        } // if

        return node;
    } // if

    struct Node* const node = insert(ptr);
    if (node == NULL)
    {
        fprintf(stderr, "    invalid insertion\n");
        return Node_destroy(node);
    } // if
    return node;
} // inserted


static struct Node*
deletion(char const** const ptr)
{
    fprintf(stderr, "deletetion()\n");
    if (match_string(ptr, "del"))
    {
        struct Node* const node = Node_create(Node_deletion);
        if (node == NULL)
        {
            return Node_allocation_error(node);
        } // if

        node->data.value = match_number(ptr);
        if (node->data.value == (size_t) -1)
        {
            node->data.sequence.start  = *ptr;
            node->data.sequence.length = match_IUPAC_NT_string(ptr);
        } // if

        if (match_string(ptr, "ins"))
        {
            node->right = inserted(ptr);
            if (node->right == NULL)
            {
                fprintf(stderr, "    invalid insertion\n");
                return Node_destroy(node);
            } // if
        } // if
        return node;
    } // if
    return NULL;
} // deletion


static struct Node*
duplication(char const** const ptr)
{
    fprintf(stderr, "duplication()\n");
    if (match_string(ptr, "dup"))
    {
        struct Node* const node = Node_create(Node_duplication);
        if (node == NULL)
        {
            return Node_allocation_error(node);
        } // if

        node->data.value = match_number(ptr);
        if (node->data.value == (size_t) -1)
        {
            node->data.sequence.start  = *ptr;
            node->data.sequence.length = match_IUPAC_NT_string(ptr);
        } // if
        return node;
    } // if
    return NULL;
} // duplication


static struct Node*
insertion(char const** const ptr)
{
    fprintf(stderr, "insertion()\n");
    if (match_string(ptr, "ins"))
    {
        struct Node* const node = Node_create(Node_insertion);
        if (node == NULL)
        {
            return Node_allocation_error(node);
        } // if
        node->right = inserted(ptr);
        if (node->right == NULL)
        {
            fprintf(stderr, "    invalid insertion\n");
            Node_destroy(node);
        } // if
        return node;
    } // if
    return NULL;
} // insertion


static struct Node*
inversion(char const** const ptr)
{
    fprintf(stderr, "inversion()\n");
    if (match_string(ptr, "inv"))
    {
        struct Node* const node = Node_create(Node_inversion);
        if (node == NULL)
        {
            return Node_allocation_error(node);
        } // if

        node->data.value = match_number(ptr);
        if (node->data.value == (size_t) -1)
        {
            node->data.sequence.start  = *ptr;
            node->data.sequence.length = match_IUPAC_NT_string(ptr);
        } // if
        return node;
    } // if
    return NULL;
} // inversion


static struct Node*
conversion(char const** const ptr)
{
    fprintf(stderr, "conversion()\n");
    if (match_string(ptr, "inv"))
    {
        struct Node* const node = Node_create(Node_conversion);
        if (node == NULL)
        {
            return Node_allocation_error(node);
        } // if

        node->right = range(ptr);
        if (node->right == NULL)
        {
            fprintf(stderr, "    invalid range\n");
            return Node_destroy(node);
        } // if
        return node;
    } // if
    return NULL;
} // conversion


static struct Node*
variant(char const** const ptr)
{
    fprintf(stderr, "variant()\n");
    struct Node* const loc = location(ptr);
    if (loc == NULL)
    {
        return Node_allocation_error(loc);
    } // if

    struct Node* const node = Node_create(Node_variant);
    if (node == NULL)
    {
        return Node_allocation_error(loc);
    } // if

    if (match_char(ptr, '='))
    {
        node->left = loc;
        if (loc->type == Node_uncertain)
        {
            fprintf(stderr, "    invalid location\n");
            Node_destroy(node);
        } // if
        node->right = Node_create(Node_equal);
        return node;
    } // if

    node->left = substitution(ptr);
    if (node->left != NULL)
    {
        node->left->left = loc;
        if (loc->type == Node_range)
        {
            fprintf(stderr, "    invalid location\n");
            return Node_destroy(node);
        } // if
        return node;
    } // if

    node->left = deletion(ptr);
    if (node->left != NULL)
    {
        node->left->left = loc;
        return node;
    } // if

    node->left = duplication(ptr);
    if (node->left != NULL)
    {
        node->left->left = loc;
        return node;
    } // if

    node->left = insertion(ptr);
    if (node->left != NULL)
    {
        node->left->left = loc;
        if (loc->type != Node_range)
        {
            fprintf(stderr, "    invalid location\n");
            return Node_destroy(node);
        } // if
        return node;
    } // if

    node->left = inversion(ptr);
    if (node->left != NULL)
    {
        node->left->left = loc;
        if (node->type != Node_range)
        {
            fprintf(stderr, "    invalid location\n");
            return Node_destroy(node);
        } // if
        return node;
    } // if

    node->left = conversion(ptr);
    if (node->left != NULL)
    {
        node->left->left = loc;
        if (node->type != Node_range)
        {
            fprintf(stderr, "    invalid location\n");
            return Node_destroy(node);
        } // if
        return node;
    } // if

    Node_destroy(loc);
    return Node_destroy(node);
} // variant


int
main(int argc, char* argv[])
{
    if (argc <= 1)
    {
        fprintf(stderr, "Usage: ...\n");
        return EXIT_FAILURE;
    } // if

    char const* ptr = argv[1];
    struct Node* root = variant(&ptr);

    if (root == NULL)
    {
        fprintf(stderr, "failed.\n");
        return EXIT_FAILURE;
    } // if
    if (*ptr != '\0')
    {
        fprintf(stderr, "failed: %s\n", ptr);
        return EXIT_FAILURE;
    } // if

    fprintf(stderr, "success\n");

    Node_destroy(root);
    return EXIT_SUCCESS;
} // main
