#include "model.h"
#include <gtk/gtk.h>
#include <time.h>
#include "view.h"
#include <semaphore.h>

// Paylaşılan bellek yapısı
typedef struct {
    sem_t sem;         // Senkronizasyon için semafor
    size_t cnt;        // msgbuf'da kullanılan bayt sayısı
    char msgbuf[BUF_SIZE - sizeof(sem_t) - sizeof(size_t)]; // Aktarılan veri
} ShmBuf;

GString *global_message_history = NULL;
static ShmBuf *shmp = NULL;
// Global variables for job management
struct job jobs[MAX_JOBS];
int job_count = 0;


void init_shared_memory() {
    if (shmp != NULL) return; // Zaten başlatılmış
    
    int fd = shm_open(SHARED_FILE_NAME, O_CREAT | O_RDWR, 0600);
    if (fd == -1) {
        perror("shm_open hatası");
        return;
    }
    
    if (ftruncate(fd, BUF_SIZE) == -1) {
        perror("ftruncate hatası");
        close(fd);
        return;
    }
    
    // Bellekte eşle
    shmp = (ShmBuf *)mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shmp == MAP_FAILED) {
        perror("mmap hatası");
        close(fd);
        return;
    }
    
    // Semafor başlatma (ilk kez çalıştırılıyorsa)
    sem_init(&shmp->sem, 1, 1); // 1: süreçler arası paylaşımlı, 1: başlangıç değeri
 
  if (global_message_history == NULL) {
        global_message_history = g_string_new("");
    }
    
    close(fd); // fd'yi kapatabiliriz, mmap'ten sonra gerekli değil
} 


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
    int in_quotes = 0;
    char *start = command;
    
    while (*command) {
        if (*command == '"') {
            // Tırnak işaretlerini işle
            in_quotes = !in_quotes;
            *command = '\0';  // Tırnağı null ile değiştir
            if (in_quotes) {
                start = command + 1;
            } else {
                args[i++] = start;
            }
        }
        else if ((*command == ' ' || *command == '<' || *command == '>' || *command == '|') && !in_quotes) {
            // Boşluk veya özel karakterler ise
            if (*command == '<' || *command == '>' || *command == '|') {
                // Özel karakter, öncesindeki argümanı kaydet
                if (command > start && *(command-1) != '\0') {
                    *command = '\0';
                    args[i++] = start;
                    start = command + 1;
                }
                
                // Özel karakteri ayrı bir argüman olarak kaydet
                char special[2] = {*command, '\0'};
                args[i++] = strdup(special);
                
                // Eğer >> ise, ikinci > karakterini atla
                if (*command == '>' && *(command+1) == '>') {
                    args[i-1] = strdup(">>");  // ">" yerine ">>" kullan
                    command++;  // İkinci > karakterini atla
                }
                
                *command = '\0';
                start = command + 1;
            } else {
                // Normal boşluk
                *command = '\0';
                if (command > start && *(command-1) != '\0') {
                    args[i++] = start;
                }
                start = command + 1;
            }
        }
        command++;
    }
    
    // Son argümanı ekle
    if (command > start && *start) {
        args[i++] = start;
    }
    
    args[i] = NULL;  // Listeyi sonlandır
}

