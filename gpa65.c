/*
gpa65.c
GPA symbol file generator for CC65 debug info

MIT License

Copyright (c) 2023 X-Microsystems

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dbginfo.h"
#include "gpa.h"



/*****************************************************************************/
/*                             Argument handling                             */
/*****************************************************************************/



typedef struct argReturn argReturn;
struct argReturn {
    char            status;         /* Returns 0 on success */
    char            ignoreWarnings; /* Ignore CC65 data warnings */
    char            ignoreErrors;   /* Ignore CC65 data errors */
    const char*     inFile;         /* Pointer to the input file string */
    const char*     outFile;        /* Pointer to the output file string */
    char            printSegments;
    char            printScopes;
    char            printLabels;
    char            printLines;
};



static void printHelp () {
    printf("gpa65 v1.0\n");
    printf("Usage: gpa65 [options] INPUT.dbg OUTPUT.sym\n");
    printf("Convert cc65 debug data to GPA symbol files for logic analyzers.\n\n");
    printf("Program options:\n");
    printf("  -w        Ignore source data warnings\n");
    printf("  -e        Ignore source data errors\n");
    printf("  --help    Display this message and exit\n\n");
    printf("Output options (default all):\n");
    printf("  -s        Print Segments  (Sections)\n");
    printf("  -f        Print Scopes    (Functions)\n");
    printf("  -u        Print Labels    (User)\n");
    printf("  -l        Print Lines     (Source lines)\n");
}



static argReturn findArgs(int argc, char* argv[]) {
    argReturn r = {
        .status = 1,
        .printSegments  = 0,
        .printScopes    = 0,
        .printLabels    = 0,
        .printLines     = 0
    };

    /* Check for the --help flag (overrides all) */
    if(argc > 1) {
        for(int i = 1; i < argc; i++) {
            if(strcmp(argv[i], "--help") == 0) {
                printHelp();
                exit(1);
            }
        }
    }

    if(argc < 3) {
        printf("Error: Not enough arguments.\n");
        printHelp();
        exit(1);
    }

    /* Check for arguments in filenames */
    r.inFile = argv[argc - 2];
    r.outFile = argv[argc - 1];
    if(*r.inFile == '-' || *r.outFile == '-') {
        printf("Error: Missing filename.\n");
        printHelp();
        exit(1);
    }

    /* Read flags */
    int flags = 0;
    for(int i = 1; i < argc - 2; i++) {
        if(strcmp(argv[i], "-s") == 0) {
            r.printSegments = 1;
            flags++;
        } else if(strcmp(argv[i], "-f") == 0) {
            r.printScopes = 1;
            flags++;
        } else if(strcmp(argv[i], "-u") == 0) {
            r.printLabels = 1;
            flags++;
        } else if(strcmp(argv[i], "-l") == 0) {
            r.printLines = 1;
            flags++;
        } else if(strcmp(argv[i], "-w") == 0) {
            r.ignoreWarnings = 1;
        } else if(strcmp(argv[i], "-e") == 0) {
            r.ignoreErrors = 1;
        } else {
            printf("Error: Unrecognized arguments.\n");
            printHelp();
            exit(1);
        }
    }
    if(flags == 0) {
        printf("No output flags specified; defaulting to all.\n");
        r.printSegments  = 1;
        r.printScopes    = 1;
        r.printLabels    = 1;
        r.printLines     = 1;
    }

    r.status = 0;
    return r;
}



/*****************************************************************************/
/*                            Debug file handling                            */
/*****************************************************************************/


/* Debug file data */
static cc65_dbginfo Info = 0;



/* Error and warning counters */
static unsigned FileErrors = 0;
static unsigned FileWarnings = 0;



static void FileError (const cc65_parseerror* Info)
/* Callback function - is called in case of errors */
{
    /* Output a message */
    printf("%s:%s(%lu): %s",
           Info->type? "Error" : "Warning",
           Info->name,
           (unsigned long) Info->line,
           Info->errormsg);

    /* Bump the counters */
    switch (Info->type) {
    case CC65_WARNING:
        ++FileWarnings;
        break;
    default:
        ++FileErrors;
        break;
    }
}



/*****************************************************************************/
/*                               Main Function                               */
/*****************************************************************************/



int main(int argc, char *argv[]) {
    argReturn opts = findArgs(argc, argv);

    /* Open the debug info file */
    Info = cc65_read_dbginfo(opts.inFile, FileError);
    if (FileErrors > 0) {
        printf("File loaded with %u errors\n", FileErrors);
        if(opts.ignoreErrors == 0) exit(1);
        printf("-e: Ignoring source data errors.");
    } else if (FileWarnings > 0) {
        printf("File loaded with %u warnings\n", FileWarnings);
        if(opts.ignoreWarnings == 0) exit(1);
        printf("-w: Ignoring source data warnings.");
    }

    /* Open the output file */
    FILE* f = fopen(opts.outFile,"w");
    if(f == NULL) {
        printf("Error opening %s for write.", opts.outFile);
        exit(1);
    }

    /* Write the output file */
    fprintf(f, "### GPA symbol file for %s ###\r\n\r\n", opts.inFile);
    if(opts.printSegments == 1) gpa_print_segments(f, Info);
    if(opts.printScopes == 1) gpa_print_scopes(f, Info);
    if(opts.printLabels == 1) gpa_print_labels(f, Info);
    if(opts.printLines == 1) gpa_print_sources(f, Info);

    fclose(f);
    cc65_free_dbginfo(Info);

    return 0;
}
