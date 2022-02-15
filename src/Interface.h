#pragma once

#include <gtk/gtk.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <future>
#include <memory>

#include "lib/Recorder.h"
#include "lib/include/exceptions.h"
#include "libavutil/avutil.h"

#include "icon_on.xpm"
#include "icon_off.xpm"

const GOptionEntry cmd_params[] = {
	{
		.long_name = "info",
		.short_name = 'v',
		.flags = G_OPTION_FLAG_NONE,
		.arg = G_OPTION_ARG_NONE,
		.arg_data = nullptr,
		.description = "For INFO level logging",
		.arg_description = nullptr,
	},
	{
		.long_name = "debug",
		.short_name = 'w',
		.flags = G_OPTION_FLAG_NONE,
		.arg = G_OPTION_ARG_NONE,
		.arg_data = nullptr,
		.description = "For DEBUG level logging",
		.arg_description = nullptr,
	},
	{nullptr}
};


class Interface {
  public:
	GtkApplication *g_application = nullptr;

	GtkWidget *window = nullptr;
	GtkWidget *selectWindow = nullptr;
	GtkWidget *recordWindow = nullptr;
	GtkWidget *recordButton = nullptr;
	GtkWidget *startRecordButton = nullptr;
	GtkWidget *pauseButton = nullptr;
	GtkWidget *stopButton = nullptr;
	GtkWidget *headerBar = nullptr;
	GtkWidget *selectionArea = nullptr;
	GtkWidget *titleView = nullptr;
	GtkWidget *fileChoiceDialog = nullptr;
	GtkWidget *image = nullptr;
    GtkWidget *dialog = nullptr;
    GtkWidget *label = nullptr;

	std::atomic<bool> selection_enabled = false;
	std::atomic<bool> ready = false;
    std::atomic<bool> blink_enabled = false;
	std::atomic<bool> img_on = true;

	GtkTextBuffer *title = nullptr;
	GtkGesture *leftGesture = nullptr;
	GtkGesture *rightGesture = nullptr;
	GtkEventController *motionController = nullptr;
	GtkCssProvider *cssProvider = nullptr;
	GtkStyleContext *context = nullptr;
	GtkFileChooser *fileChooser = nullptr;

	double startX = 0;
	double startY = 0;
	double endX = 0;
	double endY = 0;

	std::unique_ptr<Recorder> s = nullptr;
	unsigned long fileHandler;
	std::string dest;
	cairo_surface_t *surface = nullptr;
	std::future<gboolean> blink_img;
	std::future<void> rec;

	Interface() = default;
	explicit Interface(GtkApplication *app);
	~Interface();

	// utility functions
	void getRectCoordinates(double &, double &, double &, double &) const;
	static void init_recorder(double sX, double sY, double eX, double eY);
	static void startRecording();
	static void pauseRecording();
	static void stopRecording();
	static gboolean switchImageRec();
	void setImageRecOff();
	void setImageRecOn();
    void init_error_dialog();
    void set_error_dialog_msg(const char*) const;
    void reset_gui_from_start();
    void reset_gui_from_exec();
    void reset_gui_from_stop();
    void enable_blink();

	// callback
    static gboolean on_dialog_deleted();
    static gboolean on_widget_deleted();
	static void on_save_response(GtkDialog *dialog, int response);
	static void motion_detected(GtkEventControllerMotion *controller, double x, double y, gpointer user_data);
	static void select_record_region(GtkWidget *widget, gpointer data);
	static void draw_rect(cairo_t *cr);
	static void draw(GtkDrawingArea *drawing_area, cairo_t *cr, int width, int height, gpointer data);
	static void activate(GtkApplication *app, gpointer user_data);

	// button functions
	static void handleRecord(GtkWidget *widget, gpointer data);
	static void handlePause(GtkWidget *widget, gpointer data);
	static void handleStop(GtkWidget *widget, gpointer data);
	static void handleClose(GtkWidget *widget, gpointer data);

	static void right_btn_pressed(GtkGestureClick *gesture, int n_press, double x, double y, GtkWidget *widget);
	static void left_btn_pressed(GtkGestureClick *gesture, int n_press, double x, double y, GtkWidget *widget);
	static void right_btn_released(GtkGestureClick *gesture, int n_press, double x, double y, GtkWidget *widget);
	static void left_btn_released(GtkGestureClick *gesture, int n_press, double x, double y, GtkWidget *widget);
};

int launchUI(int argc, char **argv);

