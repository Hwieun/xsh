#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>


char* xsh_read_line() {
    char *line = NULL;
    size_t buflen = 0;
    getline(&line, &buflen, stdin);
    return line;
}

char** xsh_split_line(char* line) {
    int length = 0;
    int capacity = 16;
    char **tokens = malloc(capacity * sizeof(char*));

    char *delimiters = " \t\r\n";
    char *token = strtok(line, delimiters);

    while(token != NULL) {
        tokens[length] = token;
        length++;

        if (length >= capacity) {
            capacity = (int) (capacity * 1.5);
            tokens = realloc(tokens, capacity * sizeof(char*));
        }

        token = strtok(NULL, delimiters);
    }

    tokens[length] = NULL;
    return tokens;
}


void xsh_exit(char **args) {
    exit(0);
}

/**
 * @brief cd. move dir
 * 
 * working dir is a property of the shell process itself. 
 * so it couldn't have been implemented as external program.
 * 
 * @param args 
 */
void xsh_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "xsh: cd: missing argument\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("xsh: cd");
        }
    }
}

void xsh_help(char **args) {
    char *helptext = 
    "Xsh - the X version of Shell. "
    "The following commands are available: \n"
    "  cd       Change the working directory.\n"
    "  exit     Exit the shell.\n"
    "  help     Print this help text.\n";
    printf("%s", helptext);
}

struct builtin {
    char *name;
    void (*func)(char **args);
};

struct builtin builtins[] = {
    {"help", xsh_help},
    {"exit", xsh_exit},
    {"cd", xsh_cd},
};


int xsh_num_builtins() {
    return sizeof(builtins) / sizeof(struct builtin);
}

/**
 * @brief exec command
 * 
 * pwd, ls, echo 같은 단순 명령어는 바로 실행
 * 
 * @param args 
 */
void xsh_exec(char **args) { 
    for(int i = 0; i < xsh_num_builtins(); i++) {
        if(strcmp(args[0], builtins[i].name) == 0) { // 명령어가 builtins[i] 와 일치한 경우
            builtins[i].func(args);
            return;
        }
    }

    pid_t child_pid = fork(); // child process init

    if(child_pid == 0) { // we're in the child process
        execvp(args[0], args); // 첫 인자에 대한 실행 파일을 $PATH 환경 변수의 디렉토리에서 찾아서 실행.
        perror("xsh");
        exit(1);
    } else if (child_pid > 0) { // parent process
        int status;
        do {
            waitpid(child_pid, &status, WUNTRACED); // child process의 status가 바뀌기를 대기
        } while (!WIFEXITED(status) && !WIFSIGNALED(status)); // WIFEXITED: child has exited. WIFSIGNALED: child has been terminated
    } else { // error occurred
        perror("xsh");
    }
}

int main() {
    char cwd[1024];

    while(true){
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s", cwd);
            printf(" ");
        }
        
        printf("xsh> ");
        char *line = xsh_read_line();
        char **tokens = xsh_split_line(line);

        if (tokens[0] != NULL) {
            xsh_exec(tokens);
        }

        free(tokens);
        free(line);
    }
}