#include <gtk/gtk.h>
#include <iostream>

#include "ScreenRecorder.h"

GtkWidget *window;
ScreenRecorder *s;
bool ready = false;

void init_output() {
    s->init_outputfile();
    ready = true;
    std::cout << "Initialized output streams and file" << std::endl;
}

void recorder() {
    s->init();
    std::cout << "Initialized input streams" << std::endl;
    init_output();
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
        s->CaptureStart();
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
	GtkWidget *recordButton;
	GtkWidget *pauseButton;
	GtkWidget *stopButton;
	// GtkWidget* closeButton;
	GtkWidget *headerBar;
	GtkWidget *image;
	GtkTextBuffer *title;
	GtkWidget *titleView;

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
	g_signal_connect(recordButton, "clicked", G_CALLBACK(record), nullptr);
	g_signal_connect(pauseButton, "clicked", G_CALLBACK(_pause), nullptr);
	g_signal_connect(stopButton, "clicked", G_CALLBACK(stop), nullptr);
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


//int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
//            LPTSTR    lpCmdLine, int       cmdShow) {
////      return gtk_test(argc, argv);
////    LPMSG msg = new tagMSG;
////    BlockInput(true);
////    SetConsoleCtrlHandler ((PHANDLER_ROUTINE) ControlHandler, TRUE);
//    std::future<void> foo = std::async(std::launch::async, recorder);
//    while(!future_is_ready(foo));
//    return 0;
////    while (true)
////    {
////        if(PeekMessage(msg,NULL,0,0,PM_REMOVE))
////        {
////            TranslateMessage(msg);
////            DispatchMessage(msg);
////        }
////        else
////        {
////            if(future_is_ready(foo)) {
////                free(msg);
////                return 0;
////            }
////        }
////    }
////    while(!future_is_ready(foo));
////        return 0;
////    auto s = ScreenRecorder();
////    s.init();
////    std::cout << "Initialized input streams" << std::endl;
////    s.init_outputfile();
////    std::cout << "Initialized output streams and file" << std::endl;
////    if (s.CaptureStart() >= 0) {
////        int millisecondsPause = 2000;
////        int millisecondsResume = 2000;
////        int millisecondsStop = 3000;
////        std::cout << "Capture started" << std::endl;
////        std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsPause));
////        s.PauseCapture();
////        std::cout << "Capture paused after " << ((double)millisecondsPause/1000) << " seconds" << std::endl;
////        std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsResume));
////        s.ResumeCapture();
////        std::cout << "Capture resumed after " << ((double)millisecondsResume/1000) << " seconds \n" << std::endl;
////        std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsStop));
////        s.CloseMediaFile();
////        std::cout << "Capture complete after " << ((double)millisecondsStop/1000) << " seconds \n" << std::endl;
////    }
//}