void execute_command(char *command) {
    if (strlen(command) == 0) {
        return;
    }
    
    // Dahili komutları kontrol et
    if (strcmp(command, "jobs") == 0) {
        list_jobs();
        return;
    } else if (strncmp(command, "bg ", 3) == 0) {
        bg_command(atoi(command + 3));
        return;
    } else if (strncmp(command, "fg ", 3) == 0) {
        fg_command(atoi(command + 3));
        return;
    } else if (strncmp(command, "@msg ", 5) == 0) {
        // Mesaj komutu
        module_send_message(command + 5);
        printf("Mesaj gönderildi: %s\n", command + 5);
        return;
    }
    
    // Pipe karakteri var mı kontrol et
    char *pipe_pos = strchr(command, '|');
    if (pipe_pos != NULL) {
        // Pipe ile komutları böl
        *pipe_pos = '\0';
        char *cmd1 = command;
        char *cmd2 = pipe_pos + 1;
        
        // Boşlukları temizle
        while (*cmd2 == ' ') cmd2++;
        
        // Pipe oluştur
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            return;
        }
        
        // İlk komutu çalıştır
        pid_t pid1 = fork();
        if (pid1 == 0) {
            // Çocuk süreç 1
            close(pipefd[0]);  // Okuma ucunu kapat
            dup2(pipefd[1], STDOUT_FILENO);  // stdout'u pipe'a yönlendir
            close(pipefd[1]);  // Kopyalandığı için orijinali kapat
            
            // Komutu ayrıştır ve çalıştır
            char *args[MAX_ARGS];
            parse_command(cmd1, args);
            
            int in_fd = STDIN_FILENO, out_fd = STDOUT_FILENO;
            if (handle_redirections(args, &in_fd, &out_fd) == -1) {
                exit(1);
            }
            
            if (in_fd != STDIN_FILENO) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            
            execvp(args[0], args);
            perror("execvp");
            exit(1);
        }
        
        // İkinci komutu çalıştır
        pid_t pid2 = fork();
        if (pid2 == 0) {
            // Çocuk süreç 2
            close(pipefd[1]);  // Yazma ucunu kapat
            dup2(pipefd[0], STDIN_FILENO);  // stdin'i pipe'a yönlendir
            close(pipefd[0]);  // Kopyalandığı için orijinali kapat
            
            // Komutu ayrıştır ve çalıştır
            char *args[MAX_ARGS];
            parse_command(cmd2, args);
            
            int in_fd = STDIN_FILENO, out_fd = STDOUT_FILENO;
            if (handle_redirections(args, &in_fd, &out_fd) == -1) {
                exit(1);
            }
            
            if (out_fd != STDOUT_FILENO) {
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }
            
            execvp(args[0], args);
            perror("execvp");
            exit(1);
        }
        
        // Ana süreç
        close(pipefd[0]);
        close(pipefd[1]);
        
        // Çocuk süreçleri bekle
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
    } else {
        // Normal komut (pipe olmadan)
        char *args[MAX_ARGS];
        int background = 0;
        
        // Komutu ayrıştır
        parse_command(command, args);
        
        // Arka plan çalışma kontrolü (&)
        int i = 0;
        while (args[i] != NULL) i++;
        if (i > 0 && strcmp(args[i - 1], "&") == 0) {
            background = 1;
            args[i - 1] = NULL;
        }
        
        if (args[0] == NULL) {
            return; // Boş komut
        }
        
        // Yönlendirmeleri işle
        int input_fd = STDIN_FILENO, output_fd = STDOUT_FILENO;
        if (handle_redirections(args, &input_fd, &output_fd) == -1) {
            return;
        }
        
        // Komutu çalıştır
        pid_t pid = fork();
        if (pid == 0) {
            // Çocuk süreç
            
            // Yönlendirmeleri uygula
            if (input_fd != STDIN_FILENO) {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }
            
            if (output_fd != STDOUT_FILENO) {
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);
            }
            
            execvp(args[0], args);
            printf("Hata: '%s' komutu bulunamadı\n", args[0]);
            exit(1);
        } else if (pid > 0) {
            // Ana süreç
            if (input_fd != STDIN_FILENO) close(input_fd);
            if (output_fd != STDOUT_FILENO) close(output_fd);
            
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
}

