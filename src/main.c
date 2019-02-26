#include <stdio.h>
#include <stdlib.h>


#include "../include/hgvs.h"


int
main(int argc, char* argv[])
{
    fprintf(stderr, "HGVS parser " HGVS_VERSION_STRING "\n");
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
