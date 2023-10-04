#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dbginfo.h"

/* Debug file data */
static cc65_dbginfo             Info = 0;
static const cc65_lineinfo*     lineList;
static const cc65_scopeinfo*    scopeList;
static const cc65_segmentinfo*  segmentList;
static const cc65_sourceinfo*   sourceList;
static const cc65_spaninfo*     spanList;
static const cc65_symbolinfo*   symbolList;

/* Intermediate GPA data */
/* Source Lines */


typedef struct gpa_sourcedata gpa_sourcedata;
struct gpa_sourcedata {
    const char*     source_name;    /* Pointer to the source file's name (within the CC65 data) */
    unsigned        source_line;    /* The line number in the source code file */
    unsigned long   address_start;  /* The associated address */
    cc65_line_type  line_type;      /* Type of line (ASM, Macro, C) */
    unsigned        count;          /* Nesting counter for macros */
};


/* Error and warning counters */
static unsigned FileErrors   = 0;
static unsigned FileWarnings = 0;


/* File names */
static char debugFile[100] = "in.dbg";
static char outFile[100] = "out.sym";


/*****************************************************************************/
/*                            Debug file handling                            */
/*****************************************************************************/


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

/*******************************/
/*     Compare Functions       */
/*******************************/

/* Compare source lines */
/* For each duplicate address, we want to prioritize more useful information. Assembly < C < Macro level 1 < Macro level 2, etc. */
static int compare_sourcedata(const void *a, const void *b) {
    struct gpa_sourcedata *input_a = (gpa_sourcedata*)a;
    struct gpa_sourcedata *input_b = (gpa_sourcedata*)b;
    if (input_a->address_start > input_b->address_start) return 1;                  //Sort by address
    if (input_a->address_start < input_b->address_start) return -1;

    int lineType(cc65_line_type l) {
        switch (l) {
        case CC65_LINE_ASM:
            return 0;

        case CC65_LINE_EXT:
            return 1;

        default:
            return 2;
        }
    }

    if (lineType(input_a->line_type) > lineType(input_b->line_type)) return 1;      //Sort by line type
    if (lineType(input_a->line_type) < lineType(input_b->line_type)) return -1;
    if (input_a->count > input_b->count) return 1;                                  //Sort by macro nesting depth
    if (input_a->count < input_b->count) return -1;
    return 0;
}




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
//    struct DbgInfo InfoRaw = Info;
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


    /** Segments **/
    fprintf(f, "[SECTIONS]\r\n");
    segmentList = cc65_get_segmentlist(Info);
    for(int segmentIndex = 0; segmentIndex < segmentList->count; segmentIndex++) {
        fprintf(f, "%-24s%06x..%06x\r\n",segmentList->data[segmentIndex].segment_name, segmentList->data[segmentIndex].segment_start, (segmentList->data[segmentIndex].segment_start + segmentList->data[segmentIndex].segment_size));
    }
    fprintf(f, "\r\n");

    /** Procedures and defined Scopes **/
    scopeList = cc65_get_scopelist(Info);
    fprintf(f, "[FUNCTIONS]\r\n");
    scopeList = cc65_get_scopelist(Info);
    static unsigned scopeSize = 0;
    for(int scopeIndex = 0; scopeIndex < scopeList->count; scopeIndex++) {
        /* Scope names must be collected from the attached symbol */
        symbolList = cc65_symbol_byid(Info, scopeList->data[scopeIndex].symbol_id);
        /* Only add a scope to the list if it's a procedure type with a valid range and name */
        if(scopeList->data[scopeIndex].scope_type == CC65_SCOPE_SCOPE && scopeList->data[scopeIndex].scope_size > 0 && scopeList->data[scopeIndex].scope_name != '\0') {
            scopeSize = (symbolList->data[0].symbol_value + scopeList->data[scopeIndex].scope_size - 1);
            fprintf(f, "%-24s%06x..%06x\r\n",scopeList->data[scopeIndex].scope_name, symbolList->data[0].symbol_value, scopeSize);
        }
    }
    fprintf(f, "\r\n");



    /** Labels **/
    fprintf(f, "[USER]\r\n");
    symbolList = cc65_symbol_inrange(Info, 0x0000, 0xFFFF);
    for(int symbolIndex = 0; symbolIndex < symbolList->count; symbolIndex++) {

        /* Determine whether the symbol is a scope */
        int scopeSymbol = 0;
        scopeList = cc65_scope_byname(Info, symbolList->data[symbolIndex].symbol_name);
        if(scopeList != 0) {
            for(int scopeIndex = 0; scopeIndex < scopeList->count; scopeIndex++) {
                if(scopeList->data[scopeIndex].symbol_id == symbolList->data[symbolIndex].symbol_id) {
                    scopeSymbol = 1;
                }
            }
        }

        fprintf(f, "%-24s%06lx", symbolList->data[symbolIndex].symbol_name, symbolList->data[symbolIndex].symbol_value);
        if(scopeSymbol == 0 && symbolList->data[symbolIndex].symbol_size > 1) {
            fprintf(f, " %lx", symbolList->data[symbolIndex].symbol_size);    //Don't add a size if the symbol is a scope
        }
        fprintf(f, "\r\n");
    }


    /** Source lines **/
    fprintf(f, "\r\n[SOURCE LINES]");
    sourceList = cc65_get_sourcelist(Info);
    static int lineCount = 0;
    static int lineNumber = 0;

    /* Declare the struct array using the total number of spans. Some lines don't have spans attached, so this won't be completely filled */
    gpa_sourcedata gpaSources[cc65_get_spanlist(Info)->count];

    /* Get the line data we need into a new array that can be sorted */
    for(int sourceIndex = 0; sourceIndex < sourceList->count; sourceIndex++) {
        lineList = cc65_line_bysource(Info, sourceIndex);
        for(int lineIndex = 0; lineIndex < lineList->count; lineIndex++) {
            spanList = cc65_span_byline(Info, lineList->data[lineIndex].line_id);
            for(int spanIndex = 0; spanIndex < spanList->count; spanIndex++) {
//                if(lineList->data[lineIndex].line_type != CC65_LINE_MACRO) {        //Discard macro expansions
                gpaSources[lineNumber] = (gpa_sourcedata) {
                    .source_name =      sourceList->data[sourceIndex].source_name,
                    .source_line =      lineList->data[lineIndex].source_line,
                    .address_start =    spanList->data[spanIndex].span_start,
                    .line_type =        lineList->data[lineIndex].line_type,
                    .count =            lineList->data[lineIndex].count
                };
                lineNumber++;
//                }
            }
        }
    }
    lineCount = lineNumber;

    /* Sort each line by address, with macros sources superseding C, C sources superseding assembly */
    qsort(gpaSources, lineCount, sizeof(gpa_sourcedata), compare_sourcedata);

    /* Output the source lines */
    static char* previousSource = 0;
    for(lineNumber = 0; lineNumber < lineCount; lineNumber++) {
        if(gpaSources[lineNumber - 1].source_name != gpaSources[lineNumber].source_name || lineNumber == 0) {       //Don't print the filename if the previous line was from the same file
            fprintf(f, "\r\nFile: %s\r\n", gpaSources[lineNumber].source_name);
        }
        if(lineNumber < lineCount - 1) {                                                                            //Comment superseded lines, ignore the last line
            if(gpaSources[lineNumber].address_start == gpaSources[lineNumber + 1].address_start) {
                fprintf(f, "#", gpaSources[lineNumber].source_name);
            }
        }
        fprintf(f, "%-10d%x\r\n", gpaSources[lineNumber].source_line, gpaSources[lineNumber].address_start);
    }


    fclose(f);
    cc65_free_dbginfo(Info);

    return 0;
}
