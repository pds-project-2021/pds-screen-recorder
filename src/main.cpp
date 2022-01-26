#include <iostream>

#include "Recorder.h"
#include "ui/include/Interface.h"

// todo: this is only for sleep() function, remove it for release
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

//void print_all_wrap(){
//	cout << "---------------" << endl;
//
//	wrapper<AVFormatContext>::print_count();
//	wrapper<AVInputFormat>::print_count();
//	wrapper<AVCodec>::print_count();
//	wrapper<AVCodecParameters>::print_count();
//	wrapper<AVCodecContext>::print_count();
//	wrapper<AVOutputFormat>::print_count();
//	wrapper<AVDictionary>::print_count();
//	Rescaler::print_count();
//	wrapper<AVStream>::print_count();
//}
//
//void prova(){
//	auto rec = Recorder{};
//	auto screen = Screen{};
//
//	screen.set_dimension("1920x1080");
////	screen.set_dimension(720, 480);
//	screen.set_offset("0x0");
////	screen.set_offset(100,0);
//
//	rec.init(screen);
//	cout << "end of init\n" << endl;
//
//	rec.capture();
////	cout << "start of capture\n" << endl;
////
////	sleep(5);
////	cout << "pausing for 5 seconds" << endl;
////	rec.pause();
////
////	auto cnt = 5;
////	while(cnt){
////		sleep(1);
////		cout << "resume in " << cnt-- << " seconds" << endl;
////	}
////	rec.resume();
////	cout << "resumed capture" << endl;
////
//	sleep(5);
//	rec.terminate();
//	cout << "end of capture\n" << endl;
//
//	print_all_wrap();
//	sleep(5);
//
//}


int main(int argc, char **argv) {
    Interface t = Interface();
	auto res = t.launchUI(argc, argv);

	return res;

//	prova();
//	print_all_wrap();
//	cout << "prova 2" << endl;
//	prova();
//	print_all_wrap();


//	return 0;
}

