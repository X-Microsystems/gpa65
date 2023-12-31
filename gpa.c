/*
gpa.c
GPA symbol conversion functions

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


const unsigned int columnWidth = 40;

/*****************************************************************************/
/*                                   Labels                                  */
/*****************************************************************************/



void gpa_print_labels(FILE* f, cc65_dbginfo Info) {
    const cc65_scopeinfo*    scopeList;
    const cc65_symbolinfo*   symbolList;

    fprintf(f, "[USER]\r\n");
    symbolList = cc65_symbol_inrange(Info, 0x0000, 0xFFFF);
    for(int symbolIndex = 0; symbolIndex < symbolList->count; symbolIndex++) {
        int column = columnWidth; /* Column counter for output alignment */

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

        /* Prepend the label name for clarity where needed (cheap locals and duplicates */
        const cc65_symbolinfo* symbolDuplicates = cc65_symbol_byname(Info, symbolList->data[symbolIndex].symbol_name);
        if(symbolList->data[symbolIndex].parent_id != CC65_INV_ID) {
            /* If the symbol is a cheap local */
            column -= fprintf(f, "%s/", cc65_symbol_byid(Info, symbolList->data[symbolIndex].parent_id)->data[0].symbol_name);
        } else if(symbolDuplicates->count > 1) {
            /* If the symbol is a duplicate */
            int isDuplicate = 0;
            for(int i = 0; i < symbolDuplicates->count; i++) {
                if(symbolDuplicates->data[i].symbol_id != symbolList->data[symbolIndex].symbol_id && symbolDuplicates->data[i].symbol_type != CC65_SYM_IMPORT) {
                    isDuplicate = 1;
                }
            }

            if(isDuplicate == 1) {
                if(*cc65_scope_byid(Info, symbolList->data[symbolIndex].scope_id)->data[0].scope_name != '\0') {
                    /* If the parent scope has a name, print it */
                    column -= fprintf(f, "%s/", cc65_scope_byid(Info, symbolList->data[symbolIndex].scope_id)->data[0].scope_name);
                } else {
                    /* If the parent scope has no name, print the source file name of the span's parent module instead */
                    column -= fprintf(f, "%s/", cc65_source_byid(Info, cc65_module_byid(Info, cc65_scope_byid(Info, symbolList->data[symbolIndex].scope_id)->data[0].module_id)->data[0].source_id)->data[0].source_name);
                }
            }
        }

        /* Print the name and address */
        fprintf(f, "%-*s %06lX", column, symbolList->data[symbolIndex].symbol_name, symbolList->data[symbolIndex].symbol_value);
        /* Print the size if the symbol is not a scope */
        if(scopeSymbol == 0 && symbolList->data[symbolIndex].symbol_size > 1) {
            fprintf(f, " %X", symbolList->data[symbolIndex].symbol_size);
        }
        fprintf(f, "\r\n");
    }
    fprintf(f, "\r\n");
}



/*****************************************************************************/
/*                                   Scopes                                  */
/*****************************************************************************/



void gpa_print_scopes(FILE* f, cc65_dbginfo Info) {
    const cc65_scopeinfo*    scopeList;
    const cc65_symbolinfo*   symbolList;

    scopeList = cc65_get_scopelist(Info);
    fprintf(f, "[FUNCTIONS]\r\n");
    scopeList = cc65_get_scopelist(Info);
    unsigned scopeSize = 0;
    for(int scopeIndex = 0; scopeIndex < scopeList->count; scopeIndex++) {
        /* Scope names must be collected from the attached symbol */
        symbolList = cc65_symbol_byid(Info, scopeList->data[scopeIndex].symbol_id);
        /* Only add a scope to the list if it's a procedure type with a valid range and name */
        if(scopeList->data[scopeIndex].scope_type == CC65_SCOPE_SCOPE && scopeList->data[scopeIndex].scope_size > 0 && scopeList->data[scopeIndex].scope_name) {
            scopeSize = (symbolList->data[0].symbol_value + scopeList->data[scopeIndex].scope_size - 1);
            fprintf(f, "%-*s %06lX..%06X\r\n", columnWidth, scopeList->data[scopeIndex].scope_name, symbolList->data[0].symbol_value, scopeSize);
        }
    }
    fprintf(f, "\r\n");
}



/*****************************************************************************/
/*                                  Segments                                 */
/*****************************************************************************/


/* Compare segment addresses */
static int compare_segmentdata(const void *a, const void *b) {
    struct cc65_segmentdata *input_a = (cc65_segmentdata*)a;
    struct cc65_segmentdata *input_b = (cc65_segmentdata*)b;
    /* Sort by address */
    if (input_a->segment_start > input_b->segment_start) return 1;
    if (input_a->segment_start < input_b->segment_start) return -1;
    return 0;
}



