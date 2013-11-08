#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define TRUE  1
#define FALSE 0

#define NOWARNING 1

/* http://ja.wikipedia.org/wiki/ボイヤー-ムーア文字列検索アルゴリズム */
#define ENABLE_ALGORITHM_BOYER_MOORE 1

typedef unsigned long int       bool;

typedef void (line_printf_f)(const char *filename, const char *format, ...);

typedef struct
{
    line_printf_f *line_printf;
} fgrep_info_t;

static void* initialize(int argc, char **argv);
static void fgrep(void *instance, char *pattern, const char *filename);
static size_t get_file_size(FILE *fp);
static void fgrep_buffer(void *instance, char *pattern, char *lines, const char *filename);
static void line_printf_with_filename(const char *filename, const char *format, ...);
static void line_printf_without_filename(const char *filename, const char *format, ...);

#ifdef ENABLE_ALGORITHM_BOYER_MOORE
#include <stdint.h>
static void make_delta1(int *delta1, uint8_t *pat, int32_t patlen);
static void make_delta2(int *delta2, uint8_t *pat, int32_t patlen);
static int is_prefix(uint8_t *word, int wordlen, int pos);
static int suffix_length(uint8_t *word, int wordlen, int pos);
static uint8_t* boyer_moore(uint8_t *string, uint32_t stringlen, uint8_t *pat, uint32_t patlen, int *delta1, int *delta2);
#endif  /* ENABLE_ALGORITHM_BOYER_MOORE */

int
main(int argc, char **argv)
{
    void *instance;
    char *pattern;
    int i;

    pattern = argv[1];

    instance = initialize(argc, argv);

    for(i = 2; i < argc; i++) {
        fgrep(instance, pattern, argv[i]);
    }

    return 0;
}

static void*
initialize(int argc, char **argv)
{
    static fgrep_info_t fgrep_info;

    fgrep_info.line_printf = (argc >= 4) ? line_printf_with_filename : line_printf_without_filename;

    return &fgrep_info;
#ifdef NOWARNING
    (void)argv;
#endif  /* NOWARNING */
}

static void
fgrep(void *instance, char *pattern, const char *filename)
{
    FILE *fp;
    size_t file_size;
    char *lines;

    fp = fopen(filename, "r");
    if(fp == NULL) {
        fprintf(stderr, "file open error(%s).\n", filename);
        goto END;
    }

    file_size = get_file_size(fp);

    lines = malloc(file_size);
    if(lines == NULL) {
        fprintf(stderr, "malloc error.\n");
        goto ENDPROC2;
    }

    if(fread(lines, 1, file_size, fp) != file_size) {
        fprintf(stderr, "file read error(%s).\n", filename);
        goto ENDPROC1;
    }

    fgrep_buffer(instance, pattern, lines, filename);

ENDPROC1:
    free(lines);
ENDPROC2:
    fclose(fp);
END:
    return;
}

static size_t
get_file_size(FILE *fp)
{
    int origin = ftell(fp);
    size_t result;

    fseek(fp, 0, SEEK_END);
    result = ftell(fp);
    fseek(fp, 0, origin);

    return result;
}

#ifdef ENABLE_ALGORITHM_BOYER_MOORE

#define ALPHABET_LEN 255
#define NOT_FOUND patlen
#define max(a, b) ((a < b) ? b : a)

static void
fgrep_buffer(void *instance, char *pattern, char *lines, const char *filename)
{
    fgrep_info_t *fgrep_infop = instance;
    line_printf_f *line_printf = fgrep_infop->line_printf;
    char *line;
    int delta1[ALPHABET_LEN];
    int *delta2;
    int pattern_length = strlen(pattern);

    if((line = strtok(lines, "\n")) == '\0') {
        return;
    }

    delta2 = malloc(pattern_length * sizeof(int));

    make_delta1(delta1, (uint8_t *)pattern, pattern_length);
    make_delta2(delta2, (uint8_t *)pattern, pattern_length);

    if(boyer_moore((uint8_t *)line, strlen(line), (uint8_t *)pattern, pattern_length, delta1, delta2) != NULL) {
        line_printf(filename, "%s\n", line);
    }

    while((line = strtok(NULL, "\n")) != '\0') {
        if(boyer_moore((uint8_t *)line, strlen(line), (uint8_t *)pattern, pattern_length, delta1, delta2) != NULL) {
            line_printf(filename, "%s\n", line);
        }
    }

    free(delta2);

    return;
}

// delta1 table: delta1[c] contains the distance between the last
// character of pat and the rightmost occurence of c in pat.
// If c does not occur in pat, then delta1[c] = patlen.
// If c is at string[i] and c != pat[patlen-1], we can
// safely shift i over by delta1[c], which is the minimum distance
// needed to shift pat forward to get string[i] lined up
// with some character in pat.
// this algorithm runs in alphabet_len+patlen time.
static void
make_delta1(int *delta1, uint8_t *pat, int32_t patlen)
{
    int i;
    for (i=0; i < ALPHABET_LEN; i++) {
        delta1[i] = NOT_FOUND;
    }
    for (i=0; i < patlen-1; i++) {
        delta1[pat[i]] = patlen-1 - i;
    }
}

