#ifndef VIEW_H
#define VIEW_H

#include <gtk/gtk.h>
#include "model.h"

// GUI elements
extern GtkWidget *window;
extern GtkWidget *terminal_view;
extern GtkWidget *command_entry;
extern GtkWidget *shared_message_view;
extern GtkWidget *notebook;
extern GtkTextBuffer *terminal_buffer;
extern GtkTextBuffer *shared_buffer;
extern GtkTextMark *terminal_end_mark;

// View function declarations
void create_gui();
void add_terminal_tab(GtkWidget *button, gpointer user_data);
void append_to_terminal(const gchar *text, gpointer user_data);
gboolean update_shared_messages(gpointer user_data);

void on_tab_switched(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data);

typedef struct {
    int stdin_pipe[2];
    int stdout_pipe[2];
    int stderr_pipe[2];
    pid_t shell_pid;
    GIOChannel *stdout_channel;
    GIOChannel *stderr_channel;
    GtkWidget *term_view;
    GtkTextBuffer *term_buffer;
    GtkTextMark *term_mark;

    GString *message_history;  // yeni satır: geçmiş mesajları saklar
    // diğer alanlar...
} TerminalTab;

#endif // VIEW_H
