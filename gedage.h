#pragma once

typedef struct {
    int year, month, week, day;
    char *phrase;
    char modifier;
} GedAge;

GedAge *gedAgeParse551(char *payload);

char *gedAgePayload(GedAge *age);
