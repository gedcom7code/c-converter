/**
 * See lead-in comment from snesl2utf8.h for purpose, 
 * documentation, license, etc.
 */

#include "ansel2utf8.h"

#include <string.h> // strcasecmp
#include <ctype.h> // isspace
#include <stdio.h>  // FILE*, fseek, etc


const char *codec_names[] = {
    "Unknown",
    "ANSEL",
    "UTF-8",
    "UTF-16 little-endian",
    "UTF-16 big-endian",
    "UTF-32 little-endian",
    "UTF-32 big-endian",
    "ASCII (note: incomplete implementation)",
};


int ansel_next_codepoint(DecodingFileReader *s) {
    static int special[] = { // code points of ANSEL bytes from 0xA1 through 0xFF
        0x141, 0xD8, 0x110, 0xDE, 0xC6, 0x152, 0x2B9, 
        0xB7, 0x266D, 0xAE, 0xB1, 0x1A0, 0x1AF, 0x2BE, -0xAF, 
        0x2BF, 0x142, 0xF8, 0x111, 0xFE, 0xE6, 0x153, 0x2BA, 
        0x131, 0xA3, 0xF0, -0xBB, 0x1A1, 0x1B0, 0x25A1, 0x25A0, 
        0xB0, 0x2113, 0x2117, 0xA9, 0x2667, 0xBF, 0xA1, 0xDF, 
        0x20AC, -0xC9, -0xCA, -0xCB, -0xCC, 0x65, 0x6F, 0xDF, 
        -0xD0, -0xD1, -0xD2, -0xD3, -0xD4, -0xD5, -0xD6, -0xD7,
        -0xD8, -0xD9, -0xDA, -0xDB, -0xDC, -0xDD, -0xDE, -0xDF, 
        0x309, 0x300, 0x301, 0x302, 0x303, 0x304, 0x306, 0x307, 
        0x308, 0x30C, 0x30A, 0xFE20, 0xFE21, 0x315, 0x30B, 0x310, 
        0x327, 0x328, 0x323, 0x324, 0x325, 0x333, 0x332, 0x326, 
        0x328, 0x32E, 0xFE22, 0xFE23, 0x338, -0xFD, 0x313, -0xFF
    };
    if (s->mid) { int tmp = s->mid; s->mid = 0; return tmp; }
    if (s->lc) { return s->low[--(s->lc)]; }
    if (s->hc1 != s->hc2) { int tmp = s->high[s->hc2]; s->hc2 = (s->hc2+1)&0xF; return tmp; }
    
    int b = fgetc(s->f);
    if (b < 0) return b; // EOF and other read errors
    if (b > 0xFF) return -b; // larger than a byte? Should be impossible
    if (b < 0x80) return b; // ASCII
    if (b < 0xA1) return -b; // unmapped by every known ANSEL variant
    if (b < 0xE0 || special[b-0xA1] < 0) // single glyph or unmapped
        return special[b-0xA1];
    
    // combining: get the answer first, then push diacritics into queue
    int ans = ansel_next_codepoint(s);
    
    if (b == 0xFC) // center (only one in ANSEL)
        s->mid = special[b-0xA1]; // if several only keeps one
    else if (b >= 0xF0 && b <= 0xF9) { // low
        if (s->lc < 16) // drop 17th and beyond
            s->low[(s->lc)++] = special[b-0xA1];
    } else { // high
        if (((s->hc1+1)&0xF) != s->hc2) { // drop 16th and beyond
            s->high[(s->hc1)++] = special[b-0xA1]; s->hc1&=0xF;
        }
    }
    
    return ans;
}


int utf8_next_codepoint(DecodingFileReader *s) {
    int b = fgetc(s->f);
    if (b < 0x80) return b;
    if (b < 0xC0 || b >= 0xF8) return -b; // invalid leader

    int more = (b >= 0xC0) + (b >= 0xE0) + (b >= 0xF0);
    int ans = b & ((1<<(7-more))-1);

    for(int i=0; i<more; i+=1) {
        b = fgetc(s->f);
        if (b < 0x80 || b >= 0xC0) return -b;
        ans = (ans<<6) | (b&0x3F);
    }
    // disallow too many bytes
    if (more >= 4 && ans < (1<<16)) return -ans;
    if (more >= 3 && ans < (1<<12)) return -ans;
    if (more >= 2 && ans < (1<<7)) return -ans;

    // disallow things UTF-16 cannot handle
    if (ans >= 0x110000 || (ans >= 0xD800 && ans < 0xE000)) return -ans;

    return ans;
}


