//
// Created by gabriele on 31/10/21.
//

#include "include/Stream.h"


Stream::Stream(const Format &format, const Codec &codec) {
	video = avformat_new_stream(format.outputContext.get_video(), codec.output.get_video());
	if (!video) {
		throw avException("Error in creating a av format new video stream");
	}
	audio = avformat_new_stream(format.outputContext.get_audio(), codec.output.get_audio());
	if (!audio) {
		throw avException("Error in creating a av format new audio stream");
	}
}
