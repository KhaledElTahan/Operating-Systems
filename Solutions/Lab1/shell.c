#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

char input[100];

int log_file;
typedef struct Command Command;
typedef struct Var Var;


struct Command {
    char* input_type;
    int length;
    char* args[4];

}cmd;
struct Var {
    char identifier[15];
    char value[15];
    Var* nxt;
};

Var* var = NULL;

void on_child_exit() {
    write(log_file, "Child terminated\n", strlen("Child terminated\n"));
    //fflush(log_file);
    printf("child pid : %d\n", getpid());
    int status;
    waitpid(-1, &status, WNOHANG);
}

void register_child_signal(void (*on_child_exit)())
{
    signal(SIGCHLD, on_child_exit);
}

void setup_environment()
{

    char path[100];
    getcwd(path, sizeof(path));
    chdir(path);
    log_file = open("log_file.txt", O_CREAT  | O_RDWR,0666);

}

char* read_input()
{
    scanf("%[^\n]%*c", input);
    return input;
}
int parse_input()
{
    cmd.input_type = NULL;
    for(int i = 0; i < 4; i++) cmd.args[i] = NULL;
    cmd.length = 0;
    char* ptr = strtok(input, " ");
    cmd.args[0] = ptr;
    cmd.length++;
    // if export take only two arguments
    if(strcmp(cmd.args[0], "export") == 0) {
        ptr = strtok(NULL, "\n");
        cmd.args[1] = ptr;
        cmd.input_type = "execute_shell_bultin";
        return 0;
    } else if(strcmp(cmd.args[0], "echo") == 0) {
        while(ptr != NULL) {
            ptr = strtok(NULL, "\"");
            cmd.args[cmd.length++] = ptr;
        }
    } else {

        while (ptr != NULL) {
            ptr = strtok(NULL, " ");
            cmd.args[cmd.length++] = ptr;
        }
    }
    if (strcmp(cmd.args[0], "cd") == 0 || strcmp(cmd.args[0], "echo") == 0
        || strcmp(cmd.args[0], "export") == 0) {
        cmd.input_type = "execute_shell_bultin";
    } else {

        cmd.input_type = "executable_or_error";
    }

    if (cmd.args[1] != NULL && strcmp(cmd.args[1], "&") == 0) {
        return 1;
    } else {
        return 0;
    }



}
int command_is_not_exit()
{
    return strcmp(cmd.args[0], "exit") != 0;
}

Var* find_var(Var* ins, const char* idn)
{
    if(ins == NULL) {
        return  NULL;
    } else if(strcmp(ins->identifier, idn) == 0){
        return ins;
    }else if (ins->nxt == NULL) {
        return ins;
    }
    return find_var(ins->nxt, idn);
}

void evaluate_expression(char* exp)
{

    if(exp == NULL) {
        return;
    }
    char* ptr = strtok(exp, "$");
    if(*exp != '$')
        printf("%s", ptr);
    while (ptr != NULL) {
        if((ptr-1 != NULL) && *(ptr-1) == '$') {
            char* ins_ptr = strtok(ptr, " ");
            if(ins_ptr != NULL) {
                ptr = ins_ptr;
            }
        }else {

            char* ins_ptr = strtok(NULL, "$");
            if(ins_ptr != NULL) {
                ptr = ins_ptr;
                ins_ptr = strtok(ins_ptr, " ");
                if(ins_ptr != NULL){
                    ptr = ins_ptr;
                }
            }else {
                if(exp != ptr)
                    printf("%s\n", ptr);
                else
                    printf("\n");

                return;
            }
        }

        Var *ins = find_var(var, ptr);
        if (ins == NULL || strcmp(ins->identifier, ptr) != 0) {
            // no variable
            printf("Invalid expression : No variable '%s'\n", ptr);
            return;
        } else {
            // variable exist
            printf("%s ",ins->value);
            if(ptr != NULL)
                ptr = strtok(NULL, "$");
        }
    }
    printf("\n");
}

void execute_shell_bultin() {
    if(strcmp(cmd.args[0], "cd") == 0) {
        // cd command
        chdir(cmd.args[1]);
    } else if (strcmp(cmd.args[0], "echo") == 0) {
        // echo command
        evaluate_expression(cmd.args[1]);
    } else if (strcmp(cmd.args[0], "export") == 0) {
        // export command
        char* var_name = strtok(cmd.args[1], "=");

        char* value ;
        if(var_name != NULL)
            value = strtok(NULL, "=");

        Var* ins = find_var(var, var_name);
        if(ins == NULL) {
            // add first
            var = (Var*) malloc(sizeof (Var));
            strcpy(var->identifier, var_name);
            if(*value == '\"'){
                char* ins_value = strtok(value+1, "\"");
                strcpy(var->value, ins_value);


            }else {
                strcpy(var->value, value);

            }

            var->nxt = NULL;
        } else if(strcmp(ins->identifier, var_name) == 0) {
            strcpy(ins->value, value);
        } else {
            ins->nxt = (Var*) malloc(sizeof (Var));

            ins = ins->nxt;
            strcpy(ins->identifier, var_name);
            if(*value == '\"'){
                char* ins_value = strtok(value+1, "\"");
                strcpy(ins->value, ins_value);

            }else {
                strcpy(ins->value, value);

            }
            ins->nxt = NULL;
        }
    }


}

int execute_command(int in_background)
{
    int child_id = fork();
    if (child_id == 0) {
        int res = execvp(cmd.args[0], cmd.args);
        if(res < 0 && strcmp(cmd.args[0], "exit") != 0)
            printf("Error Message not found\n");
        exit(0);

    } else if (!in_background) {
        int status;
        waitpid(child_id, &status, 0);
    }
    return 0;
}


void shell()
{
    do {
        int in_background = parse_input(read_input());
        if(strcmp(cmd.args[0], "echo") != 0) {


            for (int i = 1; i < 4; i++) {
                if (cmd.args[i] != NULL && *(cmd.args[i]) == '$') {
                    char *ptr = cmd.args[i] + 1;
                    Var *ins = find_var(var, ptr);
                    if (ins == NULL) {
                        printf("Invalid expression : No variable '%s'\n", ptr);
                    } else {
                        char* ptr = strtok(ins->value, " ");
                        int j = i;
                        while (ptr != NULL) {
                            cmd.args[j++] = ptr;
                            ptr = strtok(NULL, " ");
                        }


                    }
                }
            }
        }

        if (strcmp(cmd.input_type, "execute_shell_bultin") == 0) {
            execute_shell_bultin();
        } else if (strcmp(cmd.input_type, "executable_or_error") == 0){
            execute_command(in_background);
        }

    }while(command_is_not_exit());
}

int main(int argc, char* argv[])
{

    register_child_signal(on_child_exit);
    setup_environment();
    shell();

    return 0;
}
