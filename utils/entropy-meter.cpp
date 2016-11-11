/*
Part of this code come from zxcvbn-c example.
Copyright (c) 2015, Tony Evans
Copyright (c) 2016, KeePassXC Team

See zxcvbn/zxcvbn.cpp for complete COPYRIGHT Notice
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include "zxcvbn/zxcvbn.h"

/* For pre-compiled headers under windows */
#ifdef _WIN32
#include "stdafx.h"
#endif

static void calculate(const char *pwd)
{
    double e = ZxcvbnMatch(pwd, 0, 0);
    int len = strlen(pwd);
    printf("Pass '%s' \tLength %d\tEntropy %.3f\tLog10 %.3f\n", pwd, len, e, e * 0.301029996);
}

int main(int argc, char **argv)
{
    printf("KeePassXC Entropy Meter, based on zxcvbn-c.\nEnter your password below or pass it as argv\n> ");
    int i;
    i = 1;
    if (i >= argc)
    {
        /* No test passwords on command line, so get them from stdin */
        char line[500];
        while(fgets(line, sizeof line, stdin))
        {
            /* Drop the trailing newline character */
            for(i = 0; i < static_cast<int>(sizeof line - 1); ++i)
            {
                if (line[i] < ' ')
                {
                    line[i] = 0;
                    break;
                }
            }
            if (line[0])
                calculate(line);
                printf("> ");
        }
    }
    else
    {
        /* Do the test passwords on the command line */
        for(; i < argc; ++i)
        {
            calculate(argv[i]);
        }
    }
    return 0;
}
