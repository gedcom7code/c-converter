/**
 * The GEDCOM EVent-Base Parser -- the Event to GEDCOM file toolchain.
 * 
 * This file and all of its contents was authored by Luther Tychonievich
 * and has been released into the public domain by its author.
 */
#pragma once


#include <stdio.h>  // FILE
#include "ged_ebp.h"

#define GED_ENDL "\n"

/**
 * GedEventSink -- the last step in a filter chain.
 * 
 * Dumps the events as a GEDCOM stream. It performs no validation,
 * assumes all IDs and tags are already handled, 
 * and uses `GED_ENDL` for line terminators.
 */
typedef struct {
    FILE *dest;
    int level;
    GedEvent last; 
} GedEventSinkState;

GedEventSinkState *gedEventSink_create(FILE *out);
void gedEventSink_free(GedEventSinkState *state);

/**
 * Consumes all events, printing out as GEDCOM
 */
void gedEventSinkFunc(GedEvent evt, GedEventSinkState *state);
