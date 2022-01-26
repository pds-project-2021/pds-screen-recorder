//
// Created by gabriele on 31/10/21.
//

#pragma once

#include <gtk/gtk.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <future>

#include "../../Recorder.h"

using namespace std;

class Interface {
private:
    GtkWidget *window;
    GtkWidget *selectWindow;
    GtkWidget *recordWindow;
    GtkWidget *recordButton;
    GtkWidget *startRecordButton;
    GtkWidget *pauseButton;
    GtkWidget *stopButton;
    GtkWidget *headerBar;
    atomic<bool> selection_enabled;
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
    double startX = 0;
    double startY = 0;
    double endX = 0;
    double endY = 0;
    atomic<bool> ready;
    atomic<bool> started;
    unique_ptr<Recorder> s = nullptr;
    bool recordered = false;

//    void clear_surface(void);
//    void draw_rect(cairo_t *cr);
//    void draw(GtkDrawingArea *drawing_area, cairo_t *cr, int width, int height, gpointer data);
//    void motion_detected(GtkEventControllerMotion *controller, double x, double y, gpointer user_data);
//    void right_btn_pressed(GtkGestureClick *gesture, int n_press, double x, double y, GtkWidget *widget);
//    void right_btn_released(GtkGestureClick *gesture, int n_press, double x, double y, GtkWidget *widget);
//    void left_btn_pressed(GtkGestureClick *gesture, int n_press, double x, double y, GtkWidget *widget);
//    void left_btn_released(GtkGestureClick *gesture, int n_press, double x, double y, GtkWidget *widget);
//    void drag(GtkGestureDrag *gesture, double offset_x, double offset_y, gpointer user_data);
//    void recorder(int sX, int sY, int eX, int eY);
//    void startRecording();
//    void pauseRecording();
//    void stopRecording();
//    void select_record_region(GtkWidget *widget, gpointer data);
//    void handleRecord(GtkWidget *widget, gpointer data);
//    int handlePause(GtkWidget *widget, gpointer data);
//    void handleStop(GtkWidget *widget, gpointer data);
//    void handleClose(GtkWidget *widget, gpointer data);
//    void activate(GtkApplication *app, gpointer user_data);
public:
    cairo_surface_t *surface = nullptr;
    int launchUI(int argc, char **argv);
    double getRectCoordinates(double&, double&, double&, double&);
};



static void handleRecord(GtkWidget *widget, gpointer data);

static int handlePause(GtkWidget *widget, gpointer data);

static void handleStop(GtkWidget *widget, gpointer data);

static void handleClose(GtkWidget *widget, gpointer data);

static void activate(GtkApplication *app, gpointer user_data);