//
// Created by gabriele on 23/12/21.
//

#include "include/utils.h"
#include "ffmpeg.h"

using namespace std;

bool is_file(char *url){
	auto str = string {url};
	return str.find(".mp4") != string::npos;
}
