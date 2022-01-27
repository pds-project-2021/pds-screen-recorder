//
// Created by gabriele on 31/10/21.
//

#include "include/Interface.h"

#include <memory>

//GtkWidget *window;
//GtkWidget *selectWindow;
//GtkWidget *recordWindow;
//GtkWidget *recordButton;
//GtkWidget *startRecordButton;
//GtkWidget *pauseButton;
//GtkWidget *stopButton;
//GtkWidget *headerBar;
//atomic<bool> selection_enabled;
//GtkWidget *image;
//GtkTextBuffer *title;
//GtkGesture *leftGesture;
//GtkGesture *rightGesture;
//GtkGesture *moveGesture;
//GtkEventController *motionController;
//GtkWidget *selectionArea;
//GtkCssProvider *cssProvider;
//GtkStyleContext *context;
//cairo_surface_t *surface = nullptr;
//cairo_surface_t *background;
//GtkWidget *titleView;
//double startX = 0;
//double startY = 0;
//double endX = 0;
//double endY = 0;
//atomic<bool> ready;
//atomic<bool> started;
//
//
//unique_ptr<Recorder> s = nullptr;

// this is used for correct closing gtk windows
//bool recordered = false;

Interface* t = nullptr;

int Interface::launchUI(int argc, char **argv){
	GtkApplication *app;
	int status;

	s = std::make_unique<Recorder>();

#ifdef WIN32
	HWND Window;
	AllocConsole();
	Window = FindWindowA("ConsoleWindowClass", NULL);
	ShowWindow(Window,0);
#endif
	app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), nullptr);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);
	return status;
}

static void clear_surface(void) {
	cairo_t *cr;

	cr = cairo_create(t->surface);

	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_paint(cr);

	cairo_destroy(cr);
}

/* Redraw the screen from the surface. Note that the ::draw
 * signal receives a ready-to-be-used cairo_t that is already
 * clipped to only draw the exposed areas of the widget
 */

