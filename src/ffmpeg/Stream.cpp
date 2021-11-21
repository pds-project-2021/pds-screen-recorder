//
// Created by gabriele on 31/10/21.
//

#include "include/Stream.h"


Stream::Stream(const Format &format, const Codec &codec) {
	this->audio = avformat_new_stream(format.outputContext.get_audio(), codec.output.get_audio());
	this->video = avformat_new_stream(format.outputContext.get_video(), codec.output.get_video());
}
