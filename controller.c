#include "controller.h"

// Stdout redirection variables
int stdout_pipe[2];
GIOChannel *channel_out;

// Controller functions
void on_command_entered(GtkWidget *entry, gpointer user_data) {
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    
    if (strlen(text) > 0) {
        // Echo command to terminal
        gchar *command_echo = g_strdup_printf("myshell> %s\n", text);
        append_to_terminal(command_echo);
        g_free(command_echo);
        
        // Execute the command (from original code)
        char command_copy[MAX_CMD_LEN];
        strncpy(command_copy, text, MAX_CMD_LEN-1);
        command_copy[MAX_CMD_LEN-1] = '\0';
        
        execute_command(command_copy);
        
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
