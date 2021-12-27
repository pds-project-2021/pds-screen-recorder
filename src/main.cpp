#include <iostream>

#include "Recorder.h"

// todo: this is only for sleep() function, remove it for release
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

void prova(){
	auto rec = Recorder{};
	auto screen = Screen{};

	rec.init(screen);
	cout << "end of init\n" << endl;

	rec.capture();
	cout << "start of capture\n" << endl;

	sleep(5);
	cout << "pausing for 5 seconds" << endl;
	rec.pause();

	auto cnt = 5;
	while(cnt){
		sleep(1);
		cout << "resume in " << cnt-- << " seconds" << endl;
	}
	rec.resume();
	cout << "resumed capture" << endl;

	sleep(5);
	rec.terminate();
	cout << "end of capture\n" << endl;
}

// template<typename T>
// bool future_is_ready(std::future<T>& t){
//     return t.wait_for(std::chrono::seconds(1)) == std::future_status::ready;
// }

// void pauseTest() {
//     int secondsPause = 2;
//     int secondsResume = 1;
//     std::this_thread::sleep_for(std::chrono::seconds(secondsPause));
//     s->PauseCapture();
//     std::cout << "Capture paused after " << secondsPause << " seconds" << std::endl;
//     std::this_thread::sleep_for(std::chrono::seconds(secondsResume));
//     s->ResumeCapture();
//     std::cout << "Capture resumed after " << secondsResume << " seconds" << std::endl;
// }

int main(int argc, char **argv) {
	prova();

	return 0;


//	auto s = ScreenRecorder();
//	s.init();
//	std::cout << "Initialized input streams" << std::endl;
//	s.init_outputfile();
//	std::cout << "Initialized output streams and file" << std::endl;
//	if (s.CaptureStart() >= 0) {
//		std::cout << "Capture started" << std::endl;
//		s.CloseMediaFile();
//		std::cout << "Capture complete" << std::endl;
//	}
  
// 	//  return gtk_test(argc, argv);
//     GtkApplication* app;
//     int status;
//     s = new ScreenRecorder();
//     //if (future_is_ready(foo)){
//     //    return 0;
//     //}
// //    auto s = ScreenRecorder();
// //    s.init();
// //    std::cout << "Initialized input streams" << std::endl;
// //    s.init_outputfile();
// //    std::cout << "Initialized output streams and file" << std::endl;
// //    if (s.CaptureStart() >= 0) {
// //        int millisecondsPause = 2000;
// //        int millisecondsResume = 2000;
// //        int millisecondsStop = 3000;
// //        std::cout << "Capture started" << std::endl;
// //        std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsPause));
// //        s.PauseCapture();
// //        std::cout << "Capture paused after " << ((double)millisecondsPause/1000) << " seconds" << std::endl;
// //        std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsResume));
// //        s.ResumeCapture();
// //        std::cout << "Capture resumed after " << ((double)millisecondsResume/1000) << " seconds \n" << std::endl;
// //        std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsStop));
// //        s.CloseMediaFile();
// //        std::cout << "Capture complete after " << ((double)millisecondsStop/1000) << " seconds \n" << std::endl;
// //    }
// //    s.init();
// //    std::cout << "Initialized input streams" << std::endl;
// //    s.init_outputfile();
// //    std::cout << "Initialized output streams and file" << std::endl;
//     app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
//     g_signal_connect(app, "activate", G_CALLBACK(activate), nullptr);
//     status = g_application_run(G_APPLICATION(app), argc, argv);
//     g_object_unref(app);

//     return status;
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
