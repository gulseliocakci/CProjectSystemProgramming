#include "view.h"
#include "controller.h"

// GUI widgets
GtkWidget *window;
GtkWidget *terminal_view;
GtkWidget *command_entry;
GtkWidget *shared_message_view;
GtkWidget *notebook;
GtkTextBuffer *terminal_buffer;
GtkTextBuffer *shared_buffer;
GtkTextMark *terminal_end_mark;

// GUI creation and interaction functions
void create_gui() {
    // Create main window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "GTK Shell");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "delete-event", G_CALLBACK(on_window_delete), NULL);
    
    // Create main layout (horizontal pane)
    GtkWidget *hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_container_add(GTK_CONTAINER(window), hpaned);
    
    // Create notebook for terminal tabs
    notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
    
    // Create "+" button for adding new tabs
    GtkWidget *add_button = gtk_button_new_with_label("+");
    gtk_widget_set_size_request(add_button, 20, 20);
    g_signal_connect(add_button, "clicked", G_CALLBACK(add_terminal_tab), NULL);
    
    // Add the button to notebook
    gtk_notebook_set_action_widget(GTK_NOTEBOOK(notebook), add_button, GTK_PACK_END);
    gtk_widget_show(add_button);
    
    // Create shared message panel
    GtkWidget *shared_frame = gtk_frame_new("Shared Messages");
    GtkWidget *shared_scroll = gtk_scrolled_window_new(NULL, NULL);
    shared_message_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(shared_message_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(shared_message_view), GTK_WRAP_WORD_CHAR);
    
    // Set up shared message buffer
    shared_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(shared_message_view));
    
    // Pack shared message widgets
    gtk_container_add(GTK_CONTAINER(shared_scroll), shared_message_view);
    gtk_container_add(GTK_CONTAINER(shared_frame), shared_scroll);
    
    // Add widgets to paned
    gtk_paned_add1(GTK_PANED(hpaned), notebook);
    gtk_paned_add2(GTK_PANED(hpaned), shared_frame);
    gtk_paned_set_position(GTK_PANED(hpaned), 600);
    
    // Create first terminal tab
    add_terminal_tab(NULL, NULL);
    
    // Get the widgets from the first tab
    GtkWidget *vbox = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), 0);
    GList *children = gtk_container_get_children(GTK_CONTAINER(vbox));
    GtkWidget *scroll = GTK_WIDGET(children->data);
    command_entry = GTK_WIDGET(children->next->data);
    g_list_free(children);
    
    children = gtk_container_get_children(GTK_CONTAINER(scroll));
    terminal_view = GTK_WIDGET(children->data);
    g_list_free(children);
    
    // Set up terminal buffer and mark for auto-scroll
    terminal_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(terminal_view));
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(terminal_buffer, &iter);
    terminal_end_mark = gtk_text_buffer_create_mark(terminal_buffer, "end", &iter, FALSE);
    
    // Set initial welcome message
    gtk_text_buffer_set_text(terminal_buffer, "GTK Shell - Terminal Emulator\n\n", -1);
    
    // Set up periodic update of shared messages
    g_timeout_add(500, update_shared_messages, NULL);
    
    // Show all widgets
    gtk_widget_show_all(window);
}

void add_terminal_tab(GtkWidget *button, gpointer user_data) {
    // Create a new tab with terminal view
    GtkWidget *tab_label = gtk_label_new("Shell");
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *term = gtk_text_view_new();
    GtkWidget *entry = gtk_entry_new();
    
    gtk_text_view_set_editable(GTK_TEXT_VIEW(term), FALSE);
    gtk_widget_set_can_focus(term, FALSE);
    
    // Set monospace font using CSS (modern approach)
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "textview { font-family: monospace; font-size: 10pt; }", -1, NULL);
    GtkStyleContext *context = gtk_widget_get_style_context(term);
    gtk_style_context_add_provider(context,
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    
    // Pack widgets
    gtk_container_add(GTK_CONTAINER(scroll), term);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);
    
    // Add to notebook
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, tab_label);
    
    // Connect signals
    g_signal_connect(entry, "activate", G_CALLBACK(on_command_entered), NULL);
    
    // Show all widgets
    gtk_widget_show_all(vbox);
    
    // Switch to the new tab
    gint page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) - 1;
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page_num);
    
    // Focus on entry
    gtk_widget_grab_focus(entry);
}

void append_to_terminal(const gchar *text) {
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(terminal_buffer, &iter);
    gtk_text_buffer_insert(terminal_buffer, &iter, text, -1);
    
    // Scroll to end
    gtk_text_buffer_get_end_iter(terminal_buffer, &iter);
    gtk_text_buffer_move_mark(terminal_buffer, terminal_end_mark, &iter);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(terminal_view), terminal_end_mark, 0.0, TRUE, 0.0, 1.0);
}

gboolean update_shared_messages(gpointer user_data) {
    char buffer[BUF_SIZE];
    
    // Read messages from shared memory
    module_read_messages(buffer);
    
    // Update shared message view
    gtk_text_buffer_set_text(shared_buffer, buffer, -1);
    
    return G_SOURCE_CONTINUE;  // Continue the timeout
}
