#include <gtk/gtk.h>
#include <iostream>

static void
print_hello(GtkWidget* widget,
    gpointer   data)
{
    g_print("Hello World\n");
}

static void
activate(GtkApplication* app,
    gpointer        user_data)
{
    GtkWidget* window;
    GtkWidget* buttonGrid;
    GtkWidget* recordButton;
    GtkWidget* pauseButton;
    GtkWidget* stopButton;

    window = gtk_application_window_new(app);
    buttonGrid = gtk_grid_new();
    gtk_window_set_title(GTK_WINDOW(window), "Screen recorder");
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);

    recordButton = gtk_button_new_with_label("Record");
    pauseButton = gtk_button_new_with_label("Pause");
    stopButton = gtk_button_new_with_label("Stop");
    g_signal_connect(recordButton, "clicked", G_CALLBACK(print_hello), NULL);
    gtk_window_set_child(GTK_WINDOW(window), buttonGrid);
    gtk_grid_attach(GTK_GRID(buttonGrid), recordButton, 0, 0, 100, 50);
    //gtk_grid_insert_next_to(GTK_GRID(buttonGrid), recordButton, GTK_POS_RIGHT);
    gtk_grid_attach_next_to(GTK_GRID(buttonGrid), pauseButton, recordButton, GTK_POS_RIGHT, 100, 50);
    gtk_grid_attach_next_to(GTK_GRID(buttonGrid), stopButton, pauseButton, GTK_POS_RIGHT, 100, 50);
    std::cout << gtk_grid_get_baseline_row << "\n";
    gtk_window_present(GTK_WINDOW(window));
}

int
main(int    argc,
    char** argv)
{
    GtkApplication* app;
    int status;

    app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
