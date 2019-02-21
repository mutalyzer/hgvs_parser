#include <stdio.h>
#include <stdlib.h>


#include "../include/hgvs_parser.h"


int
main(int argc, char* argv[])
{
    if (argc <= 1)
    {
        fprintf(stderr, "Usage: %s string\n", argv[0]);
        return EXIT_FAILURE;
    } // if

    HGVS_Node* root = HGVS_parse(argv[1]);
    if (root == NULL)
    {
        fprintf(stderr, "failed.\n");
        return EXIT_FAILURE;
    } // if

    HGVS_Node_destroy(root);

    fprintf(stderr, "ok.\n");
    return EXIT_SUCCESS;
} // main
