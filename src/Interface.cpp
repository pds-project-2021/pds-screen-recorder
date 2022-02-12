#include "Interface.h"

// global ref of the interface for gtk callbacks
std::unique_ptr<Interface> t = nullptr;

int launchUI(int argc, char **argv) {
#ifdef WIN32
	HWND Window;
	AllocConsole();
	Window = FindWindowA("ConsoleWindowClass", nullptr);
	ShowWindow(Window,0);
#endif
	auto app = gtk_application_new("org.gtk.recs", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(Interface::activate), nullptr);
	auto status = g_application_run(G_APPLICATION(app), argc, argv);
//	g_object_unref(app);
	return status;
}

gboolean Interface::switchImageRec() {
	if (t->window == nullptr) return FALSE;

	if (t->started && !(t->s->is_paused())) {
		if (t->img_on) t->setImageRecOff();
		else t->setImageRecOn();
	} else if (!t->img_on) t->setImageRecOn();
	gtk_widget_queue_draw(t->window);

	return TRUE;
}

gboolean
on_widget_deleted(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    gtk_window_set_hide_on_close(GTK_WINDOW(t->fileChoiceDialog), true);
    delete_file(t->dest);

    gtk_window_close(GTK_WINDOW (t->fileChoiceDialog));
    #ifdef WIN32
    gtk_window_set_hide_on_close(GTK_WINDOW(t->window), false);
    gtk_window_minimize(GTK_WINDOW(t->window));
    gtk_window_present(GTK_WINDOW(t->window));
    gtk_window_unminimize(GTK_WINDOW(t->window));
    #else
    gtk_window_present(GTK_WINDOW(t->window));
    #endif

    // the user can record again only if the file dialog window is closed
    gtk_widget_set_sensitive(GTK_WIDGET(t->recordButton), true);
    std::cout << "File window closed" << std::endl;
    return TRUE;
}

