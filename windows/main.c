/*
Esad Akar
MARCH, 2017
fw is a program that detects file change and runs a provided commmand

logging stderr to stdout:
fl | grep -v -E 'exe|txt' | fw -c make -r demo >log.txt 2>&1
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <windows.h>
#include <conio.h>
#include <process.h>

#define MAXFILES 100
#define MAXFNAME 100
#define KEY_ESC 0x1B

typedef struct {
    char* pname;
    char* runcall;
    char* compcall;
} Params;

Params p;
char kill[100];
int is_running;
HANDLE run_mutex;

// reads input from stdin until EOF or \n
int _getline(FILE*, char*, int);

// prints program usage if initial parameters are wrong or missing
void usage();

// main system interface, makes system calls defined by user in initial arguments, seperate thread
void system_call();

// reads keyboard input when -r program is not running, seperate thread
void input();


int main(int argc, char** argv)
{
    char *pname = *argv, *run = NULL, *compile = NULL;

    // initial arguments processing
    while (--argc > 0) {
        if ((*++argv)[0] == '-') {
            switch (*++argv[0]) {
                case 'c':
                    compile = *++argv;
                    break;
                case 'r':
                    run = *++argv;
                    break;
                default:
                    argc = -1;
                    break;
            }
            --argc;
        }
    }
    if (argc != 0 || compile == NULL) {
        usage();
        return 1;
    }

    // get files to watch from stdin, max 100 files, small project
    char* fnames[MAXFILES];

    for (int i=0; i < MAXFILES; i++) {
        fnames[i] = calloc(MAXFILES, MAXFNAME);
    }

    // get file list
    // if -r program is provided, remove from list
    // build array that holds original mod time for each file
    printf("Files to watch:\n~ ");
    int n;
    struct stat st;
    time_t mtimes[MAXFILES];
    for (n=0; _getline(stdin, fnames[n], MAXFNAME); n++) {
        if (run && strcmp(fnames[n], run) == 0)
            --n;
        else if (stat(fnames[n], &st) == -1)
            fprintf(stderr, "%s can't access %s\n", pname, fnames[n]);
        else
            mtimes[n] = st.st_mtime;
        printf("~ ");
    }

    // create string for system call
    char compcall[100];
    sprintf(compcall, "cls && %s", compile);

    // create kill compcall
    if (run)
        sprintf(kill,"taskkill /im %s /f", run);

    // detect file changes
    p.pname = pname;
    p.runcall = run;
    p.compcall = compcall;

    is_running = 0;
    run_mutex = CreateMutex(0, FALSE, 0);
    _beginthread(system_call, 0, 0);
    _beginthread(input, 0, 0);

    while (1) {
        Sleep(1000);
        int mod = 0;
        for (int i=0; i < n; i++) {
            stat(fnames[i], &st);
            if (mtimes[i] != st.st_mtime) {
                mtimes[i] = st.st_mtime;
                mod = 1;
            }
        }

        if (mod) {
            if (run)
                system(kill);
            _beginthread(system_call, 0, 0);
        }
    }

    CloseHandle(run_mutex);
}

void input()
{
    for(;;) {

        WaitForSingleObject(run_mutex, INFINITE);
        int ir = is_running;
        ReleaseMutex(run_mutex);

        if (!ir) {
            int c =_getch();
            if (c == 'q')
                system(kill);
            else if (c == 'r') {
                if (p.runcall) {
                    is_running = 1; // beginthread takes too long so the loop starts again with ir begin 0, which is wrong
                    system(kill);
                }
                _beginthread(system_call, 0, 0);
            }
            else if (c == KEY_ESC) {
                if (p.runcall)
                    system(kill);
                exit(0);
            }
        }
    }
}

void system_call()
{
    system(p.compcall);

    if (p.runcall) {

        WaitForSingleObject(run_mutex, INFINITE);
        is_running = 1;
        ReleaseMutex(run_mutex);

        clock_t start = clock(), end;
        int ret = system(p.runcall);
        end = clock();
        printf("\n\n------------------------------------------------------\n");
        printf("%s: %s returned %d in %.2lf(s)\n\n", p.pname, p.runcall, ret, (float)(end - start)/CLOCKS_PER_SEC);
        fflush(stdout);

        WaitForSingleObject(run_mutex, INFINITE);
        is_running = 0;
        ReleaseMutex(run_mutex);
    }
}

int _getline(FILE* f, char* b, int max)
{
    int i, c;
    for (i = 0; i < max && (c=getc(f)) != EOF && c != '\n'; i++)
        b[i] = c;
    if (i == 0 && c == '\n')
        b[i++] = '\n';
    b[i] = '\0';
    return i;
}

void usage()
{
    char str[] = "\n\
  FLAGS\n\
    -C    - compilation command ex: make, 'gcc main.c'\n\
    -r    - (optional) program to run after compilation, including the file extension\n\
\n\
  KEYS\n\
    'ESC' - kill filewatch\n\
    'q'   - if -r is provided, kill the -r program\n\
    'r'   - restart -r program by terminating the program by name\n\
            CAUTION: name clashing with other programs may occur\n\
\n\
  TIP\n\
    use ' fl | grep -v ' to filter out unwanted files\n\
    Programs that use stdin will not work because a single console window is shared with filewatch\n\
    DO NOT WATCH EXECUTIBLES\n";
    fprintf(stderr, "%s", str);
}
