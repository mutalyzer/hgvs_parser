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

    if (HGVS_parse(argv[1]) != 0)
    {
        return EXIT_FAILURE;
    } // if

    return EXIT_SUCCESS;
} // main
