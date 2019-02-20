#if !defined(__hgvs_parser_h__)
#define __hgvs_parser_h__


#include <stddef.h>


static size_t const HGVS_Node_downstream = 1;
static size_t const HGVS_Node_upstream   = 2;
static size_t const HGVS_Node_negative   = 1;
static size_t const HGVS_Node_positive   = 2;


typedef struct HGVS_Node
{
    enum HGVS_Node_Type
    {
        HGVS_Node_error_allocation,
        HGVS_Node_error,
        HGVS_Node_error_message,
        HGVS_Node_number,
        HGVS_Node_unknown,
        HGVS_Node_point,
        HGVS_Node_offset,
        HGVS_Node_position,
        HGVS_Node_uncertain,
        HGVS_Node_range,
        HGVS_Node_sequence,
        HGVS_Node_variant,
        HGVS_Node_duplication,
        HGVS_Node_inversion,
        HGVS_Node_deletion,
        HGVS_Node_deletion_insertion,
        HGVS_Node_substitution,
        HGVS_Node_inserted,
        HGVS_Node_inserted_compound,
        HGVS_Node_insertion,
        HGVS_Node_range_exact,
        HGVS_Node_conversion,
        HGVS_Node_repeat,
        HGVS_Node_equal,
    } HGVS_Node_Type;

    struct HGVS_Node* left;
    struct HGVS_Node* right;

    enum HGVS_Node_Type type;
    size_t              data;
    char const*         ptr;
} HGVS_Node;


void
HGVS_Node_destroy(HGVS_Node* node);


HGVS_Node*
HGVS_parse(char const* const str);


#endif