// delta2 table: given a mismatch at pat[pos], we want to align
// with the next possible full match could be based on what we
// know about pat[pos+1] to pat[patlen-1].
//
// In case 1:
// pat[pos+1] to pat[patlen-1] does not occur elsewhere in pat,
// the next plausible match starts at or after the mismatch.
// If, within the substring pat[pos+1 .. patlen-1], lies a prefix
// of pat, the next plausible match is here (if there are multiple
// prefixes in the substring, pick the longest). Otherwise, the
// next plausible match starts past the character aligned with
// pat[patlen-1].
//
// In case 2:
// pat[pos+1] to pat[patlen-1] does occur elsewhere in pat. The
// mismatch tells us that we are not looking at the end of a match.
// We may, however, be looking at the middle of a match.
//
// The first loop, which takes care of case 1, is analogous to
// the KMP table, adapted for a 'backwards' scan order with the
// additional restriction that the substrings it considers as
// potential prefixes are all suffixes. In the worst case scenario
// pat consists of the same letter repeated, so every suffix is
// a prefix. This loop alone is not sufficient, however:
// Suppose that pat is "ABYXCDEYX", and text is ".....ABYXCDEYX".
// We will match X, Y, and find B != E. There is no prefix of pat
// in the suffix "YX", so the first loop tells us to skip forward
// by 9 characters.
// Although superficially similar to the KMP table, the KMP table
// relies on information about the beginning of the partial match
// that the BM algorithm does not have.
//
// The second loop addresses case 2. Since suffix_length may not be
// unique, we want to take the minimum value, which will tell us
// how far away the closest potential match is.
static void
make_delta2(int *delta2, uint8_t *pat, int32_t patlen)
{
    int p;
    int last_prefix_index = patlen-1;

    // first loop
    for (p=patlen-1; p>=0; p--) {
        if (is_prefix(pat, patlen, p+1)) {
            last_prefix_index = p+1;
        }
        delta2[p] = last_prefix_index + (patlen-1 - p);
    }

    // second loop
    for (p=0; p < patlen-1; p++) {
        int slen = suffix_length(pat, patlen, p);
        if (pat[p - slen] != pat[patlen-1 - slen]) {
            delta2[patlen-1 - slen] = patlen-1 - p + slen;
        }
    }
}

// true if the suffix of word starting from word[pos] is a prefix
// of word
static int
is_prefix(uint8_t *word, int wordlen, int pos)
{
    int i;
    int suffixlen = wordlen - pos;
    // could also use the strncmp() library function here
    for (i = 0; i < suffixlen; i++) {
        if (word[i] != word[pos+i]) {
            return 0;
        }
    }
    return 1;
}

// length of the longest suffix of word ending on word[pos].
// suffix_length("dddbcabc", 8, 4) = 2
static int
suffix_length(uint8_t *word, int wordlen, int pos)
{
    int i;
    // increment suffix length i to the first mismatch or beginning
    // of the word
    for (i = 0; (word[pos-i] == word[wordlen-1-i]) && (i < pos); i++);
    return i;
}

static uint8_t*
boyer_moore(uint8_t *string, uint32_t stringlen, uint8_t *pat, uint32_t patlen, int *delta1, int *delta2)
{
    uint32_t i;

    i = patlen-1;
    while (i < stringlen) {
        int j = patlen-1;
        while (j >= 0 && (string[i] == pat[j])) {
            --i;
            --j;
        }
        if (j < 0) {
            return (string + i+1);
        }

        i += max(delta1[string[i]], delta2[j]);
    }

    return NULL;
}

#else  /* !ENABLE_ALGORITHM_BOYER_MOORE */

static void
fgrep_buffer(void *instance, char *pattern, char *lines, const char *filename)
{
    fgrep_info_t *fgrep_infop = instance;
    line_printf_f *line_printf = fgrep_infop->line_printf;
    char *line;

    if((line = strtok(lines, "\n")) == '\0') {
        return;
    }

    if(strstr(line, pattern) != NULL) {
        line_printf(filename, "%s\n", line);
    }

    while((line = strtok(NULL, "\n")) != '\0') {
        if(strstr(line, pattern) != NULL) {
            line_printf(filename, "%s\n", line);
        }
    }

    return;
}

#endif /* ENABLE_ALGORITHM_BOYER_MOORE */

static void
line_printf_with_filename(const char *filename, const char *format, ...)
{
    va_list arg;
    char format_with_filename[1024];

    sprintf(format_with_filename, "%s: %s", filename, format);

    va_start(arg, format);
    vprintf(format_with_filename, arg);
    va_end(arg);

    return;
}

static void
line_printf_without_filename(const char *filename, const char *format, ...)
{
    va_list arg;

    va_start(arg, format);
    vprintf(format, arg);
    va_end(arg);

    return;
#ifdef NOWARNING
    (void)filename;
#endif  /* NOWARNING */
}
