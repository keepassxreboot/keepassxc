/*
Part of this code come from zxcvbn-c example.
Copyright (c) 2015, Tony Evans
Copyright (c) 2016, KeePassXC Team

See zxcvbn/zxcvbn.cpp for complete COPYRIGHT Notice
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "zxcvbn/zxcvbn.h"

/* For pre-compiled headers under windows */
#ifdef _WIN32
#ifndef __MINGW32__
#include "stdafx.h"
#endif
#endif

static void calculate(const char *pwd, int advanced)
{
    double e;
    int len = strlen(pwd);
    if (advanced == 0){
      e = ZxcvbnMatch(pwd, 0, 0);
      printf("Pass '%s' \tLength %d\tEntropy %.3f\tLog10 %.3f\n", pwd, len, e, e * 0.301029996);
    } else {
      int ChkLen;
      ZxcMatch_t *info, *p;
      double m = 0.0;
      e = ZxcvbnMatch(pwd, 0, &info);
      for(p = info; p; p = p->Next) {
          m += p->Entrpy;
      }
      m = e - m;
      printf("Pass '%s' \tLength %d\tEntropy %.3f\tLog10 %.3f\n  Multi-word extra bits %.1f\n", pwd, len, e, e * 0.301029996, m);
      p = info;
      ChkLen = 0;
      while(p) {
        int n;
        switch(static_cast<int>(p->Type))
        {
            case BRUTE_MATCH:                     printf("  Type: Bruteforce     ");   break;
            case DICTIONARY_MATCH:                printf("  Type: Dictionary     ");   break;
            case DICT_LEET_MATCH:                 printf("  Type: Dict+Leet      ");   break;
            case USER_MATCH:                      printf("  Type: User Words     ");   break;
            case USER_LEET_MATCH:                 printf("  Type: User+Leet      ");   break;
            case REPEATS_MATCH:                   printf("  Type: Repeated       ");   break;
            case SEQUENCE_MATCH:                  printf("  Type: Sequence       ");   break;
            case SPATIAL_MATCH:                   printf("  Type: Spatial        ");   break;
            case DATE_MATCH:                      printf("  Type: Date           ");   break;
            case BRUTE_MATCH+MULTIPLE_MATCH:      printf("  Type: Bruteforce(Rep)");   break;
            case DICTIONARY_MATCH+MULTIPLE_MATCH: printf("  Type: Dictionary(Rep)");   break;
            case DICT_LEET_MATCH+MULTIPLE_MATCH:  printf("  Type: Dict+Leet(Rep) ");   break;
            case USER_MATCH+MULTIPLE_MATCH:       printf("  Type: User Words(Rep)");   break;
            case USER_LEET_MATCH+MULTIPLE_MATCH:  printf("  Type: User+Leet(Rep) ");   break;
            case REPEATS_MATCH+MULTIPLE_MATCH:    printf("  Type: Repeated(Rep)  ");   break;
            case SEQUENCE_MATCH+MULTIPLE_MATCH:   printf("  Type: Sequence(Rep)  ");   break;
            case SPATIAL_MATCH+MULTIPLE_MATCH:    printf("  Type: Spatial(Rep)   ");   break;
            case DATE_MATCH+MULTIPLE_MATCH:       printf("  Type: Date(Rep)      ");   break;

            default:                printf("  Type: Unknown%d ", p->Type);   break;
        }
        ChkLen += p->Length;
        printf("  Length %d  Entropy %6.3f (%.2f) ", p->Length, p->Entrpy, p->Entrpy * 0.301029996);
        for(n = 0; n < p->Length; ++n, ++pwd) {
          printf("%c", *pwd);
        }
        printf("\n");
        p = p->Next;
      }
      ZxcvbnFreeInfo(info);
      if (ChkLen != len)
          printf("*** Password length (%d) != sum of length of parts (%d) ***\n", len, ChkLen);
    }
}

int main(int argc, char **argv)
{
    printf("KeePassXC Entropy Meter, based on zxcvbn-c.\nEnter your password below or pass it as argv\n");
    printf("  Usage: entropy-meter [-a] [pwd1 pwd2 ...]\n> ");
    int i, advanced;
    if ((argc > 1) && (argv[1][0] == '-') && (!strcmp(argv[1], "-a")))
    {
      advanced = 1;
    }
    i = 2;
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
                calculate(line,advanced);
                printf("> ");
        }
    }
    else
    {
        /* Do the test passwords on the command line */
        for(; i < argc; ++i)
        {
            calculate(argv[i],advanced);
        }
    }
    return 0;
}
