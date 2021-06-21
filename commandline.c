#include "ged_ebp.h"
#include <string.h>

/**
 * Simple command-line wrapper.
 * Argument handling done manually to avoid dependence on platform-specific libraries.
 */
int main(int argc, char *argv[]) {
    FILE *in = stdin;
    FILE *out = stdout;
    
    int overwrite = 0;
    ged_xref_case_insensitive = 0;
    ged_few_phrases = 0;

    for(int i=1; i<argc; i+=1) {
        if (!strcmp("-h", argv[i])
        || !strcmp("--help", argv[i])) {
            fprintf(stderr, "USAGE: %s [options] [infile.ged] [outfile.ged]\n"
            "where options may be\n"
            "  -h --help        this help message\n"
            "  -f --force       overwrite existing outfile.ged\n"
            "  -x --xreficase   compare xrefs case-insensitively\n"
            "  -p --fewphrases  omit PHRASE when reasonable payload available\n" , argv[0]);
            return 1;
        }
        else if (!strcmp("-f", argv[i]) || !strcmp("--force", argv[i])) overwrite = 1;
        else if (!strcmp("-x", argv[i]) || !strcmp("--xreficase", argv[i])) ged_xref_case_insensitive = 1;
        else if (!strcmp("-p", argv[i]) || !strcmp("--fewphrases", argv[i])) ged_few_phrases = 1;
        else if (in == stdin) {
            in = fopen(argv[i], "rb");
            if (!in) {
                fprintf(stderr, "ERROR: unable to read from %s\n", argv[i]);
                return 2;
            }
        }
        else if (out == stdout) {
            out = fopen(argv[i], overwrite ? "wb" : "wxb");
            if (!out) {
                fprintf(stderr, "ERROR: unable to write to %s\n", argv[i]);
                return 3;
            }
        } else {
            fprintf(stderr, "ERROR: unexpected argument %s\n", argv[i]);
            return 4;
        }
    }
    
    if (out == stdout && overwrite) {
        fprintf(stderr, "ERROR: --force flag incompatible with stdout output\n");
        return 5;
    }

    ged551to700(in, out);
    return 0;
}



/*


#include <argp.h> // parse command-line arguments
#include "ged_ebp.h"

static char doc[] = "Converts GEDCOM 5.5.1 to 7.0 (without validation)";
static char args_doc[] = "input.ged output.ged";
struct arguments {
    char *from;
    char *to;
    int overwrite;
};

static struct argp_option options[] = {
    {"from", 'f', "FILE", 0, "Read from file instead of standard input (defaults to first argument)"},
    {"to", 't', "FILE", 0, "Write to file instead of standard input (defaults to second argument)"},
    {"overwrite", 'o', 0, 0, "Overwrite output file if it already exists"},
    {"xreficase", 'x', 0, 0, "Compare pointers case-insensitively"},
    {"fewphrase", 'p', 0, 0, "Omit phrases when an applicable enumeration is found"},
{0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *args = state->input;
    switch(key) {
        case 'f': args->from = *arg ? arg : NULL; break;
        case 't': args->to = *arg ? arg : NULL; break;
        case 'o': args->overwrite = 1; break;
        case 'x': ged_xref_case_insensitive = 1; break;
        case 'p': ged_few_phrases = 1; break;
        case ARGP_KEY_ARG:
            if (state->arg_num == 0 && !args->from) args->from = arg;
            else if (state->arg_num == 1 && !args->to) args->to = arg;
            else argp_usage(state);
            break;
        case ARGP_KEY_END:
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp parser = { options, parse_opt, args_doc, doc };
*/
/**
 * Simple command-line wrapper.
 * Run with no arguments to see documentation.
 */
/*
int main(int argc, char *argv[]) {
    struct arguments args;
    args.from = args.to = 0;
    args.overwrite = ged_xref_case_insensitive = ged_few_phrases = 0;
    argp_parse(&parser, argc, argv, 0, 0, &args);
    
    FILE *in = args.from ? fopen(args.from, "rb") : stdin;
    if (!in) {
        fprintf(stderr, "ERROR: unable to open %s for reading\n", args.from);
        return 2;
    }
    
    FILE *out = args.to ? fopen(args.to, args.overwrite ? "wb" : "wxb") : stdout;
    if (!out) {
        if (args.overwrite) {
            fprintf(stderr, "ERROR: unable to open %s for writing\n", argv[2]);
        } else {
            fprintf(stderr, "ERROR: unable to create %s for writing\n  (did you forget to use `--overwrite`?)\n", argv[2]);
        }
        return 3;
    }

    ged551to700(in, out);
    return 0;
}
*/
