CC := clang -O2 -pedantic -Wall -Werror
PIPELINE_C := $(wildcard pipeline/*.c)
OBJECTS := commandline.o ansel2utf8.o ged_ebp.o ged_ebp_parse.o ged_ebp_emit.o strtrie.o geddate.o gedage.o

.PHONY: all clean distclean

all: ged5to7

clean:
	rm -f *.o pipeline/*.o

distclean: clean
	rm -f ged5to7

ged5to7: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

ged_ebp.o: ged_ebp.c ged_ebp.h pipeline/config.h $(PIPELINE_C)
	$(CC) -c -o $@ $<

%.o: %.c %.h
	$(CC) -c -o $@ $<
