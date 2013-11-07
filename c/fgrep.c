#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define TRUE  1
#define FALSE 0

#define NOWARNING 1

typedef char                    i8;
typedef int                     i16;
typedef long int                i32;
typedef unsigned char           u8;
typedef unsigned int            u16;
typedef unsigned long int       u32;

typedef unsigned long int       bool;

typedef void (line_printf_f)(const char *filename, const char *format, ...);

typedef struct
{
    line_printf_f *line_printf;
} fgrep_info_t;

static void* initialize(int argc, char **argv);
static void fgrep(void *instance, const char *pattern, const char *filename);
static size_t get_file_size(FILE *fp);
static void fgrep_buffer(void *instance, const char *pattern, char *lines, const char *filename);
static void line_printf_with_filename(const char *filename, const char *format, ...);
static void line_printf_without_filename(const char *filename, const char *format, ...);

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
fgrep(void *instance, const char *pattern, const char *filename)
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

static void
fgrep_buffer(void *instance, const char *pattern, char *lines, const char *filename)
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
