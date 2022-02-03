#pragma once

#include <gtk/gtk.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <future>
#include <memory>

#include "lib/Recorder.h"

//using namespace std;

class Interface {
public:
    GtkWidget *window;
    GtkWidget *selectWindow;
    GtkWidget *recordWindow;
    GtkWidget *recordButton;
    GtkWidget *startRecordButton;
    GtkWidget *pauseButton;
    GtkWidget *stopButton;
    GtkWidget *headerBar;
    std::atomic<bool> selection_enabled;
    GtkWidget *image;
    GtkTextBuffer *title;
    GtkGesture *leftGesture;
    GtkGesture *rightGesture;
    GtkEventController *motionController;
    GtkWidget *selectionArea;
    GtkCssProvider *cssProvider;
    GtkStyleContext *context;
    GtkWidget *titleView;
    GtkWidget *fileChoiceDialog;
    GtkFileChooser *fileChooser;
    double startX = 0;
    double startY = 0;
    double endX = 0;
    double endY = 0;
    std::atomic<bool> ready;
    std::atomic<bool> started;
    std::unique_ptr<Recorder> s = nullptr;
    unsigned long fileHandler;
    std::string dest;
    cairo_surface_t *surface = nullptr;

    Interface(GtkApplication *app);
    ~Interface(){
        g_print("Interface has been destroyed\n");
        // terminate capture if it's running
        if (s->is_capturing()){
            s->terminate();
        }
#ifdef linux
        gtk_window_destroy(GTK_WINDOW(window));
//        gtk_window_set_hide_on_close(GTK_WINDOW(selectWindow), false);
//        gtk_window_close(GTK_WINDOW(selectWindow));
//        gtk_window_set_hide_on_close(GTK_WINDOW(recordWindow), false);
//        gtk_window_close(GTK_WINDOW(recordWindow));
//        gtk_window_set_hide_on_close(GTK_WINDOW(fileChoiceDialog), false);
//        gtk_window_close(GTK_WINDOW(fileChoiceDialog));
#else
        gtk_window_set_hide_on_close(GTK_WINDOW(window), false);
        gtk_window_close(GTK_WINDOW(window));
#endif
    };
    void getRectCoordinates(double&, double&, double&, double&) const;
};


int launchUI(int argc, char **argv);

static void handleRecord(GtkWidget *widget, gpointer data);

static int handlePause(GtkWidget *widget, gpointer data);

static void handleStop(GtkWidget *widget, gpointer data);

static void handleClose(GtkWidget *widget, gpointer data);

static void activate(GtkApplication *app, gpointer user_data);