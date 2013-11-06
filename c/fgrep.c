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

static size_t get_file_size(FILE *fp);
static void fgrep(const char *pattern, char *string_buffer);

int
main(int argc, char **argv)
{
    char *pattern = argv[1];
    char *filename = argv[2];
    FILE *fp;
    size_t file_size;
    char *string_buffer;

    fp = fopen(filename, "r");
    file_size = get_file_size(fp);

    string_buffer = malloc(file_size);

    if(fread(string_buffer, 1, file_size, fp) != file_size) {
        fprintf(stderr, "file read error(%s).\n", filename);
        goto ENDPROC;
    }

    fgrep(pattern, string_buffer);

ENDPROC:
    free(string_buffer);
    fclose(fp);

    return 0;
#ifdef NOWARNING
    (void)argc;
#endif  /* NOWARNING */
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
fgrep(const char *pattern, char *string_buffer)
{
    char *line;

    if((line = strtok(string_buffer, "\n")) != '\0') {
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
