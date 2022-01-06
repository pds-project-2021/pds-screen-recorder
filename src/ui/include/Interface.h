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

int launchUI(int argc, char **argv, Recorder *rec);

static void record(GtkWidget *widget, gpointer data);

static int handlePause(GtkWidget *widget, gpointer data);

static void handleStop(GtkWidget *widget, gpointer data);

static void handleClose();

static void activate(GtkApplication *app, gpointer user_data);
