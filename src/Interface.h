//
// Created by gabriele on 31/10/21.
//

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
    GtkGesture *moveGesture;
    GtkEventController *motionController;
    GtkWidget *selectionArea;
    GtkCssProvider *cssProvider;
    GtkStyleContext *context;
    cairo_surface_t *background;
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

    Interface();
    cairo_surface_t *surface = nullptr;
    int launchUI(int argc, char **argv);
    void getRectCoordinates(double&, double&, double&, double&);
};



static void handleRecord(GtkWidget *widget, gpointer data);

static int handlePause(GtkWidget *widget, gpointer data);

static void handleStop(GtkWidget *widget, gpointer data);

static void handleClose(GtkWidget *widget, gpointer data);

static void activate(GtkApplication *app, gpointer user_data);