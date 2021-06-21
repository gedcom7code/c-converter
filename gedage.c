#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "gedage.h"

GedAge *gedAgeParse551(char *payload) {
    char *p = payload;
    GedAge *ans = calloc(1, sizeof(GedAge));
    ans->year = ans->month = ans->week = ans->day = -1;
    
    while(isspace(*p)) p+=1;

#define GED_AGE_INVALID { ans->phrase = payload; return ans; }

    if (*p == 'C' || *p == 'c') {
        p += 1; if (*p != 'H' && *p != 'h') GED_AGE_INVALID
        p += 1; if (*p != 'I' && *p != 'i') GED_AGE_INVALID
        p += 1; if (*p != 'L' && *p != 'l') GED_AGE_INVALID
        p += 1; if (*p != 'D' && *p != 'd') GED_AGE_INVALID
        p += 1;
        while(isspace(*p)) p+=1;
        if (*p) GED_AGE_INVALID
        ans->modifier = '<';
        ans->year = 8;
        ans->phrase = payload;
        return ans;
    }

    if (*p == 'I' || *p == 'i') {
        p += 1; if (*p != 'N' && *p != 'n') GED_AGE_INVALID
        p += 1; if (*p != 'F' && *p != 'f') GED_AGE_INVALID
        p += 1; if (*p != 'A' && *p != 'a') GED_AGE_INVALID
        p += 1; if (*p != 'N' && *p != 'n') GED_AGE_INVALID
        p += 1; if (*p != 'T' && *p != 't') GED_AGE_INVALID
        p += 1;
        while(isspace(*p)) p+=1;
        if (*p) GED_AGE_INVALID
        ans->modifier = '<';
        ans->year = 1;
        ans->phrase = payload;
        return ans;
    }

    if (*p == 'S' || *p == 's') {
        p += 1; if (*p != 'T' && *p != 't') GED_AGE_INVALID
        p += 1; if (*p != 'I' && *p != 'i') GED_AGE_INVALID
        p += 1; if (*p != 'L' && *p != 'l') GED_AGE_INVALID
        p += 1; if (*p != 'L' && *p != 'l') GED_AGE_INVALID
        p += 1; if (*p != 'B' && *p != 'b') GED_AGE_INVALID
        p += 1; if (*p != 'O' && *p != 'o') GED_AGE_INVALID
        p += 1; if (*p != 'R' && *p != 'r') GED_AGE_INVALID
        p += 1; if (*p != 'N' && *p != 'n') GED_AGE_INVALID
        p += 1;
        while(isspace(*p)) p+=1;
        if (*p) GED_AGE_INVALID
        ans->year = 0;
        ans->phrase = payload;
        return ans;
    }

    if (*p == '>' || *p == '<') {
        ans->modifier = *(p++);
        while(isspace(*p)) p+=1;
    }
    
    if (!*p) GED_AGE_INVALID;
    
    int num;
    while(*p) {
        if (*p < '0' || '9' < *p) GED_AGE_INVALID;
        num = 0;
        while ('0' <= *p && *p <= '9') num = 10*num + (*(p++)-'0');
        while(isspace(*p)) p+=1;
        if (!*p && ans->year < 0 && ans->month < 0 && ans->day < 0) 
        { ans->year = num; break; }
        else if (*p == 'y' || *p == 'Y') { ans->year = num; p += 1; }
        else if (*p == 'm' || *p == 'M') { ans->month = num; p+=1; }
        else if (*p == 'w' || *p == 'W') { ans->week = num; p+=1; }
        else if (*p == 'd' || *p == 'D') { ans->day = num; p += 1; }
        else GED_AGE_INVALID

        while(isspace(*p)) p+=1;
    }
    return ans;
}



/// the digits needed to represent n in base-10
static inline int gedAgeDigits(int n) {
    int ans = 1;
    while(n > 9) { ans += 1; n /= 10; }
    return ans;
}


char *gedAgePayload(GedAge *a) {
    long len = 0;
    if (a->modifier) len += 2; // "< "
    if (a->year >= 0) len += gedAgeDigits(a->year)+2; // "<num>y "
    if (a->month >= 0) len += gedAgeDigits(a->year)+2;// "<num>m "
    if (a->week >= 0) len += gedAgeDigits(a->year)+2; // "<num>w "
    if (a->day >= 0) len += gedAgeDigits(a->year)+2;  // "<num>d "
    // last space really a \0
    
    char *ans = malloc(len);
    char *out = ans;
    *out = 0;
    if (len <= 2) return ans; // nothing (but a modifier)
    
    if (a->modifier) out += sprintf(out, "%c", a->modifier);
    if (a->year >= 0) out += sprintf(out, "%s%dy", (ans != out ? " " : ""),  a->year);
    if (a->month >= 0) out += sprintf(out, "%s%dm", (ans != out ? " " : ""),  a->month);
    if (a->week >= 0) out += sprintf(out, "%s%dw", (ans != out ? " " : ""),  a->week);
    if (a->day >= 0) out += sprintf(out, "%s%dd", (ans != out ? " " : ""),  a->day);
    
    return ans;
}
