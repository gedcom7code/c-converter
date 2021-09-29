# GEDCOM 5.5.1 to GEDCOM 7.0 converter

GEDCOM 7.0 is a breaking change with GEDCOM 5.5.1. This means that 5.5.1 files cannot be parsed as-is as if they were 7.0 files.
This project is a zero-dependency public-domain ANSI-C implementation of a 5.5.1 to 7.0 converter.
C was chosen because it as very few features, so it should be able to convert the code to other languages easily;
and because many other languages have methods for calling C code natively.

Current status:

- Single-pass operations
    - [x] Detect character encodings, as documented in [ELF Serialisation](https://fhiso.org/TR/elf-serialisation).
    - [x] Convert to UTF-8
    - [x] Normalize line whitespace, including stripping leading spaces
    - [x] Remove `CONC`
    - [x] Normalize case of tags
    - [x] Limit character set of cross-reference identifiers
    - [x] Fix `@` usage
    - [x] Convert `LANG` payloads to BCP 47 tags, using [FHISO's mapping](https://github.com/fhiso/legacy-format/blob/master/languages.tsv)
    - [x] Convert `DATE`
        - [x] replace date_phrase with `PHRASE` structure
        - [x] replace calendar escapes with calendar tags
        - [x] change `BC` and `B.C.` to `BCE` and remove if found in unsupported calendars
        - [x] replace dual years with single years and `PHRASE`s
        - [x] replace just-year dual years in unqualified date with `BET`/`AND`
    - [x] Convert `AGE`
        - [x] change age words to canonical forms (stillborn as `0y`, child as `< 8y`, infant as `< 1y`) with `PHRASE`s
        - [x] Normalize spacing in `AGE` payloads
        - [x] add missing `y`
    - [x] Convert `MEDI`.`FORM` payloads to media types
    - (deferred) Convert `INDI`.`NAME`
        - (deferred) replace `/`surname`/` with name part
        - (deferred) combine payload and parts
        - (deferred) convert `_RUFNAME` to `RUFNAME`
    - (deferred) Convert `PLAC` structures to `PLACE` records and `WHERE` pointers thereto
    - [x] Enumerated values
        - [x] Normalize case
        - [x] Convert user-text to `PHRASE`s
    - [x] change `SOUR` with text payload into pointer to `SOUR` with `NOTE`
    - [x] change `NOTE` record or with pointer payload into `SNOTE`
    - [x] change `OBJE` with no payload to pointer to new `OBJE` record
    - [x] Convert `FONE` and `ROMN` to `TRAN` and their `TYPE`s to BCP-47 `LANG`s
    - [x] tag renaming, including
        - `EMAI`, `_EMAIL` → `EMAIL`
        - `FORM`.`TYPE` → `FORM`.`MEDI`
        - (deferred) `_SDATE` → `SDATE` -- `_SDATE` is also used as "accessed at" date for web resources by some applications so this change is not universally correct
        - `_UID` → `UID`
        - `_ASSO` → `ASSO`
        - `_CRE`, `_CREAT` → `CREA`
        - `_DATE` → `DATE`
        - other?
    - [x] `ASSO`.`RELA` → `ASSO`.`ROLE` (changing payload OTHER + PHRASE)
    - [x] change `RFN`, `RIN`, and `AFN` to `EXID`
    - [x] change `_FSFTID`, `_APID` to `EXID`
    - [x] remove `SUBN`, `HEAD`.`FILE`, `HEAD`.`CHAR`
        - (deferred) `HEAD`.`PLAC` was originally on this list, but has been deferred to a later version
    - [x] change `FILE` payloads into URLs
        - [x] Windows-style `\` becomes `/`
        - [x] Windows diver letter `C:\WINDOWS` becomes `file:///c:/WINDOWS`
        - [x] POSIX-stye `/User/foo` becomes `file:///User/foo`
    - [x] update the `GEDC`.`VERS` to `7.0`
    - [x] (extra) change string-valued `INDI`.`ALIA` into `NAME` with `TYPE` `AKA`
    - [ ] (5.5) change base64-encoded OBJE into GEDZIP
    - [ ] Change any illegal tag `XYZ` into `_EXT_XYZ`
- two-pass operations
    - [ ] use heuristic to change some pointer-`NOTE` to nested-`NOTE` instead of `SNOTE`
    - [x] add `SCHMA` for all used known extensions
        - [ ] add URIs (or standard tags) for all extensions from <https://wiki-de.genealogy.net/GEDCOM/_Nutzerdef-Tag> and <http://www.gencom.org.nz/GEDCOM_tags.html>

# Usage

## Building using the Makefile

Edit `Makefile` as needed; likely changes include

- Change from `CC := clang` to your C compiler
- If on Windows, change the target from `ged5to7` to `ged5to7.exe`

Then run `make`.

## Building using Visual Studio

To instead build using Visual Studio, simply open the c-converter.sln
file with Visual Studio and build the solution normally.

## Running

To run, execute the resulting `ged5to7`.
Run `ged5to7 --help` for a list of command-line options.

# Design Notes

The code is designed to be thread-safe (no mutable globals or `static` locals) though threading has not yet been added.

The code is currently first-draft status by someone who usually does not write large code bases others read.
It has inconsistent naming (e.g., `ged_destroy_event` vs `changePayloadToDynamic`),
some shortcuts (e.g., some `struct`s are allocated as `long`s and cast to `struct`),
inconsistent style (e.g., three different ways to emit locally-created `GedEvent`s),
etc.
In some places some energy was spent making it efficient, in other places it is definitely *not* as efficient as it easily could be.
Overall, the code needs a major refactor before it is easy to read.
