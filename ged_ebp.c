/**
 * The GEDCOM EVent-Base Parser core implementation.
 * 
 * This file and all of its contents was authored by Luther Tychonievich
 * and has been released into the public domain by its author.
 */

#include <stdlib.h> // for calloc and free
#include <stdio.h>  // for fprintf

#include "ged_ebp.h"
#include "ged_ebp_parse.h"
#include "ged_ebp_emit.h"
#include "pipeline/config.h"


void ged_destroy_event(GedEvent *evt) {
    if (evt->type == GED_RECORD && (evt->flags & GED_OWNS_DATA)) {
        ged_destroy_structure(evt->record);
    } else if (evt->data && (evt->flags & GED_OWNS_DATA)) {
        free(evt->data);
        evt->data = 0;
    }
    evt->flags = 0;
    evt->type = GED_UNUSED;
}



/// recursively frees all structures in a record
void ged_destroy_structure(GedStructure *s) {
    while (s) {
        if (s->child) ged_destroy_structure(s->child);
        ged_destroy_event(&s->tag);
        ged_destroy_event(&s->anchor);
        ged_destroy_event(&s->payload);
        GedStructure *tmp = s;
        s = s->sibling;
        free(tmp);
    }
}






/**
 * conversion pipeline implementation
 */


struct ged_filter {
    GedFilterFunc passes[2];
    void *state;
};

struct ged_event_stage {
    GedEvent event;  
    size_t stage; // index of next filter to use from pipeline
};

struct ged_event_stage_stack {
    void (*emit)(struct ged_event_stage_stack *self, GedEvent event);
    struct ged_event_stage *stack;
    size_t cap, top, last, stage;
};



void _show_stack_entry(const struct ged_event_stage *entry) {
    fprintf(stderr, "%lu ", entry->stage);
    _show_event(&entry->event);
}
void _show_stack(const struct ged_event_stage_stack *stack) {
    fprintf(stderr, "-------- Stack -------\n");
    for(size_t i=0; i<stack->top; i+=1) {
        fprintf(stderr, "%c", (i==stack->last ? '>' : ' '));
        _show_stack_entry(stack->stack + i);
    }
    fprintf(stderr, "----------------------\n");
}

void _show_event(const GedEvent *event) {
    static const char *event_name[] = {
        "UNUSED",
        "START",
        "END",
        "ANCHOR",
        "POINTER",
        "TEXT",
        "LINEBREAK",
        "EOF",
        "ERROR",
    };
    fprintf(stderr, "%s", event_name[event->type]);
    if (event->data)
        fprintf(stderr, "%c%s (%p)", (event->flags & GED_OWNS_DATA ? '*' : ' '), event->data, (void *)event->data);
    fprintf(stderr, "\n");
}


void ged_event_stage_stack_emit(struct ged_event_stage_stack *self, GedEvent event) {
    if (!self->cap) { 
        self->cap = 4; 
        self->stack = malloc(sizeof(struct ged_event_stage)*self->cap); 
    }
    if (self->top >= self->cap) {
        self->cap *= 2;
        self->stack = realloc(self->stack, sizeof(struct ged_event_stage)*self->cap);
    }
    self->stack[self->top].event.type = event.type;
    self->stack[self->top].event.data = event.data;
    self->stack[self->top].event.flags = event.flags;
    self->stack[self->top].stage = self->stage;
    self->top += 1; 
}
struct ged_event_stage ged_event_stage_stack_pop(struct ged_event_stage_stack *self) {
    if (self->last+1 < self->top) { // need to reorder
        size_t swaps = (self->top-self->last)>>1;
        for(size_t i=0; i<swaps; i+=1) {
            struct ged_event_stage tmp = self->stack[self->last+i];
            self->stack[self->last+i] = self->stack[self->top-i-1];
            self->stack[self->top-i-1] = tmp;
        }
    }
    self->top -= 1;
    self->last = self->top;
    return self->stack[self->top];
}

