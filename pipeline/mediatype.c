#include <string.h>

/**
 * 5.5.1 used custom file format labels; 7.0 uses media types:
 * 
 * - bmp -> image/bmp
 * - gif -> image/gif
 * - jpg -> image/jpg
 * - ole -> application/x-oleobject ? application/x-ole-storage ?
 * - pcx -> image/vnd.zbrush.pcx ? image/x-pcx ?
 * - tif -> image/tiff
 * - wav -> audio/vnd.wave ? audio/x-wav ? 
 * 
 */
void ged_mediatype(GedEvent *event, GedEmitterTemplate *emitter, void *state) {
    short *depth = (short *)state;
    short *nest = depth+1;
    if (event->type == GED_START) {
        if (*depth == 0 && !*nest && !strcmp("OBJE", event->data))
            *depth = 1;
        else if (*depth == 1 && !*nest && !strcmp("FILE", event->data))
            *depth = 2;
        else if (*depth == 2 && !*nest && !strcmp("FORM", event->data))
            *depth = 3;
        else if (*depth) *nest += 1;
    }
    if (event->type == GED_END) {
        if (*nest) *nest -= 1;
        else if (*depth) *depth -= 1;
    }

    if (*depth == 3 && *nest == 0 && event->type == GED_TEXT) {
        // multimedia format; change to new string
        if (!strcmp("gif", event->data)) {
            if (event->flags & GED_OWNS_DATA) free(event->data);
            event->data = "image/gif";
            event->flags &= ~GED_OWNS_DATA;
        } else if (!strcmp("jpg", event->data)) {
            if (event->flags & GED_OWNS_DATA) free(event->data);
            event->data = "image/jpeg";
            event->flags &= ~GED_OWNS_DATA;
        } else if (!strcmp("tif", event->data)) {
            if (event->flags & GED_OWNS_DATA) free(event->data);
            event->data = "image/tiff";
            event->flags &= ~GED_OWNS_DATA;
        } else if (!strcmp("bmp", event->data)) {
            if (event->flags & GED_OWNS_DATA) free(event->data);
            event->data = "image/bmp"; // or image/x-bmp or image/x-ms-bmp
            event->flags &= ~GED_OWNS_DATA;
        } else if (!strcmp("ole", event->data)) {
            if (event->flags & GED_OWNS_DATA) free(event->data);
            event->data = "application/x-oleobject"; // or application/x-ole-storage
            event->flags &= ~GED_OWNS_DATA;
        } else if (!strcmp("pcx", event->data)) {
            if (event->flags & GED_OWNS_DATA) free(event->data);
            event->data = "image/vnd.zbrush.pcx"; // or image/x-pcx
            event->flags &= ~GED_OWNS_DATA;
        } else if (!strcmp("wav", event->data)) {
            if (event->flags & GED_OWNS_DATA) free(event->data);
            event->data = "audio/vnd.wave"; // or audio/x-wav
            event->flags &= ~GED_OWNS_DATA;
        } else {
            char *data = malloc(strlen(event->data)+15);
            *data = 0;
            strcat(data, "application/x-");
            strcat(data+14, event->data);
            if (event->flags & GED_OWNS_DATA) free(event->data);
            event->data = data;
            event->flags |= GED_OWNS_DATA;
        }
    }
    emitter->emit(emitter, *event);
}
