#pragma once

/**
 * `calendar` :  either NULL (meaning Gregorian) or a calendar name like "JULIAN" or "FRENCH_R"
 * `day` : either 0 (meaning no day given) of day within month
 * `year` : year (always set)
 * `epoch` : either NULL or "BCE"
 */
typedef struct {
    char *calendar;
    int day;
    char *month;
    int year;
    char *epoch;
} GedDate;

/**
 * `d1` : either NULL (only phrase) or `malloc`ed first date
 * `d2` : either NULL (single-date value) or `malloc`ed second date
 * `modifier` : NULL or one of "FROM", "TO", "BET", "BEF", "AFT", "EST", "CAL", "ABT"; "FROM" may and "BET" will contain a `d2`; others will not
 * `phrase` : either NULL or a date phrase; if non-null, `malloc`ed separately from `freeMe`
 * `freeMe` : either NULL or the `malloc`ed region of memory all `char *` pointers reachable from this structure (except `phrase`) point to.
 */
typedef struct {
    GedDate *d1, *d2;
    char *modifier;
    char *phrase;
    void *freeMe;
} GedDateValue;

/**
 * Given a GEDCOM 5.5.1 date payload, returns a GedDateValue.
 * Relatively forgiving of strange spacing and unknown calendars and
 * months; most other errors will result in a `phrase` containing the
 * whole payload.
 */
GedDateValue *gedDateParse551(char *payload);


/**
 * Formats a parsed DateValue for GEDCOM 7.0.
 * Always returned a `malloc`ed string.
 * Does not handle `d->phrase`, only the payload of the DATE structure.
 */
char *gedDatePayload(GedDateValue *d);