int utf16_next_codepoint(DecodingFileReader *s, int le) {
    int b1,b2;
    if ((b1 = fgetc(s->f)) < 0) return b1;
    if ((b2 = fgetc(s->f)) < 0) return b2;
    int s1 = le ? ((b2<<8)|b1) : ((b1<<8)|b2);
    if (s1 < 0xD800 || s1 >= 0xE000) return s1;

    if (s1 >= 0xDC00) return -s1; // cannot have trailing surrogate first
    if ((b1 = fgetc(s->f)) < 0) return b1;
    if ((b2 = fgetc(s->f)) < 0) return b2;
    int s2 = le ? ((b2<<8)|b1) : ((b1<<8)|b2);
    if (s2 < 0xDC00 || s2 >= 0xE000) return -s2; // must be trailing surrogate

    return (((s1&0x3FF)<<10) | (s2&0x3FF)) + 0x10000;
}


int utf32_next_codepoint(DecodingFileReader *s, int le) {
    int b1,b2,b3,b4;
    if ((b1 = fgetc(s->f)) < 0) return b1;
    if ((b2 = fgetc(s->f)) < 0) return b2;
    if ((b3 = fgetc(s->f)) < 0) return b3;
    if ((b4 = fgetc(s->f)) < 0) return b4;
    int ans = le ? ((b4<<24)|(b3<<16)|(b2<<8)|b1) 
                 : ((b1<<24)|(b2<<16)|(b3<<8)|b4);

    // disallow things UTF-16 cannot handle
    if (ans >= 0x110000 || (ans >= 0xD800 && ans < 0xE000)) return -ans;

    return ans;
}


int nextCodepoint(DecodingFileReader *s) {
    switch(s->format) {
        case NONE: return fgetc(s->f);
        case ANSEL: return ansel_next_codepoint(s);
        case UTF8: return utf8_next_codepoint(s);
        case UTF16LE: return utf16_next_codepoint(s, 1);
        case UTF16BE: return utf16_next_codepoint(s, 0);
        case UTF32LE: return utf32_next_codepoint(s, 1);
        case UTF32BE: return utf32_next_codepoint(s, 0);
        case ASCII: return fgetc(s->f);
    }
}

int nextUTF8byte(DecodingFileReader *s) {
    if (s->queuesize) return s->queue[--(s->queuesize)];
    int codepoint = nextCodepoint(s);
    if (codepoint < (1<<7)) return codepoint;
    if (codepoint < (1<<11)) {
        s->queue[s->queuesize++] = (codepoint&0x3F)|0x80;
        return (codepoint>>6)|0xC0;
    }
    if (codepoint < (1<<16)) {
        s->queue[s->queuesize++] = (codepoint&0x3F)|0x80;
        s->queue[s->queuesize++] = ((codepoint>>6)&0x3F)|0x80;
        return (codepoint>>12)|0xE0;
    }
    if (codepoint < (1<<21)) {
        s->queue[s->queuesize++] = (codepoint&0x3F)|0x80;
        s->queue[s->queuesize++] = ((codepoint>>6)&0x3F)|0x80;
        s->queue[s->queuesize++] = ((codepoint>>12)&0x3F)|0x80;
        return (codepoint>>18)|0xF0;
    }
    return -codepoint; // larger than UTF-8 allows
}