//#ifdef WIN32
//BOOL SaveToFile(HBITMAP hBitmap3, LPCTSTR lpszFileName)
//{
//    HDC hDC;
//    int iBits;
//    WORD wBitCount;
//    DWORD dwPaletteSize=0, dwBmBitsSize=0, dwDIBSize=0, dwWritten=0;
//    BITMAP Bitmap0;
//    BITMAPFILEHEADER bmfHdr;
//    BITMAPINFOHEADER bi;
//    LPBITMAPINFOHEADER lpbi;
//    HANDLE fh, hDib, hPal,hOldPal2=NULL;
//    hDC = CreateDC("DISPLAY", NULL, NULL, NULL);
//    iBits = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
//    DeleteDC(hDC);
//    if (iBits <= 1)
//        wBitCount = 1;
//    else if (iBits <= 4)
//        wBitCount = 4;
//    else if (iBits <= 8)
//        wBitCount = 8;
//    else
//        wBitCount = 24;
//    GetObject(hBitmap3, sizeof(Bitmap0), (LPSTR)&Bitmap0);
//    bi.biSize = sizeof(BITMAPINFOHEADER);
//    bi.biWidth = Bitmap0.bmWidth;
//    bi.biHeight =-Bitmap0.bmHeight;
//    bi.biPlanes = 1;
//    bi.biBitCount = wBitCount;
//    bi.biCompression = BI_RGB;
//    bi.biSizeImage = 0;
//    bi.biXPelsPerMeter = 0;
//    bi.biYPelsPerMeter = 0;
//    bi.biClrImportant = 0;
//    bi.biClrUsed = 256;
//    dwBmBitsSize = ((Bitmap0.bmWidth * wBitCount +31) & ~31) /8
//                   * Bitmap0.bmHeight;
//    hDib = GlobalAlloc(GHND,dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER));
//    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
//    *lpbi = bi;
//
//    hPal = GetStockObject(DEFAULT_PALETTE);
//    if (hPal)
//    {
//        hDC = GetDC(NULL);
//        hOldPal2 = SelectPalette(hDC, (HPALETTE)hPal, FALSE);
//        RealizePalette(hDC);
//    }
//
//
//    GetDIBits(hDC, hBitmap3, 0, (UINT) Bitmap0.bmHeight, (LPSTR)lpbi + sizeof(BITMAPINFOHEADER)
//                                                         +dwPaletteSize, (BITMAPINFO *)lpbi, DIB_RGB_COLORS);
//
//    if (hOldPal2)
//    {
//        SelectPalette(hDC, (HPALETTE)hOldPal2, TRUE);
//        RealizePalette(hDC);
//        ReleaseDC(NULL, hDC);
//    }
//
//    fh = CreateFile(lpszFileName, GENERIC_WRITE,0, NULL, CREATE_ALWAYS,
//                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
//
//    if (fh == INVALID_HANDLE_VALUE)
//        return FALSE;
//
//    bmfHdr.bfType = 0x4D42; // "BM"
//    dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwPaletteSize + dwBmBitsSize;
//    bmfHdr.bfSize = dwDIBSize;
//    bmfHdr.bfReserved1 = 0;
//    bmfHdr.bfReserved2 = 0;
//    bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize;
//
//    WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
//
//    WriteFile(fh, (LPSTR)lpbi, dwDIBSize, &dwWritten, NULL);
//    GlobalUnlock(hDib);
//    GlobalFree(hDib);
//    CloseHandle(fh);
//
//    return TRUE;
//}
//
//int screenCapture(int x, int y, int w, int h, LPCSTR fname)
//{
//    HDC hdcSource = GetDC(NULL);
//    HDC hdcMemory = CreateCompatibleDC(hdcSource);
//
//    int capX = GetDeviceCaps(hdcSource, HORZRES);
//    int capY = GetDeviceCaps(hdcSource, VERTRES);
//
//    HBITMAP hBitmap = CreateCompatibleBitmap(hdcSource, w, h);
//    HBITMAP hBitmapOld = (HBITMAP)SelectObject(hdcMemory, hBitmap);
//
//    BitBlt(hdcMemory, 0, 0, w, h, hdcSource, x, y, SRCCOPY);
//    hBitmap = (HBITMAP)SelectObject(hdcMemory, hBitmapOld);
//
//    DeleteDC(hdcSource);
//    DeleteDC(hdcMemory);
//
//    HPALETTE hpal = NULL;
//    if(SaveToFile(hBitmap, fname)) return 1;
//    return 0;
//}
//#endif

void Interface::getRectCoordinates(double& offsetX, double& offsetY, double& width, double& height) {
    if (startX > endX) {
        if (startY > endY) {
            offsetX = startX;
            offsetY = startY;
            width = endX - startX;
            height = endY - startY;
        }
        else {
            offsetX = startX;
            offsetY = endY;
            width = endX - startX;
            height = startY - endY;
        }
    } else {
        if (startY > endY) {
            offsetX = endX;
            offsetY = startY;
            width = startX - endX;
            height = endY - startY;
        }
        else {
            offsetX = endX;
            offsetY = endY;
            width = startX - endX;
            height = startY - endY;
        }
    }
}

Interface::Interface() {
    t = this;
};

static void draw_rect(cairo_t *cr) {
//    cairo_set_source_rgb (cr, 1, 1, 1);
    double offsetX, offsetY, width, height;
    t->getRectCoordinates(offsetX, offsetY, width, height);
    cairo_rectangle(cr, offsetX, offsetY, width, height);
//	if (startX > endX) {
//		if (startY > endY) cairo_rectangle(cr, startX, startY, endX - startX, endY - startY);     /* set rectangle */
//		else cairo_rectangle(cr, startX, endY, endX - startX, startY - endY);
//	} else {
//		if (startY > endY) cairo_rectangle(cr, endX, startY, startX - endX, endY - startY);     /* set rectangle */
//		else cairo_rectangle(cr, endX, endY, startX - endX, startY - endY);
//	}
//    cairo_rectangle (cr, 10, 10, 180, 180);
	cairo_set_source_rgba(cr, 0.3, 0.4, 0.6, 0.7);   /* set fill color */
	cairo_fill(cr);                            /* fill rectangle */
}

