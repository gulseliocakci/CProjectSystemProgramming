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
void append_to_terminal(const gchar *text);
gboolean update_shared_messages(gpointer user_data);

#endif // VIEW_H