Interface::Interface(GtkApplication *app) {
	g_application = app;

	window = gtk_application_window_new(app);
	selectWindow = gtk_application_window_new(app);
	recordWindow = gtk_application_window_new(app);
	selectionArea = gtk_drawing_area_new();
	ready = false;
	started = false;
	selection_enabled = false;
	headerBar = gtk_header_bar_new();
	image = gtk_image_new_from_file("../assets/icon_small.png");
	title = gtk_text_buffer_new(nullptr);
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(title), "  ", 2);
	titleView = gtk_text_view_new_with_buffer(title);
	gtk_window_set_title(GTK_WINDOW(window), "Screen recorder");
	gtk_window_set_default_size(GTK_WINDOW(window), 463, 50);
	gtk_window_set_default_size(GTK_WINDOW(recordWindow), 120, 30);
	gtk_widget_set_size_request(selectWindow, 2560, 1080);
	gtk_window_set_decorated(GTK_WINDOW(window), false);
	gtk_window_set_decorated(GTK_WINDOW(selectWindow), false);
	gtk_window_set_decorated(GTK_WINDOW(recordWindow), false);
	gtk_window_set_resizable(GTK_WINDOW(window), false);
	gtk_window_set_resizable(GTK_WINDOW(selectWindow), false);
	gtk_window_set_resizable(GTK_WINDOW(recordWindow), false);
	recordButton = gtk_button_new_with_label("Record");
	pauseButton = gtk_button_new_with_label("Pause");
	stopButton = gtk_button_new_with_label("Stop");
	startRecordButton = gtk_button_new_with_label("Start recording");

	g_signal_connect(recordButton, "clicked", G_CALLBACK(select_record_region), nullptr);
	g_signal_connect(startRecordButton, "clicked", G_CALLBACK(handleRecord), nullptr);
	g_signal_connect(pauseButton, "clicked", G_CALLBACK(handlePause), nullptr);
	g_signal_connect(stopButton, "clicked", G_CALLBACK(handleStop), nullptr);

	gtk_window_set_child(GTK_WINDOW(window), headerBar);
	gtk_window_set_child(GTK_WINDOW(recordWindow), startRecordButton);

	gtk_image_set_pixel_size(GTK_IMAGE(image), 32);
	gtk_header_bar_set_decoration_layout(GTK_HEADER_BAR(headerBar), ":minimize,close");
	gtk_header_bar_pack_start(GTK_HEADER_BAR(headerBar), image);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(headerBar), stopButton);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(headerBar), pauseButton);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(headerBar), recordButton);
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
	fileChoiceDialog = gtk_file_chooser_dialog_new("Open File",
	                                               nullptr,
	                                               action,
	                                               (const char *) "Cancel",
	                                               GTK_RESPONSE_CANCEL,
	                                               (const char *) "Save",
	                                               GTK_RESPONSE_ACCEPT,
	                                               NULL);
	fileChooser = GTK_FILE_CHOOSER (fileChoiceDialog);

	gtk_file_chooser_set_current_name(fileChooser, "Untitled.mp4");

	fileHandler = g_signal_connect (fileChoiceDialog, "response",
	                                G_CALLBACK(on_save_response),
	                                NULL);
    fileHandler = g_signal_connect (fileChoiceDialog, "destroy",
                                    G_CALLBACK(on_widget_deleted),
                                    NULL);
	gtk_window_set_hide_on_close(GTK_WINDOW(fileChoiceDialog), true);
	gtk_window_set_title(GTK_WINDOW(fileChoiceDialog), "Choose video capture file destination");

	gtk_window_set_child(GTK_WINDOW(selectWindow), selectionArea);
	leftGesture = gtk_gesture_click_new();
	rightGesture = gtk_gesture_click_new();
	gtk_gesture_single_set_button(GTK_GESTURE_SINGLE (leftGesture), 1);
	gtk_gesture_single_set_button(GTK_GESTURE_SINGLE (rightGesture), 3);
	motionController = gtk_event_controller_motion_new();
	g_signal_connect (motionController, "motion",
	                  G_CALLBACK(motion_detected), selectionArea);
	g_signal_connect (leftGesture, "pressed",
	                  G_CALLBACK(left_btn_pressed), selectionArea);
	g_signal_connect (leftGesture, "released",
	                  G_CALLBACK(left_btn_released), selectionArea);
	g_signal_connect (rightGesture, "pressed",
	                  G_CALLBACK(right_btn_pressed), selectionArea);
	g_signal_connect (rightGesture, "released",
	                  G_CALLBACK(right_btn_released), selectionArea);

	gtk_widget_add_controller(selectionArea, GTK_EVENT_CONTROLLER (leftGesture));
	gtk_widget_add_controller(selectionArea, GTK_EVENT_CONTROLLER (rightGesture));
	gtk_widget_add_controller(selectionArea, GTK_EVENT_CONTROLLER (motionController));

	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(handleClose), nullptr);
	gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(selectionArea), draw, nullptr, nullptr);
	gtk_window_present(GTK_WINDOW(window));
	gtk_window_set_hide_on_close(GTK_WINDOW(selectWindow), true);
	gtk_window_set_hide_on_close(GTK_WINDOW(recordWindow), true);
	gtk_widget_set_name(selectWindow, "selector_window");

	cssProvider = gtk_css_provider_new();
	gtk_css_provider_load_from_path(cssProvider, "../src/style.css");
	context = gtk_widget_get_style_context(selectWindow);
	gtk_style_context_add_provider(context,
	                               GTK_STYLE_PROVIDER(cssProvider),
	                               GTK_STYLE_PROVIDER_PRIORITY_USER); // I had used wrong priority on first try
	gtk_style_context_save(context);
	s = std::make_unique<Recorder>();
	gtk_widget_set_sensitive(GTK_WIDGET(pauseButton), false);
	gtk_widget_set_sensitive(GTK_WIDGET(stopButton), false);
	g_timeout_add_seconds(1, reinterpret_cast<GSourceFunc>(switchImageRec), window);
	img_on = true;

	blink_img = std::async(std::launch::async, switchImageRec);
}

Interface::~Interface() {
	g_print("Interface has been destroyed\n");
	// terminate capture if it's running
	if (s->is_capturing()) {
		s->terminate();
	}
//    if (surface) cairo_surface_destroy(surface);

#ifdef linux
//	gtk_window_destroy(GTK_WINDOW(window));
//    gtk_window_set_hide_on_close(GTK_WINDOW(selectWindow), false);
//    gtk_window_close(GTK_WINDOW(selectWindow));
//    gtk_window_set_hide_on_close(GTK_WINDOW(recordWindow), false);
//    gtk_window_close(GTK_WINDOW(recordWindow));
//    gtk_window_set_hide_on_close(GTK_WINDOW(fileChoiceDialog), false);
//    gtk_window_close(GTK_WINDOW(fileChoiceDialog));
#else
	//	gtk_window_set_hide_on_close(GTK_WINDOW(window), false);
	//	gtk_window_close(GTK_WINDOW(window));
#endif
}