static void draw(GtkDrawingArea *drawing_area, cairo_t *cr, int width, int height, gpointer data) {
	if (!t->surface) {
		cairo_set_source_rgba(cr, 0, 0, 0, 0.7);   /* set fill color */
	} else cairo_set_source_surface(cr, t->surface, 0, 0);
	cairo_paint(cr);
}

void motion_detected(GtkEventControllerMotion *controller, double x, double y, gpointer user_data) {
	if (t->selection_enabled) {
		if (!t->surface) return;
        t->endX = x;
        t->endY = y;
		cairo_t *cr;
//        cairo_surface_destroy(surface);
//        surface = cairo_image_surface_create_from_png("../src/screen.png");
		cr = cairo_create(t->surface);
		cairo_set_source_rgba(cr, 0, 0, 0, 0.7);
		cairo_paint(cr);
		draw_rect(cr);
		cairo_destroy(cr);
		gtk_widget_queue_draw(GTK_WIDGET(t->selectionArea));
	} else if (!gtk_window_is_active(GTK_WINDOW(t->recordWindow))) gtk_window_present(GTK_WINDOW(t->recordWindow));
}

static void right_btn_pressed(GtkGestureClick *gesture, int n_press, double x, double y, GtkWidget *widget) {
	g_print("Left button pressed\n");
	std::cout << "Start coordinates: " << x << ", " << y << std::endl;
	gtk_window_close(GTK_WINDOW(t->selectWindow));
	gtk_window_close(GTK_WINDOW(t->recordWindow));
	if (t->surface) cairo_surface_destroy(t->surface);
//    t->surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, gtk_widget_get_allocated_width (selectWindow), gtk_widget_get_allocated_height (selectWindow));
	gtk_window_set_hide_on_close(GTK_WINDOW(t->window), false);
	gtk_window_present(GTK_WINDOW(t->window));

	auto c = gtk_window_get_hide_on_close(GTK_WINDOW(t->window));
}

static void right_btn_released(GtkGestureClick *gesture, int n_press, double x, double y, GtkWidget *widget) {
	g_print("Right button released\n");
	std::cout << "End coordinates: " << x << ", " << y << std::endl;
	gtk_gesture_set_state(GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);
}

//void refresh() {
//    while(gtk_widget_is_visible(GTK_WIDGET(selectWindow))){
////        if (surface)
////            cairo_surface_destroy (surface);
////        surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, gtk_widget_get_allocated_width (selectWindow), gtk_widget_get_allocated_height (selectWindow));
////        draw_rect(cairo_create (surface));
//        gtk_widget_queue_draw(GTK_WIDGET(selectionArea));
//        this_thread::sleep_for(chrono::milliseconds(16));
//    };
//}
static void left_btn_pressed(GtkGestureClick *gesture, int n_press, double x, double y, GtkWidget *widget) {
	g_print("Left button pressed\n");
	std::cout << "Start coordinates: " << x << ", " << y << std::endl;
    t->startX = x;
    t->startY = y;
    t->endX = x;
    t->endY = y;
//    if (surface) cairo_surface_destroy (surface);
//    screenCapture(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), "../src/screen.png");
//    background = cairo_image_surface_create_from_png("../src/screen.png");
//    surface = cairo_image_surface_create_from_png("../src/screen.png");
    t->surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
	                                     gtk_widget_get_allocated_width(t->selectWindow),
	                                     gtk_widget_get_allocated_height(t->selectWindow));
    t->selection_enabled = true;
	gtk_window_close(GTK_WINDOW(t->recordWindow));
}

