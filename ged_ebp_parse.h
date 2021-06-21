/**
 * The GEDCOM EVent-Base Parser -- the GEDCOM file to Event toolchain.
 * 
 * This file and all of its contents was authored by Luther Tychonievich
 * and has been released into the public domain by its author.
 */

#include <stdlib.h> // for FILE
#pragma once

#include "ged_ebp.h"
#include "ansel2utf8.h"

typedef struct {
    DecodingFileReader *reader;
    int stage; 
    int inLevel, lastLevel;
    char *anchor;
} GedEventSourceState;

/// allocate and initialize reading state
GedEventSourceState *gedEventSource_create(FILE *in);

/// deallocate reading state
void gedEventSource_free(GedEventSourceState *state);

/// parse the input, returning the next event and advancing the file
/// pointer to past it
GedEvent gedEventSource_get(GedEventSourceState *state);

/// reset internal state so _get will return the first event next
void gedEventSource_rewind(GedEventSourceState *state);