// Mesaj gönderme fonksiyonu
void module_send_message(const char *msg) {
    // Paylaşılan belleği başlat
    init_shared_memory();
    
    if (shmp == NULL) {
        fprintf(stderr, "Paylaşılan bellek başlatılamadı.\n");
        return;
    }
    
    // Zaman damgası oluştur
    time_t now = time(NULL);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", localtime(&now));
    
    // Get the current tab number
    int current_tab = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)) + 1;
    
    // Mesajı formatlama (tab bilgisini ekleyerek)
    char formatted_msg[BUF_SIZE];
    snprintf(formatted_msg, BUF_SIZE, "[%s] %d.sekme: %s", timestamp, current_tab, msg);
    
    // Semafor ile senkronize et
    sem_wait(&shmp->sem);
    
    // Mesajı paylaşılan belleğe yaz
    strncpy(shmp->msgbuf, formatted_msg, sizeof(shmp->msgbuf) - 1);
    shmp->msgbuf[sizeof(shmp->msgbuf) - 1] = '\0';
    shmp->cnt = strlen(shmp->msgbuf);
    
    // Global mesaj geçmişini güncelle
    if (!global_message_history) {
        global_message_history = g_string_new("");
    }
    g_string_append_printf(global_message_history, "%s\n", formatted_msg);
    
    sem_post(&shmp->sem);
    
     // Broadcast message to all tabs
    int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook));
    for (int i = 0; i < n_pages; i++) {
        GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), i);
        if (page) {
            TerminalTab *tab = g_object_get_data(G_OBJECT(page), "tab-data");
            if (tab) {
                // Eğer message_history henüz oluşturulmadıysa oluştur
                if (!tab->message_history) {
                    tab->message_history = g_string_new("");
                }
                
                // Mesajı geçmişe ekle
                g_string_append_printf(tab->message_history, "%s\n", formatted_msg);
            }
        }
    }
    
    // Mesaj gönderildiğini ekrana yazdır
    printf("Mesaj gönderildi: %s\n", msg);
}



// Mesaj geçmişini almak için yeni bir fonksiyon ekleyin
const char* module_get_message_history() {
    if (global_message_history == NULL) {
        return "";
    }
    return global_message_history->str;
}


// Mesajları okuma fonksiyonu
void module_read_messages(char *buffer) {
    // Paylaşılan belleği başlat
    init_shared_memory();
    
    if (shmp == NULL) {
        strcpy(buffer, "Henüz mesaj yok.");
        return;
    }
    
    // Semafor ile senkronize et
    sem_wait(&shmp->sem);
    
    // Son mesajı al
    if (shmp->cnt > 0) {
        strncpy(buffer, shmp->msgbuf, BUF_SIZE - 1);
        buffer[BUF_SIZE - 1] = '\0';
    } else {
        strcpy(buffer, "Henüz mesaj yok.");
    }
    
    sem_post(&shmp->sem);
}   


//ekleme yaptim burda
int handle_redirections(char *args[], int *input_fd, int *output_fd) {
    int i, j;
    
    // Varsayılan değerler
    *input_fd = STDIN_FILENO;
    *output_fd = STDOUT_FILENO;
    
    // Yönlendirmeleri ara
    for (i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            // Girdi yönlendirme
            if (args[i+1] == NULL) {
                fprintf(stderr, "Hata: < işaretinden sonra dosya adı eksik\n");
                return -1;
            }
            
            int fd = open(args[i+1], O_RDONLY);
            if (fd == -1) {
                perror("open");
                return -1;
            }
            
            *input_fd = fd;
            
            // Argüman listesinden yönlendirmeyi kaldır
            for (j = i; args[j] != NULL; j++) {
                args[j] = args[j+2];
            }
            
            i--; // Dizindeki değişikliği hesaba kat
        }
        else if (strcmp(args[i], ">") == 0) {
            // Çıktı yönlendirme (üzerine yaz)
            if (args[i+1] == NULL) {
                fprintf(stderr, "Hata: > işaretinden sonra dosya adı eksik\n");
                return -1;
            }
            
            int fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                perror("open");
                return -1;
            }
            
            *output_fd = fd;
            
            // Argüman listesinden yönlendirmeyi kaldır
            for (j = i; args[j] != NULL; j++) {
                args[j] = args[j+2];
            }
            
            i--; // Dizindeki değişikliği hesaba kat
        }
        else if (strcmp(args[i], ">>") == 0) {
            // Çıktı yönlendirme (sona ekle)
            if (args[i+1] == NULL) {
                fprintf(stderr, "Hata: >> işaretinden sonra dosya adı eksik\n");
                return -1;
            }
            
            int fd = open(args[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd == -1) {
                perror("open");
                return -1;
            }
            
            *output_fd = fd;
            
            // Argüman listesinden yönlendirmeyi kaldır
            for (j = i; args[j] != NULL; j++) {
                args[j] = args[j+2];
            }
            
            i--; // Dizindeki değişikliği hesaba kat
        }
    }
    
    return 0;
}