static void left_btn_released(GtkGestureClick *gesture, int n_press, double x, double y, GtkWidget *widget) {
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

static void drag(GtkGestureDrag *gesture, double offset_x, double offset_y, gpointer user_data) {
	cairo_surface_flush(t->surface);
	gtk_gesture_set_state(GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);
}

void recorder(int sX, int sY, int eX, int eY) {
	if (sX == sY || eX == eY) {
        t->s->init(Screen(0, 0, 0, 0));
        std::cout << "Recording area parameters set to 0" << std::endl;
    }
	else if (sX > eX) {
		if (sY > eY) t->s->init(Screen((int) (sX - eX), (int) (sY - eY), (int) eX, (int) eY));
		else t->s->init(Screen((int) (sX - eX), (int) (eY - sY), (int) eX, (int) sY));
	} else {
		if (sY > eY) t->s->init(Screen((int) (eX - sX), (int) (sY - eY), (int) sX, (int) eY));
		else t->s->init(Screen((int) (eX - sX), (int) (eY - sY), (int) sX, (int) sY));
	}

	std::cout << "Initialized input streams" << std::endl;
    t->ready = true;
    t->started = false;
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
	if (!t->ready) recorder(t->startX, t->startY, t->endX, t->endY);
	if (t->s->is_paused()) t->s->resume();
	else {
		if (!t->started) {
            t->s->capture();
            t->started = true;
		}
	}
}

void pauseRecording() {
	if (!t->ready) return;
	if (t->s->is_paused()) {
        t->s->resume();
		gtk_button_set_label(reinterpret_cast<GtkButton *>(t->pauseButton), "Pause");
	}else {
        t->s->pause();
		gtk_button_set_label(reinterpret_cast<GtkButton *>(t->pauseButton), "Resume");
	}
}

void stopRecording() {
	if (!t->ready) return;
    t->s->terminate();
    t->s = std::make_unique<Recorder>();
    t->ready = false;
    t->started = false;
}

void select_record_region(GtkWidget *widget, gpointer data) {
	gtk_window_set_hide_on_close(GTK_WINDOW(t->window), true);
	gtk_window_close(GTK_WINDOW(t->window));
	gtk_window_fullscreen(GTK_WINDOW(t->selectWindow));
	gtk_window_present(GTK_WINDOW(t->selectWindow));
	g_print("Record button pressed\n");
	auto h = gtk_widget_get_height(t->selectionArea);
}

void handleRecord(GtkWidget *widget, gpointer data) {
	if (t->s->is_capturing()) {
		std::future<void> foo = std::async(std::launch::async, stopRecording);
	}

	if (t->surface) cairo_surface_destroy(t->surface);
    t->surface = nullptr;
	std::future<void> foo = std::async(std::launch::async, startRecording);
	gtk_window_close(GTK_WINDOW(t->selectWindow));
	gtk_window_close(GTK_WINDOW(t->recordWindow));
	gtk_window_set_hide_on_close(GTK_WINDOW(t->window), false);
	gtk_window_present(GTK_WINDOW(t->window));
	g_print("Start recording button pressed\n");
}

static int handlePause(GtkWidget *widget, gpointer data) {
	std::future<void> foo = std::async(std::launch::async, pauseRecording);
	g_print("Pause button pressed\n");
	return 0;
}

static void handleStop(GtkWidget *widget, gpointer data) {
	std::future<void> foo = std::async(std::launch::async, stopRecording);
	g_print("Stop button pressed\n");
}

static void handleClose(GtkWidget *widget, gpointer data) {
	g_print("Close button pressed\n");

	// terminate capture if it's running
	if (t->s->is_capturing()){
        t->s->terminate();
	}

	if(!t->recordered){
		gtk_window_destroy(GTK_WINDOW(t->recordWindow));
		gtk_window_destroy(GTK_WINDOW(t->selectWindow));
	}
}

static void activate(GtkApplication *app, gpointer user_data) {
	// GtkWidget* buttonGrid;
	// GtkWidget* closeButton;
    if(t == nullptr) return;
    t->window = gtk_application_window_new(app);
    t->selectWindow = gtk_application_window_new(app);
    t->recordWindow = gtk_application_window_new(app);
    t->selectionArea = gtk_drawing_area_new();
    t->ready = false;
    t->started = false;
    t->selection_enabled = false;
//    gtk_window_fullscreen(GTK_WINDOW(selectWindow));
	// buttonGrid = gtk_grid_new();
    t->headerBar = gtk_header_bar_new();
    t->image = gtk_image_new_from_file("../assets/icon_small.png");
    t->title = gtk_text_buffer_new(NULL);
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(t->title), "  ", 2);
    t->titleView = gtk_text_view_new_with_buffer(t->title);
	gtk_window_set_title(GTK_WINDOW(t->window), "Screen recorder");
	gtk_window_set_default_size(GTK_WINDOW(t->window), 463, 50);
	gtk_window_set_default_size(GTK_WINDOW(t->recordWindow), 120, 30);
	gtk_widget_set_size_request(t->selectWindow, 2560, 1080);
	gtk_window_set_decorated(GTK_WINDOW(t->window), false);
	gtk_window_set_decorated(GTK_WINDOW(t->selectWindow), false);
	gtk_window_set_decorated(GTK_WINDOW(t->recordWindow), false);
	gtk_window_set_resizable(GTK_WINDOW(t->window), false);
	gtk_window_set_resizable(GTK_WINDOW(t->selectWindow), false);
	gtk_window_set_resizable(GTK_WINDOW(t->recordWindow), false);
    t->recordButton = gtk_button_new_with_label("Record");
    t->pauseButton = gtk_button_new_with_label("Pause");
    t->stopButton = gtk_button_new_with_label("Stop");
    t->startRecordButton = gtk_button_new_with_label("Start recording");

	auto _rec = g_signal_connect(t->recordButton, "clicked", G_CALLBACK(select_record_region), nullptr);
	auto recStart = g_signal_connect(t->startRecordButton, "clicked", G_CALLBACK(handleRecord), nullptr);
	auto p = g_signal_connect(t->pauseButton, "clicked", G_CALLBACK(handlePause), nullptr);
	auto s = g_signal_connect(t->stopButton, "clicked", G_CALLBACK(handleStop), nullptr);

	// gtk_window_set_child(GTK_WINDOW(window), buttonGrid);
	gtk_window_set_child(GTK_WINDOW(t->window), t->headerBar);
	gtk_window_set_child(GTK_WINDOW(t->recordWindow), t->startRecordButton);

	gtk_image_set_pixel_size(GTK_IMAGE(t->image), 32);
	gtk_header_bar_set_decoration_layout(GTK_HEADER_BAR(t->headerBar),":minimize,close");
	// gtk_header_bar_set_title_widget(GTK_HEADER_BAR(headerBar), titleView);
	gtk_header_bar_pack_start(GTK_HEADER_BAR(t->headerBar), t->image);
	// gtk_header_bar_pack_start(GTK_HEADER_BAR(headerBar), titleView);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(t->headerBar), t->stopButton);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(t->headerBar), t->pauseButton);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(t->headerBar), t->recordButton);

//    gtk_widget_set_hexpand(selectionArea, true);
//    gtk_widget_set_vexpand(selectionArea, true);
	gtk_window_set_child(GTK_WINDOW(t->selectWindow), t->selectionArea);
//    gtk_widget_action_set_enabled(selectionArea, "button_press_event", true);
    t->leftGesture = gtk_gesture_click_new();
    t->rightGesture = gtk_gesture_click_new();
//    moveGesture = gtk_gesture_drag_new();
	gtk_gesture_single_set_button(GTK_GESTURE_SINGLE (t->leftGesture), 1);
	gtk_gesture_single_set_button(GTK_GESTURE_SINGLE (t->rightGesture), 3);
    t->motionController = gtk_event_controller_motion_new();
//    g_signal_connect (selectWindow, "button-press-event",
//                      G_CALLBACK (deal_mouse_press), NULL);
	g_signal_connect (t->motionController, "motion",
	                  G_CALLBACK(motion_detected), t->selectionArea);
	g_signal_connect (t->leftGesture, "pressed",
	                  G_CALLBACK(left_btn_pressed), t->selectionArea);
	g_signal_connect (t->leftGesture, "released",
	                  G_CALLBACK(left_btn_released), t->selectionArea);
	g_signal_connect (t->rightGesture, "pressed",
	                  G_CALLBACK(right_btn_pressed), t->selectionArea);
	g_signal_connect (t->rightGesture, "released",
	                  G_CALLBACK(right_btn_released), t->selectionArea);

	gtk_widget_add_controller(t->selectionArea, GTK_EVENT_CONTROLLER (t->leftGesture));
	gtk_widget_add_controller(t->selectionArea, GTK_EVENT_CONTROLLER (t->rightGesture));
	gtk_widget_add_controller(t->selectionArea, GTK_EVENT_CONTROLLER (t->motionController));

//    gtk_widget_add_controller (selectionArea, GTK_EVENT_CONTROLLER (moveGesture));
	// gtk_header_bar_pack_start(GTK_HEADER_BAR(headerBar), closeButton);
	// gtk_grid_attach(GTK_GRID(buttonGrid), recordButton, 0, 0, 100, 50);
	// gtk_grid_insert_next_to(GTK_GRID(buttonGrid), recordButton, GTK_POS_RIGHT);
	// gtk_grid_attach_next_to(GTK_GRID(buttonGrid), pauseButton, recordButton,
	// GTK_POS_RIGHT, 100, 50); gtk_grid_attach_next_to(GTK_GRID(buttonGrid),
	// stopButton, pauseButton, GTK_POS_RIGHT, 100, 50);
	// gtk_grid_attach_next_to(GTK_GRID(buttonGrid), closeButton, stopButton,
	// GTK_POS_RIGHT, 100, 50);

	g_signal_connect(G_OBJECT(t->window), "destroy", G_CALLBACK(handleClose), nullptr);

	/* Initialize the surface to white */
//    clear_surface ();
//    g_signal_connect (selectWindow, "motion-notify-event",
//                      G_CALLBACK (deal_motion_notify_event), NULL);
	gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(t->selectionArea), draw, NULL, NULL);
