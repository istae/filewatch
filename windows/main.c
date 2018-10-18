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
#include <dirent.h>
#include <string.h>
#include <windows.h>
#include <conio.h>
#include <process.h>
#include <file.h>

#define KEY_ESC 0x1B
#define KEY_ENT '\r'

// reads input from stdin until EOF or \n
int _getline(FILE*, char*, int);
// prints program usage if initial parameters are wrong or missing
void usage();
// main system interface, makes system calls defined by user in initial arguments, seperate thread
void sys_call();
// reads keyboard input when -r program is not running, seperate thread
void input();
// records file mod time and copies file name to fnames
void register_file(char* );
// this gets called at the start, and whenver user wants to change compilation or program name parameters
void get_commands();
void print_keys();
void print_commands();

char fnames[MAX_FILE_PATHS][MAX_FILE_NAME] = {{0}};
char compile[1000];
char run[1000];
char args[1000];
char kill[1000];
time_t mtimes[MAX_FILE_PATHS];
int n;
char *pname;
int is_running;


int main(int argc, char** argv)
{   
    usage();
    pname = argv[0];
    n = 0;
    char new_file[MAX_FILE_NAME];

    // file registeration
    if (argc > 1) {
        int fcount = dirlist(argv[1], FILES_ONLY, fnames);
        for(int i=0; i < fcount; i++)
            register_file(fnames[i]);
    }
    else {
        printf("Files to watch:\n~ ");
        while(_getline(stdin, new_file, MAX_FILE_NAME)) {
            register_file(new_file);
            printf("~ ");
        }
    }

    run[0] = compile[0] = args[0] =  '\0';
    get_commands();

    is_running = 1;
    _beginthread(input, 0, 0);
    _beginthread(sys_call, 0, 0);

    while (1) {
        Sleep(1000);
        int mod = 0;
        for (int i=0; i < n; i++) {
            struct stat st;
            if (!fnames[i][0])
                continue;
            stat(fnames[i], &st);
            if (mtimes[i] != st.st_mtime) {
                mtimes[i] = st.st_mtime;
                mod = 1;
            }
        }

        if (mod) {
            if (run)
                system(kill);
            int ret = system(compile);
            if (ret == 0)
                _beginthread(sys_call, 0, 0);
        }
    }
}

void get_commands()
{
    print_commands();
    char b[1000];
    while(_getline(stdin, b, 1000)) {

        if (strncmp(b, "c=", 2)==0)
            sprintf(compile,"cls && %s", b+2);

        else if (strncmp(b, "r=", 2)==0) {
            strcpy(run, b+2);
            sprintf(kill,"taskkill /im %s /f", run);

            // remove the executible from watchlist 
            for(int i=0; i < n; i++)
                if (strcmp(fnames[i], b+2) == 0)
                    fnames[i][0] = '\0';
        }

        else if (strncmp(b, "a=", 2)==0)
            strcpy(args, b+2);

        else if (strncmp(b, "f=", 2)==0)
            register_file(b+2);
        else if (strncmp(b, "s=", 2)==0)
            system(b+2);
        else if (strcmp(b, "done")==0)
            break;
        else
            printf("wrong command\n");
    }

    if (compile[0]=='\0') {
        printf("\nplease at least provide a compilation command\nprevious commands have been saved\n");
        get_commands();
    }
}

void sys_call()
{
    if (run[0]) {
        clock_t start = clock(), end;
        char b[1000];
        sprintf(b, "cls && %s %s", run, args);
        int ret = system(b);
        end = clock();
        printf("\n------------------------------------------------------\n");
        printf("%s: %s returned %d in %.2lf(s)\n\n", pname, b, ret, (float)(end - start)/CLOCKS_PER_SEC);
    }

    print_keys();
}

void input()
{
    int ir;
    for(;;) {

        int c = _getch();

        if (c == 'c') {
            system(compile);
            print_keys();
        }

        else if (c == KEY_ESC) {
            get_commands();
            print_keys();
        }


        else if (run[0]) {
            if (c == 'q')
                system(kill);
            else if (c == 'r') {
                system(kill);
                _beginthread(sys_call, 0, 0);
            }
            else if (c == 'e') {
                system(kill);
                int ret = system(compile);
                if (ret == 0)
                    _beginthread(sys_call, 0, 0);
            }
        }
    }
}

void register_file(char* new_file)
{
    // do not register exe files
    if (strstr(new_file, ".exe"))
        return;

    struct stat st;
    if (stat(new_file, &st) == -1)
        fprintf(stderr, "%s can't access %s\n", pname, new_file);
    else {
        strcpy(fnames[n], new_file);
        mtimes[n++] = st.st_mtime;
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

void print_keys()
{
    printf("\nKeys\n");
    printf("----------------------------\n");
    printf("c   'compile'\n");
    printf("q   'kill running program\n");
    printf("r   'restart program'\n");
    printf("e   'compile and restart\n");
    printf("ESC 'exit Keys mode and get back to commands'\n\n");
    printf("Listening for changes...\n\n");
}

void print_commands()
{
    printf("\nCommands\n");
    printf("----------------------------\n");
    printf("c='compilation command' (ex: make)\n");
    printf("r='program to run' (ex: demo.exe)\n");
    printf("a='program flags' (ex: -f file.txt)\n");
    printf("s='shell command'\n");
    printf("f='new file to watch'\n");
    printf("type 'done' to continue \n\n");
}

void usage()
{
}