/* Redraw the screen from the surface. Note that the ::draw
 * signal receives a ready-to-be-used cairo_t that is already
 * clipped to only draw the exposed areas of the widget
 */
void Interface::getRectCoordinates(double &offsetX, double &offsetY, double &width, double &height) const {
	width = std::abs(startX - endX);
	height = std::abs(startY - endY);
#ifdef WIN32
	offsetX = min(startX, endX);
	offsetY = min(startY, endY);
#else
	offsetX = std::min(startX, endX);
	offsetY = std::min(startY, endY);
#endif
}

void Interface::on_save_response(GtkDialog *, int response) {
	if (response == GTK_RESPONSE_ACCEPT) {
		auto destPath = g_file_get_path(gtk_file_chooser_get_file(t->fileChooser));
		move_file(t->dest, destPath);
	} else if (response == GTK_RESPONSE_CANCEL) {
		delete_file(t->dest);
	}

	gtk_window_close(GTK_WINDOW (t->fileChoiceDialog));
#ifdef WIN32
	gtk_window_set_hide_on_close(GTK_WINDOW(t->window), false);
	gtk_window_minimize(GTK_WINDOW(t->window));
	gtk_window_present(GTK_WINDOW(t->window));
	gtk_window_unminimize(GTK_WINDOW(t->window));
#else
	gtk_window_present(GTK_WINDOW(t->window));
#endif

	// the user can record again only if the file dialog window is closed
	gtk_widget_set_sensitive(GTK_WIDGET(t->recordButton), true);
}

void Interface::setImageRecOff() {

	gtk_header_bar_remove(GTK_HEADER_BAR(headerBar), image);
	image = gtk_image_new_from_file("../assets/icon_small_off.png");
	gtk_image_set_pixel_size(GTK_IMAGE(image), 32);
	gtk_header_bar_pack_start(GTK_HEADER_BAR(headerBar), image);
	img_on = false;
}

void Interface::setImageRecOn() {

	gtk_header_bar_remove(GTK_HEADER_BAR(headerBar), image);
	image = gtk_image_new_from_file("../assets/icon_small.png");
	gtk_image_set_pixel_size(GTK_IMAGE(image), 32);
	gtk_header_bar_pack_start(GTK_HEADER_BAR(headerBar), image);
	img_on = true;
}

//static void clear_surface(void) {
//	cairo_t *cr;
//
//	cr = cairo_create(t->surface);
//
//	cairo_set_source_rgb(cr, 0, 0, 0);
//	cairo_paint(cr);
//
//	cairo_destroy(cr);
//}

void Interface::draw_rect(cairo_t *cr) {
	double offsetX, offsetY, width, height;
	t->getRectCoordinates(offsetX, offsetY, width, height);
	cairo_rectangle(cr, offsetX, offsetY, width, height);
	cairo_set_source_rgba(cr, 0.3, 0.4, 0.6, 0.7);   /* set fill color */
	cairo_fill(cr);                            /* fill rectangle */
}

void Interface::draw(GtkDrawingArea *, cairo_t *cr, int, int, gpointer) {
	if (!t->surface) {
		cairo_set_source_rgba(cr, 0, 0, 0, 0.7);   /* set fill color */
	} else cairo_set_source_surface(cr, t->surface, 0, 0);
	cairo_paint(cr);
}

void Interface::motion_detected(GtkEventControllerMotion *, double x, double y, gpointer) {
	if (t->selection_enabled) {
		if (!t->surface) return;
		t->endX = x;
		t->endY = y;
		cairo_t *cr;
		cr = cairo_create(t->surface);
		cairo_set_source_rgba(cr, 0, 0, 0, 0.7);
		cairo_paint(cr);
		draw_rect(cr);
		cairo_destroy(cr);
		gtk_widget_queue_draw(GTK_WIDGET(t->selectionArea));
	}
//    else if (!gtk_window_is_active(GTK_WINDOW(t->recordWindow))) gtk_window_present(GTK_WINDOW(t->recordWindow));
}

