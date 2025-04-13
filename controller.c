#include "controller.h"

// Stdout redirection variables
int stdout_pipe[2];
GIOChannel *channel_out;

// Controller functions

void on_command_entered(GtkWidget *entry, gpointer user_data) {
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    TerminalTab *tab = (TerminalTab *)user_data;
    
    if (strlen(text) > 0) {
        // Echo command to terminal
        gchar *command_echo = g_strdup_printf("my_shell>> %s\n", text);
        append_to_terminal(command_echo, user_data);
        g_free(command_echo);
        
        // Check if it's a message command
        if (g_str_has_prefix(text, "@msg ")) {
            // Handle as message
            module_send_message(text + 5);  // Skip "@msg "
            
            // Confirm message sent
            gchar *msg_confirm = g_strdup_printf("Mesaj gönderildi: %s\n", text + 5);
            append_to_terminal(msg_confirm, user_data);
            g_free(msg_confirm);
        } else {

            // Komutu çocuk sürece gönder
            gchar *cmd = g_strdup_printf("%s\n", text);
            write(tab->stdin_pipe[1], cmd, strlen(cmd));
            g_free(cmd);
        }
        
        // Clear entry
        gtk_entry_set_text(GTK_ENTRY(entry), "");
    }
}

gboolean on_window_delete(GtkWidget *widget, GdkEvent *event, gpointer data) {
    gtk_main_quit();
    return FALSE; // Allow the window to be destroyed
}

gboolean read_stdout_callback(GIOChannel *channel, GIOCondition condition, gpointer data) {
    GString *str = g_string_new(NULL);
    gsize bytes_read;
    GIOStatus status;
    
    status = g_io_channel_read_line_string(channel, str, &bytes_read, NULL);
    
    if (status == G_IO_STATUS_NORMAL) {
        // Append to terminal view
        gtk_text_buffer_insert_at_cursor(terminal_buffer, str->str, -1);
        
        // Scroll to end
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(terminal_buffer, &iter);
        gtk_text_buffer_move_mark(terminal_buffer, terminal_end_mark, &iter);
        gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(terminal_view), terminal_end_mark, 0.0, TRUE, 0.0, 1.0);
    }
    
    g_string_free(str, TRUE);
    return TRUE; // Keep the source
}

void setup_stdout_redirection() {
    if (pipe(stdout_pipe) != 0) {
        g_error("Failed to create pipe");
        return;
    }
    
    // Redirect stdout to our pipe
    dup2(stdout_pipe[1], STDOUT_FILENO);
    close(stdout_pipe[1]);
    
    // Set up GIOChannel for the pipe
    channel_out = g_io_channel_unix_new(stdout_pipe[0]);
    g_io_channel_set_flags(channel_out, G_IO_FLAG_NONBLOCK, NULL);
    g_io_channel_set_encoding(channel_out, NULL, NULL);
    g_io_channel_set_buffer_size(channel_out, 8192);
    
    // Add watch to read from pipe
    g_io_add_watch(channel_out, G_IO_IN | G_IO_HUP, read_stdout_callback, NULL);
}




//ekleme: controller.c dosyasına eklenecek
gboolean read_terminal_output(GIOChannel *channel, GIOCondition condition, gpointer data) {
    TerminalTab *tab = (TerminalTab *)data;
    GString *str = g_string_new(NULL);
    gsize bytes_read;
    GIOStatus status;
    
    status = g_io_channel_read_line_string(channel, str, &bytes_read, NULL);
    
    if (status == G_IO_STATUS_NORMAL && bytes_read > 0) {
        // Terminal buffer'a metin ekle
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(tab->term_buffer, &iter);
        gtk_text_buffer_insert(tab->term_buffer, &iter, str->str, -1);
        
        // Sona kaydır
        gtk_text_buffer_get_end_iter(tab->term_buffer, &iter);
        gtk_text_buffer_move_mark(tab->term_buffer, tab->term_mark, &iter);
        gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(tab->term_view), 
                                   tab->term_mark, 0.0, TRUE, 0.0, 1.0);
    }
    
    if (condition & G_IO_HUP) {
        // Kanal kapandı
        g_string_free(str, TRUE);
        return FALSE; // İzleyiciyi kaldır
    }
    
    g_string_free(str, TRUE);
    return TRUE; // İzleyiciyi tut
}

