#include "model.h"

// Global variables for job management
struct job jobs[MAX_JOBS];
int job_count = 0;

void list_jobs() {
    printf("Aktif İşlemler:\n");
    for (int i = 0; i < job_count; i++) {
        printf("[%d] %s %s (PID: %d)\n", i + 1, 
               jobs[i].is_running ? "Çalışıyor" : "Durdurulmuş", 
               jobs[i].command, 
               jobs[i].pid);
    }
}

void bg_command(int job_id) {
    if (job_id < 1 || job_id > job_count) {
        printf("Hata: Geçersiz iş ID'si.\n");
        return;
    }

    int idx = job_id - 1;
    printf("[%d] %s arka planda devam ettirildi (PID: %d)\n", job_id, jobs[idx].command, jobs[idx].pid);

    kill(jobs[idx].pid, SIGCONT);
    jobs[idx].is_running = 1;
}

void fg_command(int job_id) {
    if (job_id < 1 || job_id > job_count) {
        printf("Hata: Geçersiz iş ID'si.\n");
        return;
    }

    int idx = job_id - 1;
    printf("[%d] %s ön plana alındı (PID: %d)\n", job_id, jobs[idx].command, jobs[idx].pid);

    tcsetpgrp(STDIN_FILENO, jobs[idx].pid);
    kill(jobs[idx].pid, SIGCONT);
    jobs[idx].is_running = 1;

    waitpid(jobs[idx].pid, NULL, 0);
    remove_job(idx);
}

void remove_job(int index) {
    if (index < 0 || index >= job_count) return;
    for (int i = index; i < job_count - 1; i++) {
        jobs[i] = jobs[i + 1];
    }
    job_count--;
}

void add_job(pid_t pid, const char *command) {
    if (job_count < MAX_JOBS) {
        jobs[job_count].pid = pid;
        strncpy(jobs[job_count].command, command, sizeof(jobs[job_count].command) - 1);
        jobs[job_count].is_running = 1;  // Çalışıyor olarak işaretle
        job_count++;
    } else {
        printf("Hata: Maksimum iş sayısına ulaşıldı!\n");
    }
}

void parse_command(char *command, char *args[]) {
    int i = 0;
    char *token = strtok(command, " ");
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
}

void execute_command(char *command) {
    char *args[MAX_ARGS];
    pid_t pid;
    int background = 0;

    // Komut boş ise hiçbir şey yapma
    if (strlen(command) == 0) {
        return;
    }

    parse_command(command, args);

    int i = 0;
    while (args[i] != NULL) i++;
    if (i > 0 && strcmp(args[i - 1], "&") == 0) {
        background = 1;
        args[i - 1] = NULL;
    }

    if (args[0] == NULL) {
        return; // Boş komut
    }

    if (strcmp(args[0], "jobs") == 0) {
        list_jobs();
        return;
    }
    if (strcmp(args[0], "bg") == 0 && args[1] != NULL) {
        bg_command(atoi(args[1]));
        return;
    }
    if (strcmp(args[0], "fg") == 0 && args[1] != NULL) {
        fg_command(atoi(args[1]));
        return;
    }

    pid = fork();
    if (pid == 0) {
        // Çocuk süreç
        execvp(args[0], args);
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        // Ebeveyn süreç
        if (background) {
            add_job(pid, command);
            printf("[%d] %s arka planda çalışıyor...\n", pid, command);
        } else {
            waitpid(pid, NULL, 0);
        }
    } else {
        // Fork hatası
        perror("fork");
    }
}

void module_send_message(const char *msg) {
    int fd = shm_open(SHARED_FILE_NAME, O_CREAT | O_RDWR, 0600);
    if (fd == -1) {
        perror("shm_open");
        return;
    }
    
    if (ftruncate(fd, BUF_SIZE) == -1) {
        perror("ftruncate");
        close(fd);
        return;
    }
    
    char *msgbuf = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (msgbuf == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return;
    }
    
    strncpy(msgbuf, msg, BUF_SIZE - 1);
    msgbuf[BUF_SIZE - 1] = '\0';
    
    munmap(msgbuf, BUF_SIZE);
    close(fd);
}

void module_read_messages(char *buffer) {
    int fd = shm_open(SHARED_FILE_NAME, O_RDONLY, 0600);
    if (fd == -1) {
        // İlk kez çağrıldığında paylaşılan bellek henüz oluşturulmamış olabilir
        strcpy(buffer, "Henüz mesaj yok.");
        return;
    }
    
    char *msgbuf = mmap(NULL, BUF_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if (msgbuf == MAP_FAILED) {
        perror("mmap");
        strcpy(buffer, "Mesajlar okunamadı.");
        close(fd);
        return;
    }
    
    strncpy(buffer, msgbuf, BUF_SIZE - 1);
    buffer[BUF_SIZE - 1] = '\0';
    
    munmap(msgbuf, BUF_SIZE);
    close(fd);
}
