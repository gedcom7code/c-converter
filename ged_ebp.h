/**
 * The GEDCOM EVent-Base Parser header file.
 * 
 * This file defines the components of an event-based parser for GEDCOM.
 * The idea is to provide for arbitrary file sizes by keeping a minimal
 * amount of data resident in memory, while providing a more processed
 * version of the GEDCOM transmission than the serialization provides.
 * 
 * The following guarantees are maintained:
 * 
 * - There are the same number of `GED_START` and `GED_END~ events.
 * - The first event is a `GED_START` (with `data` `"HEAD"`)
 * - The last event is a `GED_END` (with `data` `"TRLR"`)
 * - `GED_ANCHOR` only occurs immediately following `GED_START`.
 * - Between any given pair of `GED_START` events, there may be either
 *   (a single `GED_POINTER`) or (any number of `GED_TEXT` and 
 *   `GED_LINBREAK`), but not both.
 * 
 * This file and all of its contents was authored by Luther Tychonievich
 * and has been released into the public domain by its author.
 */
#pragma once

typedef enum {
    GED_UNUSED = 0,
    
    /**
     * `GED_START` is used to indicate a new structure. Its `data` is
     * the structure's type (tag or URI).
     */
    GED_START,
    /**
     * `GED_END` is used to indicate the end of a structure, and is
     * provided after the `GED_END`s of all its substructures.
     * It has no `data`.
     */
    GED_END,
    /**
     * `GED_ANCHOR` is emitted after a `GED_START` and before any other 
     * events if the structure has a cross-reference identifier and can
     * be pointed to. Its `data` is that cross-reference identifier.
     */
    GED_ANCHOR,
    /**
     * `GED_POINTER` is a pointer-type payload. It's `data` is the
     * cross-reference identifier of the pointed-two structure, which
     * is defined by a `GED_ANCHOR` either before or after this event.
     */
    GED_POINTER,
    /**
     * `GED_TEXT` is a portion of a text-type payload. A single payload
     * may be broken up into the `data` of multiple `GED_TEXT` events.
     */
    GED_TEXT,
    /**
     * `GED_LINEBREAK` indicates a new line within a text-type payload.
     * It is given its own event type to support all CR/LF formats.
     * It has no `data`.
     */
    GED_LINEBREAK,
    /**
     * `GED_EOF` indicates the end of a transmission
     */
    GED_EOF,
    /**
     * `GED_RECORD` indicates a full parsed record stored as a linked
     * tree-like structure.
     */
    GED_RECORD,
    /**
     * `GED_ERROR` indicates an error occurred; see data for message
     */
    GED_ERROR,
} GedEventType;

typedef enum {
    /**
     * GED_OWNS_DATA means the data has been allocated by this
     * GedEvent and should be freed when this GedEvent is freed.
     */
    GED_OWNS_DATA = 1,
    /**
     * GED_TYPE_IS_URI means the data is a URI, not a tag.
     */
    GED_TYPE_IS_URI = 2,
    
    GED_FIRST_USER_DEFINED_FLAG = 4,
} GedFlags;

typedef struct GedStructure_t GedStructure;

typedef struct {
    GedEventType type;
    int flags;
    union {
        char *data;
        GedStructure *record;
    };
} GedEvent;

/**
 * The structure is currently organized using GedEvents  with the
 * potential of an anchor at every level to simplify code and avoid
 * having to worry about what's `malloc`ed and what's not. This is 
 * definitely inefficient as only records may have IDs and we're keeping
 * at least two type fields and probably 3 flags we don't need.
 */
struct GedStructure_t {
    GedEvent tag; // a GED_START event
    GedEvent anchor; // a GED_ANCHOR event
    GedEvent payload; // either GED_TEXT or GED_POINTER
    struct GedStructure_t *child;   // first child
    struct GedStructure_t *sibling; // next sibling
};

/// frees `data` and sets type and sets all `GedEvent` bytes to 0
void ged_destroy_event(GedEvent *evt);

/// recursively frees all structures in a record
void ged_destroy_structure(GedStructure *r);


/**
 * Never intended for direct use. Each emitter will have additional
 * state after the function pointer, but that state is irrelevant to
 * how pipeline filters emit.
 * 
 * See `struct ged_event_stage_stack` for an example emitter.
 * See any `.c` file in the `pipeline` folder for example functions
 * that use this template without understanding of the actual struct
 * type passed.
 */
typedef struct GedEmitterTemplate_t {
    void (*emit)(struct GedEmitterTemplate_t *self, GedEvent event);
} GedEmitterTemplate;


/**
 * The type of the callbacks used for each step of GEDCOM conversion.
 * A minimal example callback is
 * 
 * ```
 * void no_op(GedEvent event, GedEmitterTemplate *emitter, void *state) {
 *     emitter->emit(emitter, event);
 * }
 * ```
 * 
 * The `state` argument will be the same each time the callback is
 * called on a single input stream. If `emit` is not called, the event
 * is lost and not passed on to the next callback. If `emit` is called
 * more than once, a single event turns into several events in that
 * order.
 */
typedef void (*GedFilterFunc)(GedEvent *event, GedEmitterTemplate *emitter, void *state);

/**
 * A callback type for initializing the `void *state` parameter of 
 * a GedFilterFunc before the first invocation on a new dataset,
 * and the corresponding destructor callback.
 */
typedef void *(*GedFilterStateMaker)();
typedef void (*GedFilterStateFreer)(void *);


/** a helper for changing data in an event to a non-malloced string */
void changePayloadToConst(GedEvent *e, const char *val);
/** a helper for changing data in an event to a malloced string */
void changePayloadToDynamic(GedEvent *e, char *val);


void _show_event(const GedEvent *evt); // debugging helper

