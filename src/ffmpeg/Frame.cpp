//
// Created by gabriele on 23/12/21.
//

#include "Frame.h"

/**
 * Constructor for video frames
 *
 * @param width
 * @param height
 * @param format
 * @param align
 */
Frame::Frame(int width, int height, AVPixelFormat format, int align) {
	inner = av_frame_alloc(); // allocate memory for frame structure
	if (!inner) {
		throw avException("Unable to release the avframe resources");
	}

	// fill frame fields
	inner->data[0] = nullptr;
	inner->width = width;
	inner->height = height;
	inner->format = format;
	inner->pts = 0;

	// Setup the data pointers and line sizes based on the specified image
	// parameters and the provided array.
	// allocate data fields
	auto res = av_image_alloc(inner->data, inner->linesize, width, height, format, align);
	if (res < 0) {
		throw avException("Error in allocating frame data");
	}
}

/**
 * Constructor for audio frames
 *
 * @param nb_samples
 * @param format
 * @param channel_layout
 * @param align
 */
Frame::Frame(int nb_samples, AVSampleFormat format, uint64_t channel_layout, int align) {
	inner = av_frame_alloc();
	if (!inner) {
		throw avException("Unable to release the audio avframe resources");
	}

	inner->nb_samples = nb_samples;
	inner->format = format;
	inner->channel_layout = channel_layout;

	if (av_frame_get_buffer(inner, align) < 0) {
		throw avException("Could not allocate audio data buffers");
	}

}

Frame::Frame() {
	inner = av_frame_alloc();
	if (!inner) {
		throw avException("Unable to release the audio avframe resources");
	}
}