struct ged_event_stage_stack *ged_event_stage_stack_create() {
    struct ged_event_stage_stack *ans = calloc(1, sizeof(struct ged_event_stage_stack));
    ans->emit = ged_event_stage_stack_emit;
    return ans;
}
void ged_event_stage_stack_free(struct ged_event_stage_stack *x) {
    if (x->stack) free(x->stack);
    free(x);
}


void ged551to700(FILE *from, FILE *to) {
    size_t n = (sizeof(ged_pipeline)/sizeof(ged_pipeline[0]));
    struct ged_filter *pipeline = malloc(sizeof(struct ged_filter)*n);
    
    for(int i=0; i<n; i+=1) {
        pipeline[i].passes[0] = ged_pipeline[i].passes[0];
        pipeline[i].passes[1] = ged_pipeline[i].passes[1];
        pipeline[i].state = ged_pipeline[i].maker();
    }
    
    GedEventSourceState *src = gedEventSource_create(from);
    GedEventSinkState *dst = gedEventSink_create(to);
    struct ged_event_stage_stack *stack = ged_event_stage_stack_create();

    GedEvent e;
    for(int pass=0; pass<2; pass+=1) {
        if (pass > 0) gedEventSource_rewind(src);
        for(;;) { // infinite loop so that GED_EOF does get propogated
            e = gedEventSource_get(src);
            if (e.type == GED_ERROR) break;
            stack->stage = 0;
            stack->emit(stack, e);
            while(stack->top) {
                //_show_stack(stack);
                struct ged_event_stage pair = ged_event_stage_stack_pop(stack);
                while (pair.stage < n && !pipeline[pair.stage].passes[pass]) pair.stage += 1;
                if (pair.stage >= n) {
                    if (pass == 1) gedEventSinkFunc(pair.event, dst);
                    else ged_destroy_event(&pair.event);
                } else {
                    stack->stage = pair.stage + 1;
                    GedFilterFunc func = pipeline[pair.stage].passes[pass];
                    func(
                        &(pair.event),
                        (GedEmitterTemplate *)stack, 
                        pipeline[pair.stage].state
                    );
                }
            }
            if (e.type == GED_EOF) break;
        }
    }
    if (e.type == GED_ERROR)
        gedEventSinkFunc(e, dst); // to show error if there is one


    ged_event_stage_stack_free(stack);
    gedEventSink_free(dst);
    gedEventSource_free(src);

    for(int i=0; i<n; i+=1)
        ged_pipeline[i].freer(pipeline[i].state);
    
    
    free(pipeline);
}






/*
 * To do: make a simpler event chainining without recursion as follows:
 * Each processor is a function that accepts an event and calls an emit
 * callback for each new event in order.
 * The driver maintains a queue?/stack? of events
 * 
 * 
 * XYZ -> a -> (ABC, DEF)
 * ABC -> b -> (GHI, JKL) ... DEF 
 * 
 * Stack of 
 * {GedEvent event, size_t extStage}
 * and array of
 * {void (*)(GedEvent event, void(*emit)(GedEvent)), void *state}
 * but the stack can be private
 */




/** a helper for changing data */
void changePayloadToConst(GedEvent *e, const char *val) {
    if (GED_OWNS_DATA & (e->flags)) {
        free(e->data);
        e->flags &= ~GED_OWNS_DATA;
    }
    e->data = (char *)val;
}

/** a helper for changing data */
void changePayloadToDynamic(GedEvent *e, char *val) {
    if (GED_OWNS_DATA & (e->flags)) free(e->data);
    e->flags |= GED_OWNS_DATA;
    e->data = val;
}



/** Global flag; if nonzero, omit PHRASE when creating ENUMs */
int ged_few_phrases;
/** Global flag; if nonzero, compare xrefs case-insensitively */
int ged_xref_case_insensitive;
