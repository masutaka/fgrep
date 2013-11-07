#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void fgrep(const char *pattern, const char *filename);
static size_t get_file_size(FILE *fp);
static void fgrep_buffer(const char *pattern, char *lines);

int
main(int argc, char **argv)
{
    char *pattern = argv[1];
    char *filename = argv[2];

    fgrep(pattern, filename);

    return 0;
#ifdef NOWARNING
    (void)argc;
#endif  /* NOWARNING */
}

static void
fgrep(const char *pattern, const char *filename)
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

    fgrep_buffer(pattern, lines);

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
fgrep_buffer(const char *pattern, char *lines)
{
    char *line;

    if((line = strtok(lines, "\n")) == '\0') {
        return;
    }

    if(strstr(line, pattern) != NULL) {
        printf("%s\n", line);
    }

    while((line = strtok(NULL, "\n")) != '\0') {
        if(strstr(line, pattern) != NULL) {
            printf("%s\n", line);
        }
    }

    return;
}
