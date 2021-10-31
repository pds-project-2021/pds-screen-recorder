//
// Created by gabriele on 31/10/21.
//

#pragma once

#include <gtk/gtk.h>

//class Interface {
//
//};

GtkWidget *window;

static void record(GtkWidget *widget, gpointer data) ;

static int _pause(GtkWidget *widget, gpointer data) ;

static void stop(GtkWidget *widget, gpointer data);

static void close(GtkWidget *widget, gpointer data) ;

static void activate(GtkApplication *app, gpointer user_data) ;

int gtk_test(int argc, char **argv) ;


