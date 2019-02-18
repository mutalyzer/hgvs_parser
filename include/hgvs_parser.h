#if !defined(__hgvs_parser_h__)
#define __hgvs_parser_h__


#include <stdlib.h>


typedef struct HGVS_Node
{
    enum HGVS_Node_Type
    {
        HGVS_Node_upstream,
        HGVS_Node_downstream,
        HGVS_Node_number,
        HGVS_Node_position_uncertain,
        HGVS_Node_offset_negative,
        HGVS_Node_offset_positive,
        HGVS_Node_point,
        HGVS_Node_uncertain,
        HGVS_Node_location,
        HGVS_Node_range,
        HGVS_Node_insertion,
        HGVS_Node_deletion,
        HGVS_Node_variant
    } HGVS_Node_Type;

    enum HGVS_Node_Type type;
    struct HGVS_Node*   child[3];
    size_t              data;
    char const*         ptr;
} HGVS_Node;


HGVS_Node*
HGVS_Node_destroy(HGVS_Node* node);


HGVS_Node*
HGVS_parse(char const* const str);


#endif
