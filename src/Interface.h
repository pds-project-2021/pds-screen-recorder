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
    bool recordered = false;
    std::string dest;

    Interface(GtkApplication *app);
    ~Interface(){
        g_print("Interface has been destroyed\n");

        // terminate capture if it's running
        if (s->is_capturing()){
            s->terminate();
        }

        if(!recordered){
            gtk_window_destroy(GTK_WINDOW(recordWindow));
            gtk_window_destroy(GTK_WINDOW(selectWindow));
        }
        gtk_window_destroy(GTK_WINDOW(fileChooser));
//        g_object_unref(window);
//        g_object_unref(selectWindow);
//        g_object_unref(recordWindow);
//        g_object_unref(recordButton);
//        g_object_unref(startRecordButton);
//        g_object_unref(pauseButton);
//        g_object_unref(stopButton);
//        g_object_unref(headerBar);
//        g_object_unref(image);
//        g_object_unref(title);
//        g_object_unref(leftGesture);
//        g_object_unref(rightGesture);
//        g_object_unref(motionController);
//        g_object_unref(selectionArea);
//        g_object_unref(cssProvider);
//        g_object_unref(context);
//        g_object_unref(titleView);
//        g_object_unref(fileChoiceDialog);
//        g_object_unref(fileChooser);
    };
    cairo_surface_t *surface = nullptr;
    void getRectCoordinates(double&, double&, double&, double&) const;
};


int launchUI(int argc, char **argv);

static void handleRecord(GtkWidget *widget, gpointer data);

static int handlePause(GtkWidget *widget, gpointer data);

static void handleStop(GtkWidget *widget, gpointer data);

static void handleClose(GtkWidget *widget, gpointer data);

static void activate(GtkApplication *app, gpointer user_data);