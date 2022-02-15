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

	// workaround for logging cli args
	g_application_add_main_option_entries(G_APPLICATION(app), cmd_params);

	return g_application_run(G_APPLICATION(app), argc, argv);
}

void Interface::enable_blink() {
    // execute periodically blink image function with timeout for feedback when app is recording
    g_timeout_add_seconds(1, reinterpret_cast<GSourceFunc>(switchImageRec), window);
    blink_enabled = true;
}

gboolean Interface::switchImageRec() {
    try{
        if (t->window == nullptr) return FALSE;
        if (t->s->is_capturing() && !t->s->is_paused()) {
            if (t->img_on) {
                t->setImageRecOff();
            }else {
                t->setImageRecOn();
            }
        } else if (!t->img_on) {
            t->setImageRecOn();
        }
        gtk_widget_queue_draw(t->window);
    }
    catch(std::runtime_error &e) {
        std::cerr << "Error updating blinking recording icon for user notification: " << e.what() << std::endl;
        t->blink_enabled = false;
        return FALSE;
    }
    catch(std::exception &e) {
        std::cerr << "Error updating blinking recording icon for user notification: " << e.what() << std::endl;
        t->blink_enabled = false;
        return FALSE;
    }
    catch(...) {
        std::cerr << "Error updating blinking recording icon for user notification" << std::endl;
        t->blink_enabled = false;
        return FALSE;
    }
	return TRUE;
}

void Interface::init_error_dialog() {
	GtkWidget *content_area;
	auto flags = (GtkDialogFlags) (GTK_DIALOG_MODAL);
	dialog = gtk_dialog_new_with_buttons("Error",
	                                     GTK_WINDOW(window),
	                                     flags,
	                                     "_OK",
	                                     GTK_RESPONSE_CLOSE,
	                                     NULL);
	content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));
	label = gtk_label_new("Unexpected error!");
//    dialog = gtk_message_dialog_new (GTK_WINDOW(window),
//                                     flags,
//                                     GTK_MESSAGE_ERROR,
//                                     GTK_BUTTONS_CLOSE,
//                                     "Unexpected error!");
	g_signal_connect (dialog, "response",
	                  G_CALLBACK(gtk_widget_hide),
	                  NULL);
	g_signal_connect (dialog, "destroy",
	                  G_CALLBACK(gtk_widget_hide),
	                  NULL);
	gtk_window_set_deletable(GTK_WINDOW(dialog), false);
	gtk_box_append(GTK_BOX (content_area), label);
	gtk_widget_show(dialog);
	gtk_widget_hide(dialog);
}

void Interface::set_error_dialog_msg(const char *msg) const {
	if (msg != nullptr) {
		GtkWidget *content_area;
		content_area = gtk_dialog_get_content_area(GTK_DIALOG (t->dialog));
		gtk_box_remove(GTK_BOX (content_area), t->label);
		t->label = gtk_label_new(msg);
		gtk_box_append(GTK_BOX (content_area), label);
	}
}

gboolean Interface::on_dialog_deleted() {
	gtk_window_destroy(GTK_WINDOW(t->dialog));
	auto flags = (GtkDialogFlags) (GTK_DIALOG_MODAL);
	t->dialog = gtk_message_dialog_new(GTK_WINDOW(t->window),
	                                   flags,
	                                   GTK_MESSAGE_ERROR,
	                                   GTK_BUTTONS_CLOSE,
	                                   "Unexpected error!");
	g_signal_connect (t->dialog, "response",
	                  G_CALLBACK(Interface::on_dialog_deleted),
	                  NULL);
	log_info("Error dialog closed");
	gtk_widget_show(t->dialog);
	gtk_widget_hide(t->dialog);
	return TRUE;
}

gboolean Interface::on_widget_deleted() {
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
	log_info("File window closed");
	return TRUE;
}

Interface::Interface(GtkApplication *app) {
	g_application = app;

	// allocation for gtk window object
	window = gtk_application_window_new(app);
	selectWindow = gtk_application_window_new(app);
	recordWindow = gtk_application_window_new(app);
	selectionArea = gtk_drawing_area_new();

	// main window setup
	headerBar = gtk_header_bar_new();
	auto pix = gdk_pixbuf_new_from_xpm_data(icon_on);
	image = gtk_image_new_from_pixbuf(pix);
	title = gtk_text_buffer_new(nullptr);
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(title), "  ", 2);
	titleView = gtk_text_view_new_with_buffer(title);
	gtk_window_set_title(GTK_WINDOW(window), "Screen recorder");

	// setup windows' size
#ifdef WIN32
	gtk_window_set_default_size(GTK_WINDOW(window), 463, 50);
#else
	gtk_window_set_default_size(GTK_WINDOW(window), 500, 50);
#endif
	gtk_window_set_default_size(GTK_WINDOW(recordWindow), 120, 30);

	// get full screen geometry for `selectionWindow` size setup
	GdkRectangle geometry;
	auto monitor = reinterpret_cast<GdkMonitor *>(g_list_model_get_item(gdk_display_get_monitors(gtk_widget_get_display(
		selectWindow)), 0));
	gdk_monitor_get_geometry(monitor, &geometry);
	gtk_window_set_default_size(GTK_WINDOW(selectWindow), geometry.width, geometry.height);

	log_info(
		"Screen parameters: width=" + std::to_string(geometry.width) + " height=" + std::to_string(geometry.height));

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

	// file choice dialog
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
#ifdef WIN32
	fileChoiceDialog = gtk_file_chooser_dialog_new("Save File",
												   nullptr,
												   action,
												   ("_Cancel"),
												   GTK_RESPONSE_CANCEL,
												   ("_Save"),
												   GTK_RESPONSE_ACCEPT,
												   NULL);
#else
	fileChoiceDialog = gtk_file_chooser_dialog_new("Save File",
	                                               GTK_WINDOW(window),
	                                               action,
	                                               ("_Cancel"),
	                                               GTK_RESPONSE_CANCEL,
	                                               ("_Save"),
	                                               GTK_RESPONSE_ACCEPT,
	                                               NULL);
#endif
	fileChooser = GTK_FILE_CHOOSER (fileChoiceDialog);
	auto filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.mp4");
	gtk_file_chooser_add_filter(fileChooser, filter);

	gtk_file_chooser_set_current_name(fileChooser, ("Untitled.mp4"));

	fileHandler = g_signal_connect (fileChoiceDialog, "response",
	                                G_CALLBACK(on_save_response),
	                                NULL);
	fileHandler = g_signal_connect (fileChoiceDialog, "destroy",
	                                G_CALLBACK(on_widget_deleted),
	                                NULL);
	gtk_window_set_hide_on_close(GTK_WINDOW(fileChoiceDialog), true);
	gtk_window_set_title(GTK_WINDOW(fileChoiceDialog), "Choose video capture file destination");
	gtk_file_chooser_set_select_multiple(fileChooser, false);

	// selection area with a controller for mouse click
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

	// custom css for selectionArea transparency
	cssProvider = gtk_css_provider_new();
	auto css = "* { background-color: transparent; opacity: 0.7; }";
	gtk_css_provider_load_from_data(cssProvider, css, static_cast<gssize>(strlen(css)));

	context = gtk_widget_get_style_context(selectWindow);
	gtk_style_context_add_provider(context,
	                               GTK_STYLE_PROVIDER(cssProvider),
	                               GTK_STYLE_PROVIDER_PRIORITY_USER); // I had used wrong priority on first try
	gtk_style_context_save(context);

	// allocate `Recorder` and enable gui buttons
	s = std::make_unique<Recorder>();
	gtk_widget_set_sensitive(GTK_WIDGET(pauseButton), false);
	gtk_widget_set_sensitive(GTK_WIDGET(stopButton), false);
    //enable blinking user notification for recording
    enable_blink();

	// error dialog for runtime exceptions
    init_error_dialog();
}

Interface::~Interface() {
	log_info("Interface has been destroyed");

	// terminate capture if it's running
	if (s->is_capturing()) {
		s->terminate();
	}
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
		auto folder = g_file_get_path(gtk_file_chooser_get_current_folder(t->fileChooser));
		auto file_name = gtk_file_chooser_get_current_name(t->fileChooser);
		auto destPath = (std::filesystem::path(folder) / file_name).string();

		move_file(t->dest, destPath);
		log_info("Saved file in: " + destPath);
	} else if (response == GTK_RESPONSE_CANCEL) {
		delete_file(t->dest);
	}

	gtk_window_close(GTK_WINDOW (t->fileChoiceDialog));
#ifdef WIN32
	gtk_window_set_hide_on_close(GTK_WINDOW(t->window), false);
	//prevents freeze on windows (nvidia)
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
	auto pix = gdk_pixbuf_new_from_xpm_data(icon_off);
	image = gtk_image_new_from_pixbuf(pix);
	gtk_image_set_pixel_size(GTK_IMAGE(image), 32);
	gtk_header_bar_pack_start(GTK_HEADER_BAR(headerBar), image);
	img_on = false;
}

void Interface::setImageRecOn() {
	gtk_header_bar_remove(GTK_HEADER_BAR(headerBar), image);
	auto pix = gdk_pixbuf_new_from_xpm_data(icon_on);
	image = gtk_image_new_from_pixbuf(pix);
	gtk_image_set_pixel_size(GTK_IMAGE(image), 32);
	gtk_header_bar_pack_start(GTK_HEADER_BAR(headerBar), image);
	img_on = true;
}

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
	log_info("Left button pressed");
	log_info("Start coordinates: " + std::to_string(x) + ", " + std::to_string(y));

	gtk_window_close(GTK_WINDOW(t->selectWindow));
	gtk_window_close(GTK_WINDOW(t->recordWindow));
	if (t->surface) cairo_surface_destroy(t->surface);
	gtk_window_set_hide_on_close(GTK_WINDOW(t->window), false);
	gtk_window_present(GTK_WINDOW(t->window));
}

void Interface::right_btn_released(GtkGestureClick *gesture, int, double x, double y, GtkWidget *) {
	log_info("Right button released");
	log_info("End coordinates: " + std::to_string(x) + ", " + std::to_string(y));

	gtk_gesture_set_state(GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);
}

void Interface::left_btn_pressed(GtkGestureClick *, int, double x, double y, GtkWidget *) {
	log_info("Left button pressed");
	log_info("Start coordinates: " + std::to_string(x) + ", " + std::to_string(y));

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
	log_info("Left button released");
	log_info("End coordinates: " + std::to_string(x) + ", " + std::to_string(y));

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

void Interface::init_recorder(double sX, double sY, double eX, double eY) {
	auto screen = Screen{};

	auto width = std::abs(sX - eX);
	auto height = std::abs(sY - eY);
#ifdef WIN32
	auto off_x = min(sX, eX);
	auto off_y = min(sY, eY);
#else
	auto off_x = std::min(sX, eX);
	auto off_y = std::min(sY, eY);
#endif
	screen.set_dimension(width, height);
	screen.set_offset(off_x, off_y);

	if (screen.fullscreen()) {
		log_info("Recording full screen area");
	} else {
		log_info("Recording " + screen.get_video_size() + " area, with offset " + screen.get_offset_str());
	}
//	t->s->set_audio_codec("ciao");

	t->s->init(screen);
	log_info("Initialized input streams");
	t->ready = true;
}

void Interface::startRecording() {
	try {
		if (!t->ready) {
			t->init_recorder(t->startX, t->startY, t->endX, t->endY);
		}
		if (t->s->is_paused()) {
			t->s->resume();
		} else if (!t->s->is_capturing()){
			t->s->capture();
		}

        if (!t->blink_enabled) t->enable_blink();
		gtk_button_set_label(reinterpret_cast<GtkButton *>(t->recordButton), "Recording");
		gtk_widget_set_sensitive(GTK_WIDGET(t->recordButton), false);
		gtk_widget_set_sensitive(GTK_WIDGET(t->pauseButton), true);
		gtk_widget_set_sensitive(GTK_WIDGET(t->stopButton), true);

	}
	catch (avException &e) {// handle recoverable libav exceptions during initialization
		log_error("Error initializing recorder structures: " + std::string(e.what()));
        t->reset_gui_from_start();
		if (t->dialog) t->set_error_dialog_msg(e.what());
		gtk_widget_show(t->dialog);
	}
	catch (...) {// handle unexpected exceptions during initialization
		log_error("Unexpected error!");
        t->reset_gui_from_start();
		if (t->dialog) t->set_error_dialog_msg(nullptr);
		gtk_widget_show(t->dialog);
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
	//get temporary file location
	t->dest = t->s->get_destination();
	// reset recorder
	t->s = std::make_unique<Recorder>();
	t->ready = false;
	// set temporary name to current time
	auto file_name = get_current_time_str() + ".mp4";
	gtk_file_chooser_set_current_name(t->fileChooser, file_name.c_str());
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
	log_info("Record button pressed");
}


void Interface::reset_gui_from_start() {
    // delete recorder if necessary
    if(ready) {
        s = std::make_unique<Recorder>();
        ready = false;
    }
    // reset action buttons
    gtk_button_set_label(reinterpret_cast<GtkButton *>(recordButton), "Record");
    gtk_widget_set_sensitive(GTK_WIDGET(recordButton), true);
    gtk_widget_set_sensitive(GTK_WIDGET(pauseButton), false);
    gtk_widget_set_sensitive(GTK_WIDGET(stopButton), false);
}

void Interface::reset_gui_from_exec() {
	// delete recorder if necessary
	if (ready) {
		s = std::make_unique<Recorder>();
		ready = false;
	}
	// reset action buttons
	gtk_button_set_label(reinterpret_cast<GtkButton *>(recordButton), "Record");
	gtk_widget_set_sensitive(GTK_WIDGET(recordButton), true);
	gtk_widget_set_sensitive(GTK_WIDGET(pauseButton), false);
	gtk_widget_set_sensitive(GTK_WIDGET(stopButton), false);
#ifdef WIN32
	gtk_window_minimize(GTK_WINDOW(window));
	gtk_window_present(GTK_WINDOW(window));
	gtk_window_unminimize(GTK_WINDOW(window));
#endif
}

void Interface::reset_gui_from_stop() {
	// delete recorder if necessary
	if (ready) {
		s = std::make_unique<Recorder>();
		ready = false;
	}
	// reset action buttons
	gtk_button_set_label(reinterpret_cast<GtkButton *>(recordButton), "Record");
	gtk_widget_set_sensitive(GTK_WIDGET(recordButton), true);
	gtk_widget_set_sensitive(GTK_WIDGET(pauseButton), false);
	gtk_widget_set_sensitive(GTK_WIDGET(stopButton), false);
	// restore windows' states
	gtk_window_set_hide_on_close(GTK_WINDOW(fileChoiceDialog), false);
	gtk_window_close(GTK_WINDOW(fileChoiceDialog));
#ifdef WIN32
	gtk_window_minimize(GTK_WINDOW(window));
	gtk_window_set_hide_on_close(GTK_WINDOW(window), false);
	gtk_window_present(GTK_WINDOW(window));
	gtk_window_unminimize(GTK_WINDOW(window));
#endif
}

void Interface::handleRecord(GtkWidget *, gpointer) {
	if (t->s->is_capturing()) {
		t->rec = std::async(std::launch::async, stopRecording);
	}

	if (t->surface) cairo_surface_destroy(t->surface);
	t->surface = nullptr;

	t->rec = std::async(std::launch::async, startRecording);

	// close selection window
	gtk_window_close(GTK_WINDOW(t->recordWindow));
	gtk_window_close(GTK_WINDOW(t->selectWindow));
#ifdef WIN32
	gtk_window_set_hide_on_close(GTK_WINDOW(t->window), false);
#endif
	gtk_window_present(GTK_WINDOW(t->window));
	log_info("Start recording button pressed");
}

void Interface::handlePause(GtkWidget *, gpointer) {
	try {
		t->rec = std::async(std::launch::async, pauseRecording);
		log_info("Pause button pressed");
		t->rec.get();
	}
	catch (avException &e) {// handle recoverable libav exceptions during pausing
		log_error("Error closing output streams: " + std::string(e.what()));
		t->reset_gui_from_exec();
		//show error message dialog
		if (t->dialog) t->set_error_dialog_msg(e.what());
		gtk_widget_show(t->dialog);
	}
	catch (...) {// handle unexpected exceptions during pausing
		log_error("Unexpected error!");
		t->reset_gui_from_exec();
		//show error message dialog
		if (t->dialog) t->set_error_dialog_msg(nullptr);
		gtk_widget_show(t->dialog);
	}
}

void Interface::handleStop(GtkWidget *, gpointer) {
    try {
        t->rec = std::async(std::launch::async, stopRecording);
        log_info("Stop button pressed");
        // variable to check if windows' states need to be restored
        // wait until the registration is completed
	    t->rec.get();
        // set windows' states for file saving
        gtk_window_set_hide_on_close(GTK_WINDOW(t->fileChoiceDialog), true);
#ifdef WIN32
        gtk_window_set_hide_on_close(GTK_WINDOW(t->window), true);
        gtk_window_close(GTK_WINDOW(t->window));
#endif
        gtk_window_present(GTK_WINDOW(t->fileChoiceDialog));
    }
    catch(avException &e) {// handle recoverable libav exceptions during termination
        log_error("Error closing output streams: " + std::string(e.what()));
        t->reset_gui_from_stop();
        //show error message dialog
        if (t->dialog) t->set_error_dialog_msg(e.what());
        gtk_widget_show(t->dialog);
    }
    catch(...) {// handle unexpected exceptions during termination
        log_error("Unexpected error!");
        t->reset_gui_from_stop();
        //show error message dialog
        if (t->dialog) t->set_error_dialog_msg(nullptr);
        gtk_widget_show(t->dialog);
    }
}

void Interface::handleClose(GtkWidget *, gpointer) {
	log_info("Close button pressed");
	g_application_quit(G_APPLICATION(t->g_application));
    try {
        g_application_quit(G_APPLICATION(t->g_application));
    }
    catch(...) {
        throw uiException("Error while closing user interface");
    }
}

void Interface::activate(GtkApplication *app, gpointer) {
	if (app == nullptr) {
		throw uiException("Invalid application window");
	}
    try {
        t = std::make_unique<Interface>(app);
    }
    catch(...) {
        std::cerr << "Error during interface initialization" << std::endl;
        throw uiException("Could not launch user interface");
    }
}

