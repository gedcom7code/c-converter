/**
 * This code is an implementation of the ANSEL character encoding as 
 * defined in GEDCOM 5.5 and 5.5.1, with the diacritic ordering defined 
 * in MARC-8; and converts to Unicode code points as described in
 * FHISO's ANSEL conversion writuep.
 * 
 * It also includes decoders for UTF-8, UTF-16, and UTF-32 and a UTF-8 
 * encoder. While these are available in the standard libraries of many
 * languages, including them here provides a reference for those wishing
 * to code them themselves and makes the transcoding code fully stand-alone.
 * 
 * All code in this file was written by Luther Tychonievich without
 * consulting any code, pseudo-code, or algorithm except the ELF
 * Serialisation draft document.
 * 
 * 2020-04-25: Initial version, including just ANSEL decoder
 * 2020-05-07: Added UTF-X decoders
 * 2020-05-09: Added codepoint_to_utf8
 * 2020-11-17: Refactored to be thread safe and look for HEAd.CHAR
 * 
 * This code is knowing and willfully released to the public domain 
 * by its author and may be used in whole or in part, with or without
 * attribution, for any purpose without requiring any additional
 * permission, payment, notification, or other action.
 */

#include <stdio.h>  // FILE*

/** Character encodings known to this implementation */
typedef enum { NONE, ANSEL, UTF8, UTF16LE, UTF16BE, UTF32LE, UTF32BE, ASCII } Codec;

/** 
 * Names of character encodings known to this implementation 
 * codec_names[ANSEL] will give a string identifying ANSEL, etc.
 */
extern const char *codec_names[];

/**
 * A stateful wrapper is needed to parse some codecs because of 
 * multibyte characters, diacritic reordering, etc.
 */
typedef struct {
    FILE *f;
    Codec format;
    
    // state for ANSEL-to-Unicode diacritic reordering
    int high[16]; int hc1; int hc2; // circular queue
    int low[16]; int lc; // stack
    int mid; // 0 (none) or 0x388 (combining long solidus overlay)
    
    // state for codepoint-to-UTF-8 serializing
    unsigned char queue[3]; int queuesize;

} DecodingFileReader;


/**
 * A byte-by-byte ANSEL to code point converter. Returns -1*byte if a 
 * non-ANSEL byte is encountered.
 * 
 * If there are more than 15 combining diacritics on a single
 * character it may drop some of them (keeps up to 1 center, 
 * 16 low, and 15 high diacritics).
 */
int ansel_next_codepoint(DecodingFileReader *s);

/**
 * A byte-by-byte UTF-8 to code point converter. Returns -1*value if an 
 * invalid value is encountered (e.g., an invalid byte, value encoded in
 * the wrong number of bytes or outside the 0..0x10FFFF range).
 */
int utf8_next_codepoint(DecodingFileReader *s);

/**
 * A byte-by-byte UTF-16 to code point converter. Returns -1*short if an 
 * invalid short is encountered (e.g., a mispositioned surrogate).
 * 
 * Set the second argument `le` to 0 if big-endian, 1 if little-endian.
 */
int utf16_next_codepoint(DecodingFileReader *s, int le);


/**
 * A byte-by-byte UTF-32 to code point converter. Returns -1*value if an 
 * invalid value is encountered (e.g., an invalid byte, or value outside
 * the 0..0x10FFFF range).
 * 
 * Set the second argument `le` to 0 if big-endian, 1 if little-endian.
 */
int utf32_next_codepoint(DecodingFileReader *s, int le);

/**
 * Picks the appropriate xxxx_next_codepoint based on s->format
 * Returns the next byte for ASCII or NONE.
 * 
 * Intended for internal use, but potentially useful for some APIs.
 */
int nextCodepoint(DecodingFileReader *s);

/**
 * Main public API:
 * Decodes input into a codepoint, then recodes it as UTF-8.
 *
 * Negative numbers indicate EOF (-1) or encoding error (<-1)
 */
int nextUTF8byte(DecodingFileReader *);

/**
 * Initializes `s` to a new decoding file reader for `f`,
 * which must be a seekable file opened for reading.
 * Performs character detection and looks for HEAD.CHAR to back that up.
 * `fseek`s the file pointer to the first post-BOM character.
 * When complete, `s` is ready for calls to `nextUTF8Byte`.
 * 
 * Returns 0 on success, nonzero if unable to parse enough GEDCOM 
 * to learn character encoding.
 */
int decodingFileReader_init(DecodingFileReader *s, FILE *in);

/**
 * Rewind so the next character returned is the fist character,
 * of the first character after the BOM if present.
 */
void decodingFileReader_rewind(DecodingFileReader *s);
