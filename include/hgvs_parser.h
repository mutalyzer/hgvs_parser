#if !defined(__hgvs_parser_h__)
#define __hgvs_parser_h__


#include <stddef.h>


static size_t const HGVS_Node_downstream = 1;
static size_t const HGVS_Node_upstream   = 2;
static size_t const HGVS_Node_negative   = 1;
static size_t const HGVS_Node_positive   = 2;
static size_t const HGVS_Node_inverted   = 1;
static size_t const HGVS_Node_system_c   = 1;
static size_t const HGVS_Node_system_g   = 2;
static size_t const HGVS_Node_system_m   = 3;
static size_t const HGVS_Node_system_n   = 4;
static size_t const HGVS_Node_system_o   = 5;
static size_t const HGVS_Node_system_r   = 6;


typedef struct HGVS_Node
{
    enum HGVS_Node_Type
    {
        HGVS_Node_error_allocation,
        HGVS_Node_error,
        HGVS_Node_error_context,
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
        HGVS_Node_equal_allele,
        HGVS_Node_variant_list,
        HGVS_Node_reference,
        HGVS_Node_coordinate_system,
        HGVS_Node_length,
        HGVS_Node_repetition
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


size_t
HGVS_write(HGVS_Node const* const node);

#endif
