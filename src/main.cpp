#include <iostream>

#include "Recorder.h"
#include "ScreenRecorder.h"

void print_all_wrap(){
	cout << "---------------" << endl;

	wrapper<AVFormatContext>::print_count();
	wrapper<AVInputFormat>::print_count();
	wrapper<AVStream>::print_count();
	wrapper<AVCodec>::print_count();
	wrapper<AVCodecParameters>::print_count();
	wrapper<AVCodecContext>::print_count();
	wrapper<AVOutputFormat>::print_count();
	wrapper<AVDictionary>::print_count();

}

void prova(){
	auto s = Recorder{};
	s.init();
	cout << "end of init\n" << endl;
	print_all_wrap();

}

int main(int argc, char **argv) {
	prova();
	print_all_wrap();


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
	return 0;
}
