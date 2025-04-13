#include "model.h"
#include "view.h"
#include "controller.h"

int main(int argc, char *argv[]) {
    // Initialize GTK
    gtk_init(&argc, &argv);
    
    // Set up stdout redirection
    setup_stdout_redirection();
    
    // Create GUI
    create_gui();
    
    // Module başlatma mesajı
    module_send_message("\n.......GTK Shell başlatıldı........");
    
    printf("GTK Shell çalışıyor. Komutları aşağıya yazabilirsiniz.\n");
    
    // Run main loop - bu satır kritik, programın açık kalmasını sağlar
    gtk_main();
    
    printf("GTK Shell kapatılıyor...\n");
    
    return 0;
}
