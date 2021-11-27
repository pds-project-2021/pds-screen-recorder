#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <iostream>

#include "ScreenRecorder.h"

GtkWidget *window;
GtkWidget *recordButton;
GtkWidget *pauseButton;
GtkWidget *stopButton;
GtkWidget *headerBar;
GtkWidget *image;
GtkTextBuffer *title;
GtkGesture *leftGesture;
GtkWidget *titleView;
double *startX;
double *startY;
ScreenRecorder *s;
bool ready = false;
bool started = false;

void init_output() {
    s->init_outputfile();
    ready = true;
    std::cout << "Initialized output streams and file" << std::endl;
}

//Mouse click event handler function

static void
left_btn_pressed (GtkGestureClick *gesture,
                                   int                n_press,
                                   double             x,
                                   double             y,
                                   GtkWidget         *widget)
{
    g_print ("Left button pressed\n");
}

static void
left_btn_released (GtkGestureClick *gesture,
                                    int              n_press,
                                    double           x,
                                    double           y,
                                    GtkWidget       *widget)
{
    g_print ("Left button released\n");

    gtk_gesture_set_state (GTK_GESTURE (gesture),
                           GTK_EVENT_SEQUENCE_CLAIMED);
}


gboolean deal_mouse_press (GtkWidget * widget, GdkEvent * event, gpointer data)
{
    auto type = gdk_event_get_event_type(event);
    if (type == GDK_BUTTON_PRESS) {
        std::cout << "Click: ";
        switch (gdk_button_event_get_button(event)) {//Determine the type of mouse click
            case 1:
                std::cout << "Left Button" << std::endl;
                break;
            case 2:
                std::cout << "Middle Button" << std::endl;
                break;
            case 3:
                std::cout << "Right Button" << std::endl;
                break;
            default:
                std::cout << "Unknown Button" << std::endl;
        }
    }
    //Get the coordinate value of the click, from the left vertex of the window
    gdk_event_get_axis(event, GDK_AXIS_X, startX);
    gdk_event_get_axis(event, GDK_AXIS_Y, startY);
    std::cout << "press_x = " << startX << ", press_y = " << startY << std::endl;

    return TRUE;
}

//The processing function of the mouse movement event (click any button of the mouse)
gboolean deal_motion_notify_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
    //Get the coordinate value of the moving mouse, from the left vertex of the window
    double *movX;
    double *movY;
    gdk_event_get_axis(event, GDK_AXIS_X, movX);
    gdk_event_get_axis(event, GDK_AXIS_Y, movY);
    std::cout << "mov_x = " << movX << ", mov_y = " << movY << std::endl;

    return TRUE;
}

void recorder() {
    s->init();
    std::cout << "Initialized input streams" << std::endl;
    init_output();
    started = false;
//    if (s.CaptureStart() >= 0) {
//        int millisecondsPause = 2000;
//        int millisecondsResume = 2000;
//        int millisecondsStop = 7000;
//        std::cout << "Capture started" << std::endl;
//        std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsPause));
//        s.PauseCapture();
//        std::cout << "Capture paused after " << ((double)millisecondsPause/1000) << " seconds" << std::endl;
//        std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsResume));
//        s.ResumeCapture();
//        std::cout << "Capture resumed after " << ((double)millisecondsResume/1000) << " seconds \n" << std::endl;
//        std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsStop));
//        s.CloseMediaFile();
//        std::cout << "Capture complete after " << ((double)millisecondsStop/1000) << " seconds \n" << std::endl;
//    }
}

void startRecording() {
    if(!ready) recorder();
    if(s->isPaused()) s->ResumeCapture();
    else {
        if(!started) {
            s->CaptureStart();
            started = true;
        }
    }
}

void pauseRecording() {
    if(!ready) return;
    if(s->isPaused()) s->ResumeCapture();
    else s->PauseCapture();
}

void stopRecording() {
    if(!ready) return;
    s->close();
    ready = false;
    started = false;
}

static void record(GtkWidget *widget, gpointer data) {
    std::future<void> foo = std::async(std::launch::async, startRecording);
	g_print("Record button pressed\n");
}

static int _pause(GtkWidget *widget, gpointer data) {
    std::future<void> foo = std::async(std::launch::async, pauseRecording);
	g_print("Pause button pressed\n");
	return 0;
}

static void stop(GtkWidget *widget, gpointer data) {
    std::future<void> foo = std::async(std::launch::async, stopRecording);
	g_print("Stop button pressed\n");
}

static void close(GtkWidget *widget, gpointer data) {
	gtk_window_close(GTK_WINDOW(window));
}

