/**
 * This file is not a traditional header in that it includes .c files.
 * That's probably bad practice and should be fixed, but for now to add
 * another filter to the processing pipeline, do the following:
 * 
 * 1. create a file in this folder; e.g. "baz.c"
 * 2. implement a GedFilterFunc, GedFilterStateMaker, and 
 *    GedFilterStateFreer (the last two can be ged_nostate_* if
 *    no state is needed)
 * 3. ensure your function emits events in the right order:
 *    1. GED_START, 
 *    2. (optional GED_ANCHOR)
 *    3. GED_POINTER or (any number of GED_TEXT and GED_LINEBREAK)
 *    4. (any number of other structs from GED_START to GED_END)
 *    5. GED_END
 *    This may require a little state; for example, to add a 
 *    substructure you should note the adding goal when you get the
 *    GED_START but not actually add it until the next GED_END or 
 *    GED_START.
 * 4. #include your .c file below
 * 5. add your entry into the pipeline
 */

#include "nop.c" // ged_nostate_maker, ged_nostate_freer
#include "unconc.c" // ged_longstate_maker, ged_longstate_freer
#include "mergepayload.c"
#include "fixid.c"
#include "tagcase.c"
#include "rename.c"
#include "langtag.c"
#include "datefix.c"
#include "agefix.c"
#include "filenames.c"
#include "mediatype.c"
#include "addschma.c"
#include "enums.c"
#include "tran.c"
#include "alia2aka.c"
#include "discard.c"
#include "exid.c"
#include "rela2role.c"
#include "version.c"

#include "event2record.c"
// #include "noter2s.c" // unfinished
#include "note2snote.c" // to be replaced by noter2s later
#include "sours2r.c"
#include "objes2r.c"
#include "record2event.c"

#ifdef CHANGE_NAMES
#include "names.c"
#endif

struct {
    GedFilterFunc passes[2];
    GedFilterStateMaker maker;
    GedFilterStateFreer freer;
    int twopass;
} ged_pipeline[] = {
    // turn CONC into GED_TEXT and CONT into GED_LINEBREAK
    {{ged_unconc, ged_unconc}, ged_longstate_maker, ged_longstate_freer},
    
    // merge adjacent GED_TEXT and GED_LINEBREAK into single GED_TEXT
    {{ged_merge, ged_merge}, ged_mergestate_maker, ged_mergestate_freer},

    // capitalize tags; GED_ERROR if illegal characters used in tag
    {{ged_tagcase, ged_tagcase}, ged_nostate_maker, ged_nostate_freer},

    // various simple tag renames
    {{0, ged_rename}, ged_longstate_maker, ged_longstate_freer},
    // remove obsolete tags
    {{0, ged_discard}, ged_longstate_maker, ged_longstate_freer},

    // two-pass handling of SCHMA
    {{ged_addschma1, ged_addschma2}, ged_addschma_maker, ged_addschma_freer},

    // change "English" to "en", etc
    {{0, ged_langtag}, ged_langtagstate_maker, ged_langtagstate_freer},
    // Update to 7.0 DATE format
    {{0, ged_datefix}, ged_longstate_maker, ged_longstate_freer},
    // Update to 7.0 AGE format
    {{0, ged_agefix}, ged_longstate_maker, ged_longstate_freer},
    // Update to 7.0 OBJE.FILE.FORM format
    {{0, ged_mediatype}, ged_longstate_maker, ged_longstate_freer},
    // Update FILE to have URL payload
    {{0, ged_filenames}, ged_longstate_maker, ged_longstate_freer},
    // change ROMN and FONE to TRAN with appropriate LANG
    {{0, ged_tran}, ged_longstate_maker, ged_longstate_freer},
    // change AFN, RIN, and RFN into EXID with appropriate TYPE
    {{0, ged_exid}, ged_exidstate_maker, ged_exidstate_freer},
    // change RELA to ROLE with PHRASE
    {{0, ged_rela2role}, ged_longstate_maker, ged_longstate_freer},
    // change RELA to ROLE with PHRASE
    {{0, ged_note2snote}, ged_longstate_maker, ged_longstate_freer},

    // not technically 5→7, this is to fix a common misuse of ALIA
    {{0, ged_alia2aka}, ged_longstate_maker, ged_longstate_freer},

    //// pass 1 assemble parse events into records
    //{{ged_event2record,0}, ged_event2recordstate_maker, ged_event2recordstate_freer},
    ////// change all note records into note structures
    //// {{ged_noter2s1, ged_noter2s2}, ged_noter2s_maker, ged_noter2s_freer},
    //// covert assembled records back into parse events
    //{{ged_record2event,0}, ged_nostate_maker, ged_nostate_freer},

    // pass 2 assemble parse events into records
    {{0, ged_event2record}, ged_event2recordstate_maker, ged_event2recordstate_freer},
    // change non-pointer SOUR substructures into pointer to SOUR records
    {{0, ged_sours2r}, ged_longstate_maker, ged_longstate_freer},
    // change non-pointer OBJE substructures into pointer to OBJE records
    {{0, ged_objes2r}, ged_longstate_maker, ged_longstate_freer},
#ifdef CHANGE_NAMES
    // convert to 7.0 NAME stucture
    {{0, ged_names}, ged_nostate_maker, ged_nostate_freer},
#endif
    // covert assembled records back into parse events
    {{0, ged_record2event}, ged_nostate_maker, ged_nostate_freer},

    // Standardize enums
    {{0, ged_enums}, ged_longstate_maker, ged_longstate_freer},
    // restrict anchors and pointers to allowed character set
    {{0, ged_fixid}, ged_fixidstate_maker, ged_fixidstate_freer},
    
    // fix version number
    {{0, ged_version}, ged_longstate_maker, ged_longstate_freer},

    // convert '\n' back to GED_LINEBREAK to prep for CONT encoding
    {{0, ged_unmerge}, ged_nostate_maker, ged_nostate_freer}, // should be last
};
//{ged_nop, ged_nostate_maker, ged_nostate_freer},
