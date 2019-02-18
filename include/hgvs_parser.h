#if !defined(__hgvs_parser_h__)
#define __hgvs_parser_h__


#include <stdlib.h>


static size_t const INVERTED = -1;


typedef struct HGVS_Node
{
    enum HGVS_Node_Type
    {
        HGVS_Node_number,
        HGVS_Node_position_uncertain,
        HGVS_Node_offset_negative,
        HGVS_Node_offset_positive,
        HGVS_Node_point,
        HGVS_Node_point_downstream,
        HGVS_Node_point_upstream,
        HGVS_Node_uncertain,
        HGVS_Node_location,
        HGVS_Node_range,
        HGVS_Node_substitution,
        HGVS_Node_repeated,
        HGVS_Node_repeated_range,
        HGVS_Node_insertion,
        HGVS_Node_inserted,
        HGVS_Node_deletion,
        HGVS_Node_duplication,
        HGVS_Node_inversion,
        HGVS_Node_conversion,
        HGVS_Node_equal,
        HGVS_Node_deletion_insertion,
        HGVS_Node_variant
    } HGVS_Node_Type;

    enum HGVS_Node_Type type;
    struct HGVS_Node*   left;
    struct HGVS_Node*   right;

    union
    {
        size_t value;
        struct
        {
            char deleted;
            char inserted;
        } substitution;
    } data;
    char const*         ptr;
} HGVS_Node;


HGVS_Node*
HGVS_Node_destroy(HGVS_Node* node);


HGVS_Node*
HGVS_parse(char const* const str);


#endif
