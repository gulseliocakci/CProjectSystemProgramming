#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <gtk/gtk.h>
#include "model.h"
#include "view.h"

// Controller function declarations
void on_command_entered(GtkWidget *entry, gpointer user_data);
gboolean on_window_delete(GtkWidget *widget, GdkEvent *event, gpointer data);
gboolean read_stdout_callback(GIOChannel *channel, GIOCondition condition, gpointer data);
void setup_stdout_redirection();

// External declarations for stdout redirection
extern int stdout_pipe[2];
extern GIOChannel *channel_out;

#endif // CONTROLLER_H
