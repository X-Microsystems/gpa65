#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dbginfo.h"
#include "gpa.h"

/* File names */
static char debugFile[100] = "in.dbg";
static char outFile[100] = "out.sym";



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

    strcpy(debugFile, argv[1]);
    strcpy(outFile, argv[2]);
    if(argc != 3) {
        printf("GPA symbol file generator for CC65\r\n");
        printf("Usage: %s in.dbg out.sym\r\n", argv[0]);
        exit(0);
    }

    /* Open the debug info file */
    Info = cc65_read_dbginfo(debugFile, FileError);
    /* Print a status */
    if (FileErrors > 0) {
        printf("File loaded with %u errors\r\n", FileErrors);
    } else if (FileWarnings > 0) {
        printf("File loaded with %u warnings\r\n", FileWarnings);
    } else {
        printf("File loaded successfully\r\n");
    }

    /* Open the output file */
    FILE* f = fopen(outFile,"w");
    if(f == NULL) {
        printf("Error opening %s for write.", outFile);
        exit(1);
    }
    fprintf(f, "### GPA Symbol File for %s ###\r\n\r\n", debugFile);

    gpa_print_segments(f, Info);
    gpa_print_scopes(f, Info);
    gpa_print_labels(f, Info);
    gpa_print_sources(f, Info);


    fclose(f);
    cc65_free_dbginfo(Info);

    return 0;
}