void Interface::right_btn_pressed(GtkGestureClick *, int, double x, double y, GtkWidget *) {
	g_print("Left button pressed\n");
	std::cout << "Start coordinates: " << x << ", " << y << std::endl;
	gtk_window_close(GTK_WINDOW(t->selectWindow));
	gtk_window_close(GTK_WINDOW(t->recordWindow));
	if (t->surface) cairo_surface_destroy(t->surface);
	gtk_window_set_hide_on_close(GTK_WINDOW(t->window), false);
	gtk_window_present(GTK_WINDOW(t->window));
}

void Interface::right_btn_released(GtkGestureClick *gesture, int, double x, double y, GtkWidget *) {
	g_print("Right button released\n");
	std::cout << "End coordinates: " << x << ", " << y << std::endl;
	gtk_gesture_set_state(GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);
}

void Interface::left_btn_pressed(GtkGestureClick *, int, double x, double y, GtkWidget *) {
	g_print("Left button pressed\n");
	std::cout << "Start coordinates: " << x << ", " << y << std::endl;
	t->startX = x;
	t->startY = y;
	t->endX = x;
	t->endY = y;
	t->surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
	                                        gtk_widget_get_allocated_width(t->selectWindow),
	                                        gtk_widget_get_allocated_height(t->selectWindow));
	t->selection_enabled = true;
	gtk_window_close(GTK_WINDOW(t->recordWindow));
}

void Interface::left_btn_released(GtkGestureClick *gesture, int, double x, double y, GtkWidget *) {
	g_print("Left button released\n");
	std::cout << "End coordinates: " << x << ", " << y << std::endl;
	t->selection_enabled = false;
	t->endX = x;
	t->endY = y;
	gtk_gesture_set_state(GTK_GESTURE (gesture),
	                      GTK_EVENT_SEQUENCE_CLAIMED);
	cairo_t *cr;
	cr = cairo_create(t->surface);
	draw_rect(cr);
	cairo_destroy(cr);
	gtk_widget_queue_draw(GTK_WIDGET(t->selectionArea));
#ifdef WIN32
	gtk_window_close(GTK_WINDOW(t->selectWindow));
	gtk_window_present(GTK_WINDOW(t->selectWindow));
#endif
	gtk_window_present(GTK_WINDOW(t->recordWindow));
}

void Interface::recorder(double sX, double sY, double eX, double eY) {
	auto s = Screen{};

	auto width = std::abs(sX - eX);
	auto height = std::abs(sY - eY);
#ifdef WIN32
	auto off_x = min(sX, eX);
	auto off_y = min(sY, eY);
#else
	auto off_x = std::min(sX, eX);
	auto off_y = std::min(sY, eY);
#endif
	s.set_dimension(width, height);
	s.set_offset(off_x, off_y);

	if (s.fullscreen()) {
		log_info("Recording full screen area");
	} else {
		log_info("Recording " + s.get_video_size() + " area, with offset " + s.get_offset_str());
	}
    try {
        t->s->init(s);
        log_info("Initialized input streams");
        t->ready = true;
        t->started = false;
    }
    catch(avException e) {// handle recoverable libav exceptions during initialization
        GtkDialogFlags flags = static_cast<GtkDialogFlags>(GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL);
        dialog = gtk_message_dialog_new (GTK_WINDOW(window),
                                         flags,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_CLOSE,
                                         "Error initializing recorder structures : %s",
                                         e.what());
        g_signal_connect (dialog, "response",
                          G_CALLBACK (gtk_window_destroy),
                          NULL);
        std::cerr << "Error initializing recorder structures : " << e.what() << std::endl;
        t->ready = false;
    }
    catch(std::exception e) {// handle unexpected exceptions during initialization
        GtkDialogFlags flags = static_cast<GtkDialogFlags>(GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL);
        dialog = gtk_message_dialog_new (GTK_WINDOW(window),
                                         flags,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_CLOSE,
                                         "Unexpected error : %s",
                                         e.what());
        g_signal_connect (dialog, "response",
                          G_CALLBACK (gtk_window_destroy),
                          NULL);
        std::cerr << "Unexpected error : " << e.what() << std::endl;
        t->ready = false;
    }
}

