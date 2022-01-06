#include <iostream>

#include "Recorder.h"
#include "ui/include/Interface.h"

// todo: this is only for sleep() function, remove it for release
//#ifdef _WIN32
//#include <Windows.h>
//#else
//#include <unistd.h>
//#endif
//
//
//void prova(){
//	auto rec = Recorder{};
//	auto screen = Screen{};
//
////	screen.set_dimension("1920x1080");
//	screen.set_dimension(720, 480);
////	screen.set_offset("0x0");
//	screen.set_offset(100,0);
//
//	rec.init(screen);
//	cout << "end of init\n" << endl;
//
//	rec.capture();
//	cout << "start of capture\n" << endl;
//
//	sleep(5);
//	cout << "pausing for 5 seconds" << endl;
//	rec.pause();
//
//	auto cnt = 5;
//	while(cnt){
//		sleep(1);
//		cout << "resume in " << cnt-- << " seconds" << endl;
//	}
//	rec.resume();
//	cout << "resumed capture" << endl;
//
//	sleep(5);
//	rec.terminate();
//	cout << "end of capture\n" << endl;
//}


int main(int argc, char **argv) {
	auto rec = Recorder{};
	auto res = launchUI(argc, argv, &rec);

	return res;
}