//    g_signal_connect (G_OBJECT(selectionArea), "draw",
//                      G_CALLBACK(on_draw_event), NULL);
	gtk_window_present(GTK_WINDOW(t->window));
	gtk_window_set_hide_on_close(GTK_WINDOW(t->selectWindow), true);
	gtk_window_set_hide_on_close(GTK_WINDOW(t->recordWindow), true);
	gtk_widget_set_name(t->selectWindow, "selector_window");

    t->cssProvider = gtk_css_provider_new();
	gtk_css_provider_load_from_path(t->cssProvider, "../src/style.css");
    t->context = gtk_widget_get_style_context(t->selectWindow);
	gtk_style_context_add_provider(t->context,
	                               GTK_STYLE_PROVIDER(t->cssProvider),
	                               GTK_STYLE_PROVIDER_PRIORITY_USER); // I had used wrong priority on first try
	gtk_style_context_save(t->context);
//    gtk_widget_set_opacity(selectWindow, 0.70);
}

//int gtk_test(int argc, char **argv) {
//	GtkApplication *app;
//	int status;
//
//	app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
//	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
//	status = g_application_run(G_APPLICATION(app), argc, argv);
//	g_object_unref(app);
//
//	return status;
//}
//
//template<typename T>
//bool future_is_ready(std::future<T> &t) {
//	return t.wait_for(std::chrono::seconds(1)) == std::future_status::ready;
//}
//
//void pauseTest() {
//	int secondsPause = 2;
//	int secondsResume = 1;
//	std::this_thread::sleep_for(std::chrono::seconds(secondsPause));
//	s->pause();
//	std::cout << "Capture paused after " << secondsPause << " seconds" << std::endl;
//	std::this_thread::sleep_for(std::chrono::seconds(secondsResume));
//	s->resume();
//	std::cout << "Capture resumed after " << secondsResume << " seconds" << std::endl;
//}