void gpa_print_segments(FILE* f, cc65_dbginfo Info) {
    cc65_segmentinfo*  segmentList;

    fprintf(f, "[SECTIONS]\r\n");
    /* Get a list of all segments and sort them by address. Generates a compiler warning because the function returns a const type. */
    segmentList = cc65_get_segmentlist(Info);
    qsort(segmentList->data, segmentList->count, sizeof(cc65_segmentdata), compare_segmentdata);

    for(int segmentIndex = 0; segmentIndex < segmentList->count; segmentIndex++) {
        if(segmentList->data[segmentIndex].segment_size > 0 && strcmp(segmentList->data[segmentIndex].segment_name, "NULL") != 0) {
            fprintf(f, "%-*s %06X..%06X\r\n", columnWidth, segmentList->data[segmentIndex].segment_name, segmentList->data[segmentIndex].segment_start, (segmentList->data[segmentIndex].segment_start + (segmentList->data[segmentIndex].segment_size - 1)));
        }
    }
    fprintf(f, "\r\n");
}



/*****************************************************************************/
/*                                Source Lines                               */
/*****************************************************************************/



typedef struct gpa_sourcedata gpa_sourcedata;
struct gpa_sourcedata {
    const char*     source_name;    /* Pointer to the source file's name (within the CC65 data) */
    unsigned        source_line;    /* The line number in the source code file */
    unsigned long   address_start;  /* The associated address */
    cc65_line_type  line_type;      /* Type of line (ASM, Macro, C) */
    unsigned        count;          /* Nesting counter for macros */
};

/* Compare source lines
** For each duplicate address, we want to prioritize more useful information.
** Assembly < C < Macro level 1 < Macro level 2, etc.
*/
static int compare_sourcedata(const void *a, const void *b) {
    struct gpa_sourcedata *input_a = (gpa_sourcedata*)a;
    struct gpa_sourcedata *input_b = (gpa_sourcedata*)b;
    /* Sort by address */
    if (input_a->address_start > input_b->address_start) return 1;
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
    /* Sort by line type */
    if (lineType(input_a->line_type) > lineType(input_b->line_type)) return 1;
    if (lineType(input_a->line_type) < lineType(input_b->line_type)) return -1;
    /* Sort by macro nesting depth */
    if (input_a->count > input_b->count) return 1;
    if (input_a->count < input_b->count) return -1;
    return 0;
}



void gpa_print_sources(FILE* f, cc65_dbginfo Info) {
    const cc65_lineinfo*     lineList;
    const cc65_spaninfo*     spanList;
    const cc65_sourceinfo*   sourceList;

    fprintf(f, "[SOURCE LINES]");
    sourceList = cc65_get_sourcelist(Info);
    int lineCount = 0;
    int lineNumber = 0;

    /* Declare the struct array using the total number of spans. Some lines don't have spans attached, so this won't be completely filled */
    gpa_sourcedata gpaSources[cc65_get_spanlist(Info)->count];

    /* Get the line data we need into a new array that can be sorted */
    for(int sourceIndex = 0; sourceIndex < sourceList->count; sourceIndex++) {
        lineList = cc65_line_bysource(Info, sourceIndex);
        for(int lineIndex = 0; lineIndex < lineList->count; lineIndex++) {
            spanList = cc65_span_byline(Info, lineList->data[lineIndex].line_id);
            for(int spanIndex = 0; spanIndex < spanList->count; spanIndex++) {
                gpaSources[lineNumber] = (gpa_sourcedata) {
                    .source_name =      sourceList->data[sourceIndex].source_name,
                    .source_line =      lineList->data[lineIndex].source_line,
                    .address_start =    spanList->data[spanIndex].span_start,
                    .line_type =        lineList->data[lineIndex].line_type,
                    .count =            lineList->data[lineIndex].count
                };
                lineNumber++;
            }
        }
    }
    lineCount = lineNumber;

    /* Sort each line by address, with macros sources superseding C, C sources superseding assembly */
    qsort(gpaSources, lineCount, sizeof(gpa_sourcedata), compare_sourcedata);

    /* Output the source lines */
    for(lineNumber = 0; lineNumber < lineCount; lineNumber++) {
        /*Don't print the filename if the previous line was from the same file */
        if(gpaSources[lineNumber - 1].source_name != gpaSources[lineNumber].source_name || lineNumber == 0) {
            fprintf(f, "\r\nFile: %s\r\n", gpaSources[lineNumber].source_name);
        }
        /* Comment superseded lines, ignore the last line */
        int column = columnWidth;
        if(lineNumber < lineCount - 1) {
            if(gpaSources[lineNumber].address_start == gpaSources[lineNumber + 1].address_start) {
                column -= fprintf(f, "#");
            }
        }
        fprintf(f, "%-*d %06lX\r\n", column, gpaSources[lineNumber].source_line, gpaSources[lineNumber].address_start);
    }
    fprintf(f, "\r\n");
}
