#include <ctype.h>

/**
 * 5.5.1 allowed FILE payloads to be filenames without specifying OS, etc.
 * 7.0 requires them to be URLs.
 * 
 * Going on the assumption that all found filenames will be Windows, Mac, or Linux:
 * 
 * - leading \\foo\ becomes file://foo/
 * - leading x:\ becomes file:///x:/
 * - all \ become /
 * - leading / becomes file:///
 * - all ":" / "?" / "#" / "[" / "]" / "@" are percent-escaped
 * 
 * This isn't perfect, but should handle virtually all cases.
 */
void ged_filenames(GedEvent *event, GedEmitterTemplate *emitter, void *state) {
    long *isFILE = (long *)state;
    if (event->type == GED_START)
        *isFILE = !strcmp("FILE", event->data);
    if (*isFILE && event->type == GED_TEXT) {
        int needed = 0;
        if (event->data[0] == '/') needed += 7;
        if (event->data[0] == '\\') {
            if (event->data[1] == '\\' && event->data[2] != '\\') needed += 5;
            else needed += 7;
        }
        if (isalpha(event->data[0]) && event->data[1] == ':' && event->data[2] != '\\') needed += 8;
        int isURL = isalpha(event->data[0]) != 0;
        for(char *t = event->data; *t; t+=1) {
            if (isURL == 1) {
                if (*t == ':') isURL = 2;
                else if (*t == '/' || *t == '\\') isURL = 0;
            } else if (isURL > 1) {
                if (*t == '/') isURL += 1;
                else if (isURL >= 4) {
                    emitter->emit(emitter, *event);
                    return;
                } else isURL = 0;
            }
            needed += 1;
            if (*t == ':' || *t == '?' || *t == '#' || *t == '[' || *t == ']' || *t == '@') needed += 2;
        }
        
        char *payload = malloc(needed+1);
        char *p = payload;
        char *t = event->data;
        
        if (*t == '/') {
            *(p++) = 'f'; *(p++) = 'i'; *(p++) = 'l'; *(p++) = 'e';
            *(p++) = ':'; *(p++) = '/'; *(p++) = '/';
        } else if (t[0] == '\\') {
            if (t[1] == '\\' && t[2] != '\\') {
                *(p++) = 'f'; *(p++) = 'i'; *(p++) = 'l'; *(p++) = 'e';
                *(p++) = ':'; *(p++) = '/'; *(p++) = '/';
                t += 2;
            } else {
                *(p++) = 'f'; *(p++) = 'i'; *(p++) = 'l'; *(p++) = 'e';
                *(p++) = ':'; *(p++) = '/'; *(p++) = '/';
            }
        } else if (isalpha(t[0]) && t[1] == ':' && t[2] != '\\') {
            *(p++) = 'f'; *(p++) = 'i'; *(p++) = 'l'; *(p++) = 'e';
            *(p++) = ':'; *(p++) = '/'; *(p++) = '/'; *(p++) = '/';
            *(p++) = *(t++); *(p++) = *(t++);
        }
        
        for(; *t; t+=1) {
            switch(*t) {
                case '\\': *(p++) = '/'; break;
                case ':': *(p++) = '%'; *(p++) = '3'; *(p++) = 'a'; break;
                case '?': *(p++) = '%'; *(p++) = '3'; *(p++) = 'F'; break;
                case '#': *(p++) = '%'; *(p++) = '2'; *(p++) = '3'; break;
                case '[': *(p++) = '%'; *(p++) = '5'; *(p++) = 'B'; break;
                case ']': *(p++) = '%'; *(p++) = '5'; *(p++) = 'D'; break;
                case '@': *(p++) = '%'; *(p++) = '4'; *(p++) = '0'; break;
                default: *(p++) = *t;
            }
        }
        *p = 0;
        
        ged_destroy_event(event);
        emitter->emit(emitter, (GedEvent){GED_TEXT, GED_OWNS_DATA, .data=payload});
    } else {
        emitter->emit(emitter, *event);
    }
}