void Interface::startRecording() {
	if (!t->ready) {
		t->recorder(t->startX, t->startY, t->endX, t->endY);
	}
	if (t->s->is_paused()) t->s->resume();
	else {
		if (!t->started) {
			t->s->capture();
			t->started = true;
		}
	}
    if(t->ready) {
        gtk_button_set_label(reinterpret_cast<GtkButton *>(t->recordButton), "Recording");
        gtk_widget_set_sensitive(GTK_WIDGET(t->recordButton), false);
        gtk_widget_set_sensitive(GTK_WIDGET(t->pauseButton), true);
        gtk_widget_set_sensitive(GTK_WIDGET(t->stopButton), true);
    }
}

void Interface::pauseRecording() {
	if (!t->ready) return;
	if (t->s->is_paused()) {
		t->s->resume();
		gtk_button_set_label(reinterpret_cast<GtkButton *>(t->pauseButton), "Pause");
		gtk_button_set_label(reinterpret_cast<GtkButton *>(t->recordButton), "Recording");
	} else {
		t->s->pause();
		gtk_button_set_label(reinterpret_cast<GtkButton *>(t->pauseButton), "Resume");
		gtk_button_set_label(reinterpret_cast<GtkButton *>(t->recordButton), "Paused");
	}
}

void Interface::stopRecording() {
	if (!t->ready) { return; }

	t->s->terminate();
	t->dest = t->s->get_destination();

	// set temporary name to current time
	auto file_name = get_current_time_str() + ".mp4";
	gtk_file_chooser_set_current_name(t->fileChooser, file_name.c_str());

	// reset recorder
	t->s = std::make_unique<Recorder>();
	t->ready = false;
	t->started = false;

	// reset action buttons
	gtk_button_set_label(reinterpret_cast<GtkButton *>(t->recordButton), "Record");
	gtk_widget_set_sensitive(GTK_WIDGET(t->pauseButton), false);
	gtk_widget_set_sensitive(GTK_WIDGET(t->stopButton), false);
}

void Interface::select_record_region(GtkWidget *, gpointer) {
#ifdef WIN32
	gtk_window_set_hide_on_close(GTK_WINDOW(t->window), true);
	gtk_window_close(GTK_WINDOW(t->window));
#else
	gtk_window_minimize(GTK_WINDOW(t->window));
#endif

	gtk_window_fullscreen(GTK_WINDOW(t->selectWindow));
	gtk_window_present(GTK_WINDOW(t->selectWindow));
	gtk_window_present(GTK_WINDOW(t->recordWindow));
	g_print("Record button pressed\n");
}

void Interface::handleRecord(GtkWidget *, gpointer) {
	if (t->s->is_capturing()) {
		t->rec = std::async(std::launch::async, stopRecording);
	}

	if (t->surface) cairo_surface_destroy(t->surface);
	t->surface = nullptr;
    gtk_window_close(GTK_WINDOW(t->recordWindow));
    gtk_window_close(GTK_WINDOW(t->selectWindow));
#ifdef WIN32
    gtk_window_set_hide_on_close(GTK_WINDOW(t->window), false);
#endif
    gtk_window_present(GTK_WINDOW(t->window));
    g_print("Start recording button pressed\n");
	t->rec = std::async(std::launch::async, startRecording);
}

void Interface::handlePause(GtkWidget *, gpointer) {
	t->rec = std::async(std::launch::async, pauseRecording);
	g_print("Pause button pressed\n");
}

void Interface::handleStop(GtkWidget *, gpointer) {
	t->rec = std::async(std::launch::async, stopRecording);
	g_print("Stop button pressed\n");

	// wait until the registration is completed
	t->rec.wait();
    gtk_window_set_hide_on_close(GTK_WINDOW(t->fileChoiceDialog), false);

#ifdef WIN32
	gtk_window_set_hide_on_close(GTK_WINDOW(t->window), true);
	gtk_window_close(GTK_WINDOW(t->window));
#endif
	gtk_window_present(GTK_WINDOW(t->fileChoiceDialog));
}

void Interface::handleClose(GtkWidget *, gpointer) {
	g_print("Close button pressed\n");
	g_application_quit(G_APPLICATION(t->g_application));
}

void Interface::activate(GtkApplication *app, gpointer) {
	if (app == nullptr) {
		throw uiException("Invalid application window");
	}

	t = std::make_unique<Interface>(app);
}

