#ifndef MODEL_H
#define MODEL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <signal.h>
#include <semaphore.h>

// Constants
#define MAX_CMD_LEN 256
#define MAX_ARGS 10
#define MAX_JOBS 100
#define SHARED_FILE_NAME "/shared_memory"
#define BUF_SIZE 1024

// Shell job structure
struct job {
    pid_t pid;
    char command[256];
    int is_running;
};

// Model function declarations
void list_jobs();
void bg_command(int job_id);
void fg_command(int job_id);
void remove_job(int index);
void parse_command(char *command, char *args[]);
void add_job(pid_t pid, const char *command);
void execute_command(char *command);
void module_send_message(const char *msg);
void module_read_messages(char *buffer);

extern struct job jobs[MAX_JOBS];
extern int job_count;

#endif // MODEL_H