int decodingFileReader_init(DecodingFileReader *s, FILE *in) {
    s->f = in;
    s->format = NONE;
    s->hc1 = s->hc2 = s->lc = s->mid = s->queuesize = 0;
    
    // detected character encoding based on first 4 bytes
    unsigned char check[4];
    int bom = 0;
    if (fread(check, 1, 4, in) != 4) { 
        fprintf(stderr, "ERROR: empty file\n");
        return 4;
    }
    if (check[0] == 0xef && check[1] == 0xbb && check[2] == 0xbf) {
        s->format = UTF8;
        bom = 3;
    } else if (check[0] == 0xFF && check[1] == 0xFE) {
        s->format = (check[2] || check[3]) ? UTF16LE : UTF32LE;
        bom = (check[2] || check[3]) ? 2 : 4;
    } else if (check[0] == 0xFE && check[1] == 0xFF) {
        s->format = UTF16BE;
        bom = 2;
    } else if (!check[0] && !check[1] && check[2] == 0xFF && check[3] == 0xFE) {
        s->format = UTF32BE;
        bom = 4;
    } else if (check[0] && !check[1] && !check[2] && !check[3]) {
        s->format = UTF32LE;
        bom = 0;
    } else if (!check[0] && !check[1] && !check[2] && check[3]) {
        s->format = UTF32BE;
        bom = 0;
    } else if (check[0] && !check[1]) {
        s->format = UTF16LE;
        bom = 0;
    } else if (!check[0] && check[1]) {
        s->format = UTF16BE;
        bom = 0;
    }
    //fprintf(stderr, "Detected character encoding: %s (%s BOM)\n", codec_names[s->format], bom ? "with" : "without");
    
    // use detected character encoding to look for CHAR tag in HEAD
    fseek(in, bom, SEEK_SET);
    s->hc1 = s->hc2 = s->lc = s->mid = s->queuesize = 0;
    // the rule is /[\n\r][ \t]*1[ \t]+CHAR[ \t]+([^\n]*)/
    // between first and second /([\n\r]|^)0/
    /* Step Meaning
     * 0    before '0 HEAD'
     * 1    inside non-CHAR line
     * 2    past line break
     * 3    [\n\r]1
     * 4    [\n\r]1[ \t]+
     * 5    [\n\r]1[ \t]+C
     * 6    [\n\r]1[ \t]+CH
     * 7    [\n\r]1[ \t]+CHA
     * 8    [\n\r]1[ \t]+CHAR
     * 9    [\n\r]1[ \t]+CHAR[ \t]*
     */
    int step = 0;
    int octet = 0;
    char specified_encoding[256];
    specified_encoding[0] = 0;
    while(step != 2 || octet != '0') {
        octet = nextUTF8byte(s);
        if (octet == -1) 
            return 1; // not GEDCOM: file ended inside HEAD
        switch(step) {
            case 0: {
                if (isspace(octet)) {}
                else if (octet == '0') { step = 1; }
                else { return 2; } // not GEDCOM: not start with 0 HEAD
            } break;
            case 1: {
                if (octet == '\n' || octet == '\r') step = 2;
            } break;
            case 2: {
                if (octet == '\n' || octet == '\r') step = 2;
                else if (octet == '0') step = 10;
                else if (octet == '1') step = 3;
                else step = 1;
            } break;
            case 3: {
                if (octet == '\n' || octet == '\r') step = 2;
                else if (octet == ' ' || octet == '\t') step += 1;
                else step = 1;
            } break;
            case 4: {
                if (octet == '\n' || octet == '\r') step = 2;
                else if (octet == ' ' || octet == '\t') {}
                else if (octet == 'c' || octet == 'C') step += 1;
                else step = 1;
            } break;
            case 5: {
                if (octet == '\n' || octet == '\r') step = 2;
                else if (octet == 'h' || octet == 'H') step += 1;
                else step = 1;
            } break;
            case 6: {
                if (octet == '\n' || octet == '\r') step = 2;
                else if (octet == 'a' || octet == 'A') step += 1;
                else step = 1;
            } break;
            case 7: {
                if (octet == '\n' || octet == '\r') step = 2;
                else if (octet == 'r' || octet == 'R') step += 1;
                else step = 1;
            } break;
            case 8: {
                if (octet == '\n' || octet == '\r') step = 2;
                else if (octet == ' ' || octet == '\t') step += 1;
                else step = 1;
            } break;
            case 9: {
                if (octet == '\n' || octet == '\r') step = 2;
                else if (octet == ' ' || octet == '\t') {}
                else {
                    for(int i=0; i<255 && octet>0x1f; i+=1) {
                        specified_encoding[i] = octet;
                        specified_encoding[i+1] = 0;
                        octet = nextUTF8byte(s);
                    }
                    step = 10;
                }
            } break;
        }
        if (step == 10) break;
    }
    if (!specified_encoding[0]) {
        // use detected
    } else if (strcasecmp(specified_encoding, "UTF-8") == 0) {
        s->format = UTF8;
    } else if (strcasecmp(specified_encoding, "ASCII") == 0) {
        s->format = ASCII;
    } else if (strcasecmp(specified_encoding, "ANSEL") == 0) {
        s->format = ANSEL;
    } else if (strcasecmp(specified_encoding, "UNICODE") == 0) {
        if (s->format == NONE) s->format = UTF8; // non-standard use
        // standard cases (UTF16LE and UTF16BE) already detected
    } else {
        fprintf(stderr, "Unexpected encoding %s\n", specified_encoding);
        return 3; // unsupported character encoding
    }
    
    // QUESTION: is ANSEL the right default?
    if (s->format == NONE) s->format = UTF8; 
    
    fseek(in, bom, SEEK_SET);
    s->hc1 = s->hc2 = s->lc = s->mid = s->queuesize = 0;
    return 0;
}

void decodingFileReader_rewind(DecodingFileReader *s) {
    int bom = 0;

    fseek(s->f, bom, SEEK_SET);
    
    // detected character encoding based on first 4 bytes
    unsigned char check[4];
    if (fread(check, 1, 4, s->f) != 4) { 
        fseek(s->f, bom, SEEK_SET);
        return;
    }
    if (check[0] == 0xef && check[1] == 0xbb && check[2] == 0xbf) {
        bom = 3;
    } else if (check[0] == 0xFF && check[1] == 0xFE) {
        bom = (check[2] || check[3]) ? 2 : 4;
    } else if (check[0] == 0xFE && check[1] == 0xFF) {
        bom = 2;
    } else if (!check[0] && !check[1] && check[2] == 0xFF && check[3] == 0xFE) {
        bom = 4;
    } else if (check[0] && !check[1] && !check[2] && !check[3]) {
        bom = 0;
    } else if (!check[0] && !check[1] && !check[2] && check[3]) {
        bom = 0;
    } else if (check[0] && !check[1]) {
        bom = 0;
    } else if (!check[0] && check[1]) {
        bom = 0;
    }

    fseek(s->f, bom, SEEK_SET);
    s->hc1 = s->hc2 = s->lc = s->mid = s->queuesize = 0;
}
