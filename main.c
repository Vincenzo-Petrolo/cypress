#include "cypress.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv)
{
    char c;
    cypress_t cypress;

    /*Initialize the object*/
    init(&cypress);

    while ((c = getopt(argc, argv, "c:x:")) != -1)
    {
        switch (c)
        {
        case 'c': // compress
            printf("Starting compression on %s\n", optarg);
            printf("Saved %d bytes\n", compress(&cypress, optarg));
            exit(EXIT_SUCCESS);
            break;
        case 'x': // extract
            printf("Starting extraction on %s\n", optarg);
            extract(&cypress, optarg);
            exit(EXIT_SUCCESS);
            break;
        default:
            // nothing to do
            break;
        }
    }

    return 0;
}