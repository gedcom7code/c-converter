#include <ctype.h>  // for isspace
#include <stdlib.h> // for malloc
#include <string.h> // for memcpy
#include <stdio.h>  // for sprintf
#include <assert.h>
#include "geddate.h"

enum {
    GED_DATE_NONE = 0,
    GED_DATE_GREGMONTH,
    GED_DATE_KEY1,
    GED_DATE_KEY2,
    GED_DATE_KEY12,
    GED_DATE_EPOCH,
    GED_DATE_WORD,
    GED_DATE_NUMBER,
    GED_DATE_OPAREN,
    GED_DATE_CPAREN,
    GED_DATE_SLASH,
    GED_DATE_ERROR,
};

typedef struct {
    unsigned int type;
    union {
        char *token;
        long number;
    };
} GedDateToken;

/// Given 1- or 2-character string, returns !strcmp(a,b)
static inline int strSame2(const char *a, const char *b) {
    return (*(short *)a) == (*(short *)b);
}
/// Given 3- or 4-character string, returns !strcmp(a,b)
static inline int strSame4(const char *a, const char *b) {
    return (*(int *)a) == (*(int *)b);
}

GedDateToken gedDateNextToken(char **s) {
    while(isspace(**s)) *s+=1;
    
    GedDateToken ans; 
    ans.type = GED_DATE_NONE;
    ans.token = 0;
    
    if (!**s) return ans;
    
    if (**s == '(') { ans.type = GED_DATE_OPAREN; *s+=1; return ans; }
    if (**s == ')') { ans.type = GED_DATE_CPAREN; *s+=1; return ans; }
    if (**s == '/') { ans.type = GED_DATE_SLASH; *s+=1; return ans; }
    if ('0' <= **s && **s <= '9') {
        ans.type = GED_DATE_NUMBER;
        ans.number = 0;
        while ('0' <= **s && **s <= '9') {
            ans.number *= 10;
            ans.number += **s - '0';
            *s += 1;
        }
        return ans;
    }
    if (('A' <= **s && **s <= 'Z') || ('a' <= **s && **s <= 'z') || **s == '_') {
        ans.token = *s;
        while (**s && !isspace(**s)) {
            if ('a' <= **s && **s <= 'z') **s ^= ('A'^'a');
            *s += 1;
        }
        size_t len = *s - ans.token;
        if (**s) { **s = 0; *s += 1; }

        ans.type = GED_DATE_WORD;
        if (len == 2) {
            if (strSame2(ans.token, "BC")
            || strSame2(ans.token, "AD")) 
                ans.type = GED_DATE_EPOCH;
            else if (strSame2(ans.token, "TO"))
                ans.type = GED_DATE_KEY12;
        } else if (len == 3) {
            if (strSame4(ans.token, "BCE")
            || strSame4(ans.token, "ADE")) 
                ans.type = GED_DATE_EPOCH;
            else if (strSame4(ans.token, "JAN")
            || strSame4(ans.token, "FEB")
            || strSame4(ans.token, "MAR")
            || strSame4(ans.token, "APR")
            || strSame4(ans.token, "MAY")
            || strSame4(ans.token, "JUN")
            || strSame4(ans.token, "JUL")
            || strSame4(ans.token, "AUG")
            || strSame4(ans.token, "SEP")
            || strSame4(ans.token, "OCT")
            || strSame4(ans.token, "NOV")
            || strSame4(ans.token, "DEC"))
                ans.type = GED_DATE_GREGMONTH;
            else if (strSame4(ans.token, "BET")
            || strSame4(ans.token, "BEF")
            || strSame4(ans.token, "AFT")
            || strSame4(ans.token, "EST")
            || strSame4(ans.token, "CAL")
            || strSame4(ans.token, "ABT")
            || strSame4(ans.token, "INT"))
                ans.type = GED_DATE_KEY1;
            else if (strSame4(ans.token, "AND"))
                ans.type = GED_DATE_KEY2;
        } else if (len == 4) {
            if (strSame4(ans.token, "B.C.")
            || strSame4(ans.token, "A.D.")) 
                ans.type = GED_DATE_EPOCH;
            else if (strSame4(ans.token, "FROM"))
                ans.type = GED_DATE_KEY1;
        }
        return ans;
    }
    ans.type = GED_DATE_ERROR;
    ans.token = *s;
    *s += strlen(*s);
    return ans;
}



GedDateValue *gedDateParse551(char *payload) {
    char *p = payload;
    GedDateValue *ans = calloc(1, sizeof(GedDateValue));
    ans->freeMe = p;
    char *copy = strdup(p);


    GedDateToken tok = gedDateNextToken(&p);


#define GED_DATE_COPY_AS_PHRASE if (copy) { \
    ans->phrase = copy; \
    copy = 0; \
}

#define GED_DATE_EAT_SLASH if (tok.type == GED_DATE_SLASH) { \
    tok = gedDateNextToken(&p);\
    tok = gedDateNextToken(&p);\
    GED_DATE_COPY_AS_PHRASE \
}
    
    // option "n DATE (text)"
    if (tok.type == GED_DATE_OPAREN && !ans->phrase) {
        size_t len = strlen(p);
        while (len && isspace(p[len-1])) len-=1;
        if (len && p[len-1] == ')') p[len-1] = 0;
        ans->phrase = strdup(p);
        free(ans->freeMe);
        ans->freeMe = 0;
        return ans;
    }

    // optional starting keyword
    if (tok.type == GED_DATE_KEY1 || tok.type == GED_DATE_KEY12) {
        ans->modifier = tok.token;
        tok = gedDateNextToken(&p);
    }

    // required date
    ans->d1 = calloc(1, sizeof(GedDate));
    if (tok.type == GED_DATE_WORD) { // optional calendar
        ans->d1->calendar = tok.token;
        tok = gedDateNextToken(&p);
    }
    if (tok.type == GED_DATE_GREGMONTH) { // month + year
        ans->d1->month = tok.token;
        tok = gedDateNextToken(&p);
        GED_DATE_EAT_SLASH
    }
    if (tok.type == GED_DATE_NUMBER) { // year (or day)
        ans->d1->year = tok.number;
        tok = gedDateNextToken(&p);
        if (!ans->modifier && !ans->d1->month && !ans->phrase && tok.type == GED_DATE_SLASH) {
            // special dual year as undocumented shorthand for BET/AND
            GED_DATE_COPY_AS_PHRASE
            tok = gedDateNextToken(&p);
            if (tok.type == GED_DATE_NUMBER) {
                long num = tok.number;
                tok = gedDateNextToken(&p);
                if (tok.type == GED_DATE_NONE) {
                    ans->modifier = "BET";
                    ans->d2 = calloc(1, sizeof(GedDate));
                    ans->d2->year = ans->d1->year;
                    ans->d2->calendar = ans->d1->calendar;
                    long mod = 10; 
                    while (mod*10 > 0 && mod < num) mod*=10;
                    if ((ans->d2->year % mod) > num) {
                        ans->d2->year -= ans->d2->year % mod;
                        ans->d2->year += mod;
                        ans->d2->year += num;
                    } else {
                        ans->d2->year -= ans->d2->year % mod;
                        ans->d2->year += num;
                    }
                    return ans;
                }
            } else {
                tok = gedDateNextToken(&p);
            }
            // end special case
        } else {
            GED_DATE_EAT_SLASH
        }
    }
    if (!ans->d1->month && ( // month
        tok.type == GED_DATE_GREGMONTH
        || (ans->d1->calendar && tok.type == GED_DATE_WORD)
    )) {
        ans->d1->day = ans->d1->year;
        ans->d1->year = 0;
        ans->d1->month = tok.token;
        tok = gedDateNextToken(&p);
        GED_DATE_EAT_SLASH
    }
    if (ans->d1->month && !ans->d1->year) {
        if (tok.type == GED_DATE_NUMBER) { // year
            ans->d1->year = tok.number;
            tok = gedDateNextToken(&p);
            GED_DATE_EAT_SLASH
        } else { // or error (month without year illegal)
            free(ans->d1); ans->d1 = 0;
            ans->modifier = 0;
            GED_DATE_COPY_AS_PHRASE
            if (ans->freeMe) free(ans->freeMe);
            ans->freeMe = 0;
            return ans;
        }
    }
    if (tok.type == GED_DATE_EPOCH) {
        if (tok.token[0] == 'B')
            ans->d1->epoch = "BCE";
        tok = gedDateNextToken(&p);
    }
    
    // what's next depends on starting keyword
    if (!ans->modifier // no keyword, so only this date
    || strSame2(ans->modifier, "TO") // single-date keywords
    || strSame4(ans->modifier, "BEF")
    || strSame4(ans->modifier, "AFT")
    || strSame4(ans->modifier, "EST")
    || strSame4(ans->modifier, "ABT")
    || strSame4(ans->modifier, "CAL")) {
        if (tok.type != GED_DATE_NONE) {
            GED_DATE_COPY_AS_PHRASE
        }
        return ans;
    }

    if (strSame4(ans->modifier, "INT")) { // INT <date> (<date phrase>)
        ans->modifier = 0; // not used as a modifier in GEDCOM 7
        if (tok.type == GED_DATE_OPAREN && !ans->phrase) {
            size_t len = strlen(p);
            while (len && isspace(p[len-1])) len-=1;
            if (len && p[len-1] == ')') p[len-1] = 0;
            ans->phrase = strdup(p);
        } else {
            GED_DATE_COPY_AS_PHRASE
        }
        return ans;
    }
    if (strSame4(ans->modifier, "FROM")) { // FROM <date> -or- FROM <date> TO <date>
        if (tok.type == GED_DATE_NONE) return ans;
        if (tok.type == GED_DATE_KEY12
        && strSame2(tok.token, "TO")) {
            ans->d2 = calloc(1, sizeof(GedDate));
            tok = gedDateNextToken(&p);
        } else {
            GED_DATE_COPY_AS_PHRASE
            return ans;
        }
    } else if (tok.type != GED_DATE_KEY2 || !(
        (strSame4(tok.token, "AND") && strSame4(ans->modifier, "BET"))
    )) {
        GED_DATE_COPY_AS_PHRASE
        return ans;
    } else {
        ans->d2 = calloc(1, sizeof(GedDate));
        tok = gedDateNextToken(&p);
    }
    
    // need a second date
    if (tok.type == GED_DATE_WORD) { // optional calendar
        ans->d2->calendar = tok.token;
        tok = gedDateNextToken(&p);
    }
    if (tok.type == GED_DATE_GREGMONTH) { // month + year
        ans->d2->month = tok.token;
        tok = gedDateNextToken(&p);
        GED_DATE_EAT_SLASH
    }
    if (tok.type == GED_DATE_NUMBER) { // year (or day)
        ans->d2->year = tok.number;
        tok = gedDateNextToken(&p);
        GED_DATE_EAT_SLASH
    }
    if (!ans->d2->month && ( // month
        tok.type == GED_DATE_GREGMONTH
        || (ans->d2->calendar && tok.type == GED_DATE_WORD)
    )) {
        ans->d2->day = ans->d2->year;
        ans->d2->year = 0;
        ans->d2->month = tok.token;
        tok = gedDateNextToken(&p);
        GED_DATE_EAT_SLASH
    }
    if (ans->d2->month && !ans->d2->year) {
        if (tok.type == GED_DATE_NUMBER) { // year
            ans->d2->year = tok.number;
            tok = gedDateNextToken(&p);
            GED_DATE_EAT_SLASH
        } else { // or error (month without year illegal)
            free(ans->d2); ans->d2 = 0;
            GED_DATE_COPY_AS_PHRASE
            return ans;
        }
    }
    if (tok.type == GED_DATE_EPOCH) {
        if (tok.token[0] == 'B')
            ans->d2->epoch = "BCE";
        tok = gedDateNextToken(&p);
    }
    
    // and now MUST end
    if (tok.type != GED_DATE_NONE) {
        GED_DATE_COPY_AS_PHRASE
        return ans;
    }
    
#undef GED_DATE_EAT_SLASH

    if (copy) free(copy);
    return ans;
}

/// the digits needed to represent n in base-10
static inline int gedDateDigits(size_t n) {
    int ans = 1;
    while(n > 9) { ans += 1; n /= 10; }
    return ans;
}


char *gedDatePayload(GedDateValue *d) {
    size_t len = 1; // for '\0'
    if (d->modifier) len += 9; // worst case: "FROM * TO *" or "BET * AND *"
    if (d->d1) {
        if (d->d1->calendar) len += strlen(d->d1->calendar)+1;
        if (d->d1->day) len += gedDateDigits(d->d1->day)+1;
        if (d->d1->month) len += strlen(d->d1->month)+1;
        len += gedDateDigits(d->d1->year);
        if (d->d1->epoch) len += strlen(d->d1->epoch)+1;
    }
    if (d->d2) {
        if (d->d2->calendar) len += strlen(d->d2->calendar)+1;
        if (d->d2->day) len += gedDateDigits(d->d2->day)+1;
        if (d->d2->month) len += strlen(d->d2->month)+1;
        len +=  gedDateDigits(d->d2->year);
        if (d->d2->epoch) len += strlen(d->d2->epoch)+1;
    }
    char *ans = malloc(len);
    char *out = ans;
    *out = 0;
    if (d->modifier) out += sprintf(out, "%s ", d->modifier);
    if (d->d1) {
        if (d->d1->calendar) out += sprintf(out, "%s ", d->d1->calendar);
        if (d->d1->day) out += sprintf(out, "%d ", d->d1->day);
        if (d->d1->month) out += sprintf(out, "%s ", d->d1->month);
        out += sprintf(out, "%d", d->d1->year);
        if (d->d1->epoch) out += sprintf(out, " %s", d->d1->epoch);
    }
    if (d->d2) {
        if (strSame4(d->modifier, "FROM"))
            out += sprintf(out, " TO ");
        else if (strSame4(d->modifier, "BET"))
            out += sprintf(out, " AND ");
        else assert(0); // impossible, no other two-date formats exist
        
        if (d->d2->calendar) out += sprintf(out, "%s ", d->d2->calendar);
        if (d->d2->day) out += sprintf(out, "%d ", d->d2->day);
        if (d->d2->month) out += sprintf(out, "%s ", d->d2->month);
        out += sprintf(out, "%d", d->d2->year);
        if (d->d2->epoch) out += sprintf(out, " %s", d->d2->epoch);
    }
    return ans;
}