static void activate(GtkApplication *app, gpointer user_data) {
	// GtkWidget* buttonGrid;
	// GtkWidget* closeButton;

	window = gtk_application_window_new(app);
	// buttonGrid = gtk_grid_new();
	headerBar = gtk_header_bar_new();
	image = gtk_image_new_from_file("../assets/icon_small.png");
	title = gtk_text_buffer_new(NULL);
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(title), "  ", 2);
	titleView = gtk_text_view_new_with_buffer(title);
	gtk_window_set_title(GTK_WINDOW(window), "Screen recorder");
	gtk_window_set_default_size(GTK_WINDOW(window), 463, 50);
	gtk_window_set_decorated(GTK_WINDOW(window), false);
	gtk_window_set_resizable(GTK_WINDOW(window), false);
	recordButton = gtk_button_new_with_label("Record");
	pauseButton = gtk_button_new_with_label("Pause");
	stopButton = gtk_button_new_with_label("Stop");
	// closeButton = gtk_button_new_with_label("Close");
	auto rec = g_signal_connect(recordButton, "clicked", G_CALLBACK(record), nullptr);
	auto p = g_signal_connect(pauseButton, "clicked", G_CALLBACK(_pause), nullptr);
	auto s = g_signal_connect(stopButton, "clicked", G_CALLBACK(stop), nullptr);
	// g_signal_connect(closeButton, "clicked", G_CALLBACK(close), NULL);
	// gtk_window_set_child(GTK_WINDOW(window), buttonGrid);
	gtk_window_set_child(GTK_WINDOW(window), headerBar);
	gtk_image_set_pixel_size(GTK_IMAGE(image), 32);
	gtk_header_bar_set_decoration_layout(GTK_HEADER_BAR(headerBar),
	                                     ":minimize,close");
	// gtk_header_bar_set_title_widget(GTK_HEADER_BAR(headerBar), titleView);
	gtk_header_bar_pack_start(GTK_HEADER_BAR(headerBar), image);
	// gtk_header_bar_pack_start(GTK_HEADER_BAR(headerBar), titleView);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(headerBar), stopButton);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(headerBar), pauseButton);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(headerBar), recordButton);
    leftGesture = gtk_gesture_click_new();
    gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (leftGesture), 1);
    g_signal_connect (leftGesture, "pressed",
                      G_CALLBACK (left_btn_pressed), window);
    g_signal_connect (leftGesture, "released",
                      G_CALLBACK (left_btn_released), window);
    gtk_widget_add_controller (window, GTK_EVENT_CONTROLLER (leftGesture));
	// gtk_header_bar_pack_start(GTK_HEADER_BAR(headerBar), closeButton);
	// gtk_grid_attach(GTK_GRID(buttonGrid), recordButton, 0, 0, 100, 50);
	// gtk_grid_insert_next_to(GTK_GRID(buttonGrid), recordButton, GTK_POS_RIGHT);
	// gtk_grid_attach_next_to(GTK_GRID(buttonGrid), pauseButton, recordButton,
	// GTK_POS_RIGHT, 100, 50); gtk_grid_attach_next_to(GTK_GRID(buttonGrid),
	// stopButton, pauseButton, GTK_POS_RIGHT, 100, 50);
	// gtk_grid_attach_next_to(GTK_GRID(buttonGrid), closeButton, stopButton,
	// GTK_POS_RIGHT, 100, 50);
	gtk_window_present(GTK_WINDOW(window));
}

int gtk_test(int argc, char **argv) {
	GtkApplication *app;
	int status;

	app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}

template<typename T>
bool future_is_ready(std::future<T>& t){
    return t.wait_for(std::chrono::seconds(1)) == std::future_status::ready;
}

void pauseTest() {
    int secondsPause = 2;
    int secondsResume = 1;
    std::this_thread::sleep_for(std::chrono::seconds(secondsPause));
    s->PauseCapture();
    std::cout << "Capture paused after " << secondsPause << " seconds" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(secondsResume));
    s->ResumeCapture();
    std::cout << "Capture resumed after " << secondsResume << " seconds" << std::endl;
}

int main(int argc, char **argv) {
	//  return gtk_test(argc, argv);
    GtkApplication* app;
    int status;
    s = new ScreenRecorder();
#ifdef WIN32
    HWND Window;
    AllocConsole();
    Window = FindWindowA("ConsoleWindowClass", NULL);
    ShowWindow(Window,0);
#endif
    //if (future_is_ready(foo)){
    //    return 0;
    //}
//    auto s = ScreenRecorder();
//    s.init();
//    std::cout << "Initialized input streams" << std::endl;
//    s.init_outputfile();
//    std::cout << "Initialized output streams and file" << std::endl;
//    if (s.CaptureStart() >= 0) {
//        int millisecondsPause = 2000;
//        int millisecondsResume = 2000;
//        int millisecondsStop = 3000;
//        std::cout << "Capture started" << std::endl;
//        std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsPause));
//        s.PauseCapture();
//        std::cout << "Capture paused after " << ((double)millisecondsPause/1000) << " seconds" << std::endl;
//        std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsResume));
//        s.ResumeCapture();
//        std::cout << "Capture resumed after " << ((double)millisecondsResume/1000) << " seconds \n" << std::endl;
//        std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsStop));
//        s.CloseMediaFile();
//        std::cout << "Capture complete after " << ((double)millisecondsStop/1000) << " seconds \n" << std::endl;
//    }
//    s.init();
//    std::cout << "Initialized input streams" << std::endl;
//    s.init_outputfile();
//    std::cout << "Initialized output streams and file" << std::endl;
    app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), nullptr);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}



