/*
Esad Akar
MARCH, 2017
fw is a program that detects file change in the air and runs a provided commmand

logging stderr to stdout:
fl | grep -v -E 'exe|txt' | fw -c make -r demo >log.txt 2>&1
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#define MAXFILES 100
#define MAXFNAME 100

static void errormessage(char* pname)
{
    fprintf(stderr, "\nUsage: %s -c 'compile command'\n \
    (optional)\n \
    -r 'file to run after compilation'\n\n \
    use ' fl | grep -v ' to filter out unwanted files\n \
    .exe files should not be watched!\n", pname);
}

static int getline(char* b, FILE* io)
{
    int i, c;
    for (i = 0; (c=getc(io)) != EOF && c != '\n'; b[i++] = c)
        ;
    b[i] = '\0';
    return i;
}

// this is rubbish
static void getnewfnames(char** fn, int* c, int* n)
{
    char sys[1000];
    char buf[1000];
    FILE* f;
    for (int i=0; c[i] != -1; i++) {
        sprintf(sys, "fl %s | grep -v '__fw_fl_list' > __fw_fl_list", fn[c[i]]);
        system(sys);


        f = fopen("__fw_fl_list", "rb");
        if (f != NULL) {
            while (getline(fn[*n++], f))
                ;
        }
        fclose(f);
    }
    system("rm __fw_fl_list");
}

static void systemcall(char* sc, char* pname, char* run)
{
    if (run != NULL) {
        clock_t start = clock(), end;
        int ret = system(sc);
        end = clock();
        printf("\n------------------------------------------------------\n");
        printf("%s: %s returned %d in %.2lf(s)\n\n", pname, run, ret, (double)(end - start)/CLOCKS_PER_SEC);
        fflush(stdout);
    }
    else
        system(sc);
}

int main(int argc, char** argv) {

    char *pname = *argv, *run = NULL, *compile = NULL, *log = NULL;

    // initial arguments processing
    while (--argc > 0) {
        if ((*++argv)[0] == '-') {
            switch (*++argv[0]) {
                case 'r':
                    run = *++argv;
                    break;
                case 'c':
                    compile = *++argv;
                    break;
                default:
                    argc = 0;
                    break;
            }
            --argc;
        }
    }
    if (argc != 0 || compile == NULL) {
        errormessage(pname);
        return 1;
    }
    printf("Files to watch:\n");

    // get files to watch from stdin, max 100 files, small project
    char* fnames[MAXFILES];
    struct stat st;
    for (int i=0; i < MAXFILES; i++) {
        fnames[i] = malloc(MAXFNAME);
    }

    // get file list
    int n = 0;
    do  printf("~ ");
    while (getline(fnames[n++], stdin));
    --n;

    // build array that holds original mod time for each file
    time_t mtimes[n];
    for (int i=0; i < n; i++) {
        if (stat(fnames[i], &st) == -1) {
            fprintf(stderr, "%s can't access %s\n", pname, fnames[i]);
            return 1;
        }
        mtimes[i] = st.st_mtime;
    }

    // generate string for system call
    char syscall[300];
    int len = sprintf(syscall, "clear && %s", compile);
    char buf[100];
    if (run != NULL) {
        int b = sprintf(buf, " && %s", run);
        strcpy(syscall+len, buf);
        len += b ;
    }

    // detect file changes
    systemcall(syscall, pname, run);
    clock_t start = clock();

    while (1) {
        if (clock() - start > 500) {
            int mod = 0;
            for (int i=0; i < n; i++) {
                stat(fnames[i], &st);
                if (mtimes[i] != st.st_mtime) {
                    mtimes[i] = st.st_mtime;
                    mod = 1;
                }
            }
            if (mod)
                systemcall(syscall, pname, run);
            start = clock();
        }
    }

    // give heap back :(
    for (int i=0; i < MAXFILES; i++)
        free(fnames[i]);

    return 0;
}
