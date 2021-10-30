#include "ScreenRecorder.h"
using namespace std;

#define AUDIO_CHANNELS 1
#define AUDIO_SAMPLE_RATE 44100
#ifdef WIN32
#define VIDEO_CODEC 27 //H264
#else
#define VIDEO_CODEC 2 //MPEG2
#endif
#define VIDEO_BITRATE 8000000
#define FRAME_COUNT 60
#define AUDIO_CODEC 86017 //86017 MP3; 86018 AAC;
#define AUDIO_BITRATE 128000
std::mutex aD;
std::mutex aW;
std::condition_variable audioCnv;
std::condition_variable audioWrt;
/* initialize the resources*/
ScreenRecorder::ScreenRecorder() {
	av_register_all();
	avcodec_register_all();
	avdevice_register_all();
	recordVideo = false;
    frameCount = 0;
}

ScreenRecorder::~ScreenRecorder() {
	avformat_close_input(&inputFormatContext);
	if (inputFormatContext) {
		throw avException("Unable to close input");
	}

	avformat_free_context(inputFormatContext);
	if (inputFormatContext) {
		throw avException("Unable to free avformat context");
	}

	cout << "clean all" << endl;
}

void show_avfoundation_device() {
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	AVDictionary *options = NULL;
	av_dict_set(&options, "list_devices", "true", 0);
	AVInputFormat *iformat = av_find_input_format("avfoundation");
	printf("==AVFoundation Device Info===\n");
	avformat_open_input(&pFormatCtx, "", iformat, &options);
	printf("=============================\n");
}

int ScreenRecorder::init() {
	inputFormatContext = avformat_alloc_context(); // Allocate an AVFormatContext.
	audioInputFormatContext = avformat_alloc_context(); // Allocate an AVFormatContext.
	options = nullptr;

#ifdef _WIN32
	CoInitializeEx(NULL, COINIT_MULTITHREADED); // Set COM to multithreaded model
	av_dict_set(&options, "rtbufsize", "10M", 0);
	audioInputFormat = av_find_input_format("dshow");
    av_dict_set(&options, "sample_rate", to_string(AUDIO_SAMPLE_RATE).c_str(), 0);
    av_dict_set(&options, "channels", to_string(AUDIO_CHANNELS).c_str(), 0);
	auto ret = avformat_open_input(&audioInputFormatContext,
							"audio=Microfono (TONOR TC30 Audio Device)", audioInputFormat,
							&options);
	if (ret != 0) {
	  throw avException("Couldn't open input stream");
	}
	options = nullptr;
	av_dict_set(&options, "framerate", "30", 0);
	//av_dict_set(&options, "preset", "medium", 0);
	av_dict_set(&options, "offset_x", "0", 0);
	av_dict_set(&options, "offset_y", "0", 0);
	// av_dict_set(&options, "video_size", "1920x1080", 0);
	av_dict_set(&options, "show_region", "1", 0);
	inputFormat = av_find_input_format("gdigrab");
	ret = avformat_open_input(&inputFormatContext, "desktop", inputFormat, &options);
	if (ret != 0) {
	  throw avException("Couldn't open input stream");
	}
#elif defined linux
	//av_dict_set(&options, "rtbufsize", "10M", 0);
	audioInputFormat = av_find_input_format("pulse");
	auto ret = avformat_open_input(&audioInputFormatContext, "default", audioInputFormat, &options);
	if (ret != 0) {
		throw avException("Couldn't open input stream");
	}
	options = nullptr;
	av_dict_set(&options, "framerate", "30", 0);
	av_dict_set(&options, "preset", "medium", 0);
	av_dict_set(&options, "offset_x", "0", 0);
	av_dict_set(&options, "offset_y", "0", 0);
//	av_dict_set(&options, "video_size", "1920x1080", 0);
	av_dict_set(&options, "show_region", "1", 0);

	inputFormat = av_find_input_format("x11grab");
	ret = avformat_open_input(&inputFormatContext, ":0.0", inputFormat, &options);
    if (ret != 0) {
        throw avException("Couldn't open input stream");
    }
#else
	show_avfoundation_device();
	inputFormat = av_find_input_format("avfoundation");
	auto ret = avformat_open_input(&inputFormatContext, "1", inputFormat, nullptr);
	if (ret != 0) {
	  throw avException("Couldn't open input stream");
	}
	audioInputFormat = av_find_input_format("avfoundation");
	ret = avformat_open_input(&inputFormatContext, ":0", inputFormat, nullptr);
	if (ret != 0) {
	    throw avException("Couldn't open input stream");
	}

#endif

	ret = avformat_find_stream_info(inputFormatContext, &options);
	if (ret < 0) {
		throw avException("Unable to find the video stream information");
	}

	ret = avformat_find_stream_info(audioInputFormatContext, nullptr);
	if (ret < 0) {
		throw avException("Unable to find the audio stream information");
	}

	auto index = av_find_best_stream(inputFormatContext, AVMEDIA_TYPE_VIDEO, -1,
	                                 -1, nullptr, 0);
	if (index == -1) {
		throw avException("Unable to find the video stream index. (-1)");
	}

	auto audioIndex = av_find_best_stream(audioInputFormatContext,
	                                      AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
	if (audioIndex == -1) {
		throw avException("Unable to find the audio stream index. (-1)");
	}

	inputCodecPar = inputFormatContext->streams[index]->codecpar;
	//inputCodecPar->format = AV_PIX_FMT_BGR0;

	audioInputCodecPar = audioInputFormatContext->streams[audioIndex]->codecpar;
	audioInputCodecPar->format = AV_SAMPLE_FMT_S16;
	audioInputCodecPar->sample_rate = AUDIO_SAMPLE_RATE;
	audioInputCodecPar->channel_layout = AUDIO_CHANNELS==2?AV_CH_LAYOUT_STEREO:AV_CH_LAYOUT_MONO;
	audioInputCodecPar->channels = AUDIO_CHANNELS;
	audioInputCodecPar->codec_id = AV_CODEC_ID_PCM_S16LE;
	audioInputCodecPar->codec_type = AVMEDIA_TYPE_AUDIO;
	audioInputCodecPar->frame_size = 22050; // set number of audio samples in each frame

	inputCodec = avcodec_find_decoder(inputCodecPar->codec_id);
	if (inputCodec == nullptr) {
		throw avException("Unable to find the video decoder");
	}

	audioInputCodec = avcodec_find_decoder(audioInputCodecPar->codec_id);
	if (audioInputCodec == nullptr) {
		throw avException("Unable to find the audio decoder");
	}

	inputCodecContext = avcodec_alloc_context3(inputCodec);
	if (inputCodecContext == nullptr) {
		throw avException("Unable to get input video codec context");
	}
	avcodec_parameters_to_context(inputCodecContext, inputCodecPar);

	audioInputCodecContext = avcodec_alloc_context3(audioInputCodec);
	if (audioInputCodecContext == nullptr) {
		throw avException("Unable to get input audio codec context");
	}
	avcodec_parameters_to_context(audioInputCodecContext, audioInputCodecPar);

	// Initialize the AVCodecContext to use the given video AVCodec.
	ret = avcodec_open2(inputCodecContext, inputCodec, nullptr);
	if (ret < 0) {
		throw avException("Unable to open the video av codec");
	}

	// Initialize the AVCodecContext to use the given audio AVCodec.
	ret = avcodec_open2(audioInputCodecContext, audioInputCodec, nullptr);
	if (ret < 0) {
		throw avException("Unable to open the audio av codec");
	}

#ifdef _WIN32
	av_dump_format(inputFormatContext, 0, "desktop", 0);
	av_dump_format(audioInputFormatContext, 1, "dshow-audio-device", 0);
#elif defined linux
	av_dump_format(inputFormatContext, 0, ":0.0+0,0", 0);
	av_dump_format(audioInputFormatContext, 0, "default", 0);
#else
	av_dump_format(inputFormatContext, 0, "1", 0);
#endif
	return 0;
}

int ScreenRecorder::init_outputfile() {
	output_file = "../media/output.mp4";
    frameCount = FRAME_COUNT;
	outputCodec = avcodec_find_encoder((AVCodecID) VIDEO_CODEC);
	if (!outputCodec) {
		throw avException(
			"Error in finding the video av codecs. try again with correct codec");
	}
	audioOutputCodec = avcodec_find_encoder((AVCodecID) AUDIO_CODEC);
	if (!audioOutputCodec) {
		throw avException(
			"Error in finding the audio av codecs. try again with correct codec");
	}
	avformat_alloc_output_context2(&outputFormatContext, nullptr, nullptr,
	                               output_file);
	if (!outputFormatContext) {
		throw avException("Error in allocating av format output context");
	}

	videoStream = avformat_new_stream(outputFormatContext, outputCodec);
	if (!videoStream) {
		throw avException("Error in creating a av format new video stream");
	}
	audioStream = avformat_new_stream(outputFormatContext, audioOutputCodec);
	if (!audioStream) {
		throw avException("Error in creating a av format new audio stream");
	}
	videoStream->time_base = {1, 30};
	audioStream->time_base = {1, audioInputCodecContext->sample_rate};
	/* Returns the output format in the list of registered output formats which
	 * best matches the provided parameters, or returns nullptr if there is no
	 * match.
	 */
	outputFormat = av_guess_format(nullptr, output_file, nullptr);
	if (!outputFormat) {
		throw avException(
			"Error in guessing the video format. try with correct format");
	}

	outputCodecContext = avcodec_alloc_context3(outputCodec);
	if (!outputCodecContext) {
		throw avException("Error in allocating the video codec context");
	}
	audioOutputCodecContext = avcodec_alloc_context3(audioOutputCodec);
	if (!audioOutputCodecContext) {
		throw avException("Error in allocating the audio codec context");
	}
	outputCodecContext->gop_size = 10;
	outputCodecContext->max_b_frames = 5;
	outputCodecContext->time_base = videoStream->time_base;

	/* set property of the video file */
	outputCodecPar = videoStream->codecpar;
	outputCodecPar->codec_id = (AVCodecID) VIDEO_CODEC; // AV_CODEC_ID_MPEG4; AV_CODEC_ID_H264; // AV_CODEC_ID_MPEG1VIDEO; // AV_CODEC_ID_MPEG2VIDEO;
	outputCodecPar->codec_type = AVMEDIA_TYPE_VIDEO;
	outputCodecPar->format = AV_PIX_FMT_YUV420P;
	outputCodecPar->bit_rate = VIDEO_BITRATE; // 2500000
	outputCodecPar->width = inputCodecContext->width;
	outputCodecPar->height = inputCodecContext->height;

	audioOutputCodecPar = audioStream->codecpar;
	audioOutputCodecPar->codec_id = (AVCodecID) AUDIO_CODEC;
	audioOutputCodecPar->codec_type = AVMEDIA_TYPE_AUDIO;
	audioOutputCodecPar->bit_rate = AUDIO_BITRATE;
	audioOutputCodecPar->channels = audioInputCodecContext->channels;
	audioOutputCodecPar->channel_layout = audioInputCodecContext->channel_layout;
	audioOutputCodecPar->sample_rate = audioInputCodecContext->sample_rate;
	audioOutputCodecPar->format = audioOutputCodec->sample_fmts[0];
	audioOutputCodecPar->frame_size = 22050;
	auto ret = avcodec_parameters_to_context(outputCodecContext, outputCodecPar);
	if (ret < 0) {
		throw avException("Unable to get output codec context");
	}
	ret = avcodec_parameters_to_context(audioOutputCodecContext, audioOutputCodecPar);
	if (ret < 0) {
		throw avException("Unable to get output codec context");
	}

	/* Some container formats (like MP4) require global headers to be present
  Mark the encoder so that it behaves accordingly. */

	if (outputFormatContext->oformat->flags & AVFMT_GLOBALHEADER) {
		outputCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	if (codec_id == AV_CODEC_ID_H264) {
		av_opt_set(outputCodecContext->priv_data, "preset", "slow", 0);
	}

	ret = avcodec_open2(outputCodecContext, outputCodec, nullptr);
	if (ret < 0) {
		throw avException("Error in opening the video avcodec");
	}

	ret = avcodec_open2(audioOutputCodecContext, audioOutputCodec, nullptr);
	if (ret < 0) {
		throw avException("Error in opening the audio avcodec");
	}

	/* create empty video file */
	if (!(outputFormatContext->flags & AVFMT_NOFILE)) {
		ret = avio_open2(&(outputFormatContext->pb), output_file, AVIO_FLAG_WRITE,
		                 nullptr, nullptr);
		if (ret < 0) {
			char buf[35];
			av_strerror(ret, buf, sizeof(buf));
			throw avException(buf);
		}
	}

//	if (!outputFormatContext->nb_streams) {
//		throw avException("Output file dose not contain any stream");
//	}
//	audioInputCodecContext->frame_size = audioOutputCodecContext->frame_size;
	swsContext = sws_getCachedContext(
		swsContext, inputCodecPar->width, inputCodecPar->height,
		(AVPixelFormat) inputCodecPar->format, outputCodecPar->width,
		outputCodecPar->height, (AVPixelFormat) outputCodecPar->format,
		SWS_BICUBIC, nullptr, nullptr, nullptr);
	if (!swsContext) {
		throw avException(
			"Impossible to create scale context for video conversion");
	}
	swrContext = swr_alloc();
	swrContext = swr_alloc_set_opts(
		swrContext, (int64_t) audioOutputCodecContext->channel_layout,
		audioOutputCodecContext->sample_fmt, audioOutputCodecContext->sample_rate,
		(int64_t) audioInputCodecContext->channel_layout,
		audioInputCodecContext->sample_fmt, audioInputCodecContext->sample_rate,
		0, nullptr);
	if (!swrContext) {
		throw avException(
			"Impossible to create resample context for audio conversion");
	}
	if (swr_init(swrContext) < 0) {
		throw avException(
			"Impossible to initialize resample context for audio conversion");
	}
	/* imp: mp4 container or some advanced container file required header
	 * information*/
	ret = avformat_write_header(outputFormatContext, &options);
	if (ret < 0) {
		throw avException("Error in writing the header context");
	}

	// file informations
	av_dump_format(outputFormatContext, 0, output_file, 1);

	return 0;
}

/* function to capture and store data in frames by allocating required memory
 * and auto deallocating the memory.   */

AVFrame *alloc_video_frame(int width, int height, AVPixelFormat format, int align) {
	AVFrame *frame = av_frame_alloc(); // allocate memory for frame structure
	if (!frame) {
		throw avException("Unable to release the avframe resources");
	}
	// fill frame fields
	frame->data[0] = nullptr;
	frame->width = width;
	frame->height = height;
	frame->format = format;
	frame->pts = 0;
	// Setup the data pointers and linesizes based on the specified image
	// parameters and the provided array.
	// allocate data fields
	if (av_image_alloc(frame->data, frame->linesize, width, height,(AVPixelFormat) format, align) < 0) {
		throw avException("Error in allocating frame data");
	}
	return frame;
}

AVFrame *alloc_audio_frame(int nb_samples, AVSampleFormat format, uint64_t channel_layout, int align) {
	AVFrame *audioFrame = av_frame_alloc();
	if (!audioFrame) {
		throw avException("Unable to release the audio avframe resources");
	}
	audioFrame->nb_samples = nb_samples;
	audioFrame->format = format;
	audioFrame->channel_layout = channel_layout;
	if (av_frame_get_buffer(audioFrame, align) < 0) {
		throw avException("Could not allocate audio data buffers");
	}
	return audioFrame;
}

AVPacket *alloc_packet() {
	AVPacket *packet = av_packet_alloc();
	if (!packet) {
		throw avException("Error on packet initialization");
	}
	return packet;
}

static int decode(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt) {
	int result;
	*got_frame = 0;
	if (pkt) {
		// Send packet to decoder
		result = avcodec_send_packet(avctx, pkt);
		// In particular, we don't expect AVERROR(EAGAIN), because we read all
		// decoded frames with avcodec_receive_frame() until done.
		// Check result
		if (result < 0 && result != AVERROR_EOF) {
			// Decoder error
			throw avException("Failed to send packet to decoder");
			return result;
		} else if (result >= 0) {
			result = avcodec_receive_frame(avctx, frame); // Try to get a decoded
			// frame
			if (result < 0 && result != AVERROR(EAGAIN) && result != AVERROR_EOF)
				return result;
			else if (result >= 0) {
				*got_frame = 1;
				return 0;
			}
		}
	}
	return -1;
}

static int encode(AVCodecContext *avctx, AVPacket *pkt, int *got_packet, AVFrame *frame) {
	int result;
	*got_packet = 0;
	// Send frame to encoder
	result = avcodec_send_frame(avctx, frame);
	if (result < 0) {
		if (result == AVERROR(EAGAIN)) { // Buffer is full
			int ret;
			while (result == AVERROR(EAGAIN)) { // while encoder buffer is full
				ret = avcodec_receive_packet(avctx, pkt); // Try to receive packet
				result = avcodec_send_frame(avctx, frame);
			}
			result = ret;
		} else {
			throw avException("Failed to send frame to encoder"); // Error ending frame to encoder
		}
	} else {
		result = avcodec_receive_packet(avctx, pkt); // Try to receive packet
	}
	if (result >= 0) {
		*got_packet = 1;
		return result;
	} else if (result == AVERROR(EAGAIN))
		return 0;
	else {
		throw avException("Failed to receive frame from encoder"); // Error ending frame to encoder
	}
}

static int add_samples_to_fifo(AVAudioFifo *fifo, uint8_t **converted_input_samples, const int frame_size) {
//  Make the FIFO as large as it needs to be to hold both the old and the new samples.
    if (av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + frame_size) < 0) {
        throw avException("Could not reallocate FIFO");
    }

    /* Store the new samples in the FIFO buffer. */
    if (av_audio_fifo_write(fifo, (void **)converted_input_samples, frame_size) < frame_size) {
        throw avException("Could not write data to FIFO");
    }
    return 0;
}

static int init_converted_samples(uint8_t ***converted_input_samples, AVCodecContext *output_codec_context, int frame_size)
{
    int error;
    if (!(*converted_input_samples = static_cast<uint8_t **>(calloc(output_codec_context->channels,
                                                                    sizeof(**converted_input_samples))))) {
        throw avException("Could not allocate converted input sample pointers");
        return AVERROR(ENOMEM);
    }

    /* Allocate memory for the samples of all channels in one consecutive
     * block for convenience. */
    if ((error = av_samples_alloc(*converted_input_samples, NULL,
                                  output_codec_context->channels,
                                  frame_size,
                                  output_codec_context->sample_fmt, 0)) < 0) {
        av_freep(&(*converted_input_samples)[0]);
        free(*converted_input_samples);
        throw avException("Could not allocate converted input samples (error '%s')");
        return error;
    }
    return 0;
}

void ScreenRecorder::VideoDemuxing() {}

int ScreenRecorder::CloseMediaFile() {
    video->join();
    audio->join();
//    audioDemux->join();
//    unique_lock<mutex> ulC(aD);
//    finishedAudioDemux = true;
//    audioCnv.notify_one();// Send sync signal to converter thread
//    audioCnv.wait(ulC);// Wait for resume signal
//    audioCnv.notify_one();// Send sync signal to converter thread
//    audioConvert->join();
//    unique_lock<mutex> ulW(aW);
//    finishedAudioConversion = true;
//    audioWrt.notify_one();// Send sync signal to output writer thread if necessary
//    audioWrt.wait(ulW);//Wait for writer thread signal
//    audioWrt.notify_one();// Send sync signal to output writer thread if necessary
//    audioWrite->join();
    //Write video file trailer data
    auto ret = av_write_trailer(outputFormatContext);
    if (ret < 0) {
        throw avException("Error in writing av trailer");
    }
    if (!(outputFormatContext->flags & AVFMT_NOFILE)) {
        int err = avio_close(outputFormatContext->pb);
        if (err < 0) {
            throw fsException("Failed to close file");
        }
    }
}

int ScreenRecorder::initThreads() {
	video = new thread(&ScreenRecorder::CaptureVideoFrames, this);
    audio = new thread(&ScreenRecorder::CaptureAudioFrames, this);
//    finishedAudioDemux = false;
//    audioDemux = new thread(&ScreenRecorder::DemuxAudioInput, this);
//    finishedAudioConversion = false;
//    audioConvert = new thread(&ScreenRecorder::ConvertAudioFrames, this);
//    AVRational audiobq = {1, audioInputCodecContext->sample_rate * audioInputCodecContext->channels};
//    audioWrite = new thread(&ScreenRecorder::WriteAudioOutput, this, outputFormatContext, audiobq, audioStream->time_base);
	return 0;
}

int ScreenRecorder::CaptureStart() { return initThreads(); }

void ScreenRecorder::CaptureVideoFrames() {
	// video thread started
	recordVideo = true;
	// Create decoder frame
	AVFrame *frame =
		alloc_video_frame(inputCodecPar->width, inputCodecPar->height,
		                  (AVPixelFormat) inputCodecPar->format, 32);
	// Create encoder frame
	AVFrame *outputFrame =
		alloc_video_frame(outputCodecPar->width, outputCodecPar->height,
		                  (AVPixelFormat) outputCodecPar->format, 32);
	// init cycle variables
	int count = 0;
	int frameNum = 0; // frame number in a second
	int got_frame = 0;
	int got_packet = 0;

	// Create decoder packet
	AVPacket *packet = alloc_packet();
	// Create encoder packet
	AVPacket *outPacket = alloc_packet();

	// Try to extract packet from input stream
	while (av_read_frame(inputFormatContext, packet) >= 0) {
		if (count++ == frameCount) {
			break;
		}
		if (frameNum++ == 30)
			frameNum = 0; // reset every fps frames

		if (packet->stream_index == videoStream->index) {
			// Send packet to decoder
			decode(inputCodecContext, frame, &got_frame, packet);

			// check if decoded frame is ready
			if (got_frame) { // frame is ready
				// Convert frame picture format
				sws_scale(swsContext, frame->data, frame->linesize, 0,
				          inputCodecContext->height, outputFrame->data,
				          outputFrame->linesize);
				// Send converted frame to encoder
				outputFrame->pts = count - 1;
				encode(outputCodecContext, outPacket, &got_packet, outputFrame);
				// Frame was sent successfully
				if (got_packet > 0) { // Packet received successfully
					if (outPacket->pts != AV_NOPTS_VALUE) {
						outPacket->pts =
							av_rescale_q(outPacket->pts, outputCodecContext->time_base,
							             videoStream->time_base);
					}
					if (outPacket->dts != AV_NOPTS_VALUE) {
						outPacket->dts =
							av_rescale_q(outPacket->dts, outputCodecContext->time_base,
							             videoStream->time_base);
					}
					// Write packet to file
					auto result = av_write_frame(outputFormatContext, outPacket);
					if (result != 0) {
						throw avException("Error in writing video frame");
					}
				}
				av_packet_unref(outPacket);
			}
		}
	} // End of while-loop

	// Handle delayed frames
	for (int result;;) {
		// avcodec_send_frame(outputCodecContext, NULL);
		if (avcodec_receive_packet(outputCodecContext, outPacket) == 0) { // Try to get packet
			if (outPacket->pts != AV_NOPTS_VALUE) {
				outPacket->pts =
					av_rescale_q(outPacket->pts, outputCodecContext->time_base,
					             videoStream->time_base);
			}
			if (outPacket->dts != AV_NOPTS_VALUE) {
				outPacket->dts =
					av_rescale_q(outPacket->dts, outputCodecContext->time_base,
					             videoStream->time_base);
			}
            outPacket->duration = av_rescale_q(1, outputCodecContext->time_base,
                                                videoStream->time_base);
			result = av_write_frame(outputFormatContext,
			                        outPacket); // Write packet to file
			if (result != 0) {
				throw avException("Error in writing video frame");
			}
			av_packet_unref(outPacket);
		} else { // No remaining frames to handle
			break;
		}
	}
    av_free(frame);
    av_free(outputFrame);
    av_free(packet);
    av_free(outPacket);
}

void ScreenRecorder::CaptureAudioFrames() {
	// audio thread started
	// Create decoder audio frame
	AVFrame *audioFrame =
		alloc_audio_frame( 22050, audioInputCodecContext->sample_fmt,
		                  audioInputCodecContext->channel_layout, 0);
	// Create encoder audio frame
	AVFrame *audioOutputFrame = alloc_audio_frame(
		audioOutputCodecContext->frame_size, audioOutputCodecContext->sample_fmt,
		audioOutputCodecContext->channel_layout, 0);
	// Create decoder audio packet
	AVPacket *audioPacket = alloc_packet();
	// Create encoder audio packet
	AVPacket *audioOutputPacket = alloc_packet();
	int count = 0;
	int audioCount = ((int) frameCount/30*4)+1;
	int audioFrameNum = 0;
	int got_frame = 0;
	int got_packet = 0;
    uint8_t ** audio_samples = NULL;
    int64_t pts = 0;
    int dts = 0;
	// Handle audio input stream packets
	while (av_read_frame(audioInputFormatContext, audioPacket) >= 0) {
		if (count++ == audioCount) {
			break;
		}
		// Send packet to decoder
		decode(audioInputCodecContext, audioFrame, &got_frame, audioPacket);
		// check if decoded frame is ready
		if (got_frame > 0) { // frame is ready
			// Convert frame sample format
			int got_samples = swr_convert(
				swrContext, audioOutputFrame->data, audioOutputFrame->nb_samples,
				(const uint8_t **) audioFrame->data, audioFrame->nb_samples);
			if (got_samples < 0) {
				fprintf(stderr, "error: swr_convert()\n");
				exit(1);
			}
            else if (got_samples > 0) {
                pts += audioOutputFrame->nb_samples;
				audioOutputFrame->pts = pts;
				encode(audioOutputCodecContext, audioOutputPacket, &got_packet,
				       audioOutputFrame);
				// Frame was sent successfully
				if (got_packet > 0) { // Packet received successfully
					if (audioOutputPacket->pts != AV_NOPTS_VALUE) {
						audioOutputPacket->pts =
							av_rescale_q(audioOutputPacket->pts,
							             {1,
								             audioInputCodecContext->sample_rate *
									             audioInputCodecContext->channels},
							             audioStream->time_base);
					}
					if (audioOutputPacket->dts != AV_NOPTS_VALUE) {
						audioOutputPacket->dts =
							av_rescale_q(audioOutputPacket->dts,
							             {1,
								             audioInputCodecContext->sample_rate *
									             audioInputCodecContext->channels},
							             audioStream->time_base);
					}
					// Write packet to file
					audioOutputPacket->stream_index = 1;
					auto result = av_write_frame(outputFormatContext, audioOutputPacket);
					if (result != 0) {
						throw avException("Error in writing video frame");
					}
				}
				av_packet_unref(audioOutputPacket);
			}
			while (got_samples > 0 ) {
				got_samples = swr_convert(swrContext, audioOutputFrame->data, audioOutputFrame->nb_samples, nullptr, 0);
                pts += audioOutputFrame->nb_samples;
                audioOutputFrame->pts = pts;
				encode(audioOutputCodecContext, audioOutputPacket, &got_packet,
				       audioOutputFrame);
				// Frame was sent successfully
				if (got_packet > 0) { // Packet received successfully
					if (audioOutputPacket->pts != AV_NOPTS_VALUE) {
						audioOutputPacket->pts =
							av_rescale_q(audioOutputPacket->pts,
							             {1,
								             audioInputCodecContext->sample_rate *
									             audioInputCodecContext->channels},
							             audioStream->time_base);
					}
					if (audioOutputPacket->dts != AV_NOPTS_VALUE) {
						audioOutputPacket->dts =
							av_rescale_q(audioOutputPacket->dts,
							             {1,
								             audioInputCodecContext->sample_rate *
									             audioInputCodecContext->channels},
							             audioStream->time_base);
					}
					// Write packet to file
					audioOutputPacket->stream_index = 1;
					auto result = av_write_frame(outputFormatContext, audioOutputPacket);
					if (result != 0) {
						throw avException("Error in writing video frame");
					}
				}
				av_packet_unref(audioOutputPacket);
			}
			// Send converted frame to encoder
		} else
			throw avException("Failed to decode packet");
	}
    av_free(audioFrame);
    av_free(audioOutputFrame);
    av_free(audioPacket);
    av_free(audioOutputPacket);
}

void ScreenRecorder::DemuxAudioInput(){
    // Create decoder audio packet
    auto audioPacket = make_unique<AVPacket>(*alloc_packet()) ;
    int count = 0;
    int result = 0;
    double avsyncD = (frameCount+1.00)/30*4;
    int avsyncI = (int) (frameCount+1.00)/30*4;
    int audioCount = avsyncI+((avsyncD-avsyncI)>=0.5?1:0);
    auto start = std::chrono::system_clock::now();
    while (av_read_frame(audioInputFormatContext, audioPacket.get()) >= 0) {
        // Some computation here
        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;
        std::cout << elapsed_seconds.count() << "\n";
        //audioCnv.notify_one(); //signal converting thread to start if needed
        if (count++ == audioCount) {
            break;
        }
        // Send packet to decoder
        if(aD.try_lock()) {
            result = avcodec_send_packet(audioInputCodecContext, audioPacket.get());// Try to send a packet without waiting
            if (result >= 0) audioCnv.notify_one(); // notify converter thread if halted
            aD.unlock();
        }
        else {
            unique_lock<mutex> ul(aD);
            audioCnv.notify_one();// Send sync signal to converter thread if necessary
            audioCnv.wait(ul);//Wait for converter thread signal
            result = avcodec_send_packet(audioInputCodecContext, audioPacket.get());// Retry to send a packet
            if (result >= 0) audioCnv.notify_one(); // notify converter thread if halted
        }
        // Check result
        if(result == AVERROR(EAGAIN)) {//buffer is full, wait and retry
            unique_lock<mutex> ul(aD);
            audioCnv.notify_one();// Send sync signal to converter thread
            audioCnv.wait(ul);// Wait for resume signal
            result = avcodec_send_packet(audioInputCodecContext, audioPacket.get());
            if (result >= 0) audioCnv.notify_one(); // notify converter thread if halted
        }
        if (result < 0 && result != AVERROR_EOF) {
            // Decoder error
            throw avException("Failed to send packet to decoder");
        }
        start = std::chrono::system_clock::now();
    }
    //Free allocated memory
}

void ScreenRecorder::ConvertAudioFrames() {
    // Create decoder audio frame
    auto audioFrame = make_unique<AVFrame>(*alloc_audio_frame( audioInputCodecContext->frame_size, audioInputCodecContext->sample_fmt,
                                                      audioInputCodecContext->channel_layout, 0));
    // Create encoder audio frame
    auto audioOutputFrame = make_unique<AVFrame>(*alloc_audio_frame(
            audioOutputCodecContext->frame_size, audioOutputCodecContext->sample_fmt,
            audioOutputCodecContext->channel_layout, 0));
    AVAudioFifo* audioFifo = av_audio_fifo_alloc(audioInputCodecContext->sample_fmt, audioInputCodecContext->channels, 22050);
    int audioFrameNum = 0;
    int64_t pts = 0;
    int result = AVERROR(EAGAIN);
    if(aD.try_lock()) {
        result = avcodec_receive_frame(audioInputCodecContext, audioFrame.get()); // Try to get a decoded frame without waiting
        if(result>=0) audioCnv.notify_one();// Signal demuxer thread to resume if halted
        aD.unlock();
    }
    else {
        std::unique_lock<std::mutex> ul(aD);
        audioCnv.notify_one();// Sync with demuxer thread if necessary
        audioCnv.wait(ul);// Wait for audio demuxer thread sync signal
        result = avcodec_receive_frame(audioInputCodecContext, audioFrame.get()); // Try to get a decoded frame
        if(result>=0) audioCnv.notify_one();// Signal demuxer thread to resume if halted
    }
    // frame
    while(result >= 0 || result == AVERROR(EAGAIN)) {
        if(result >= 0) {
            int got_samples = swr_convert(
                    swrContext, audioOutputFrame->data, audioOutputFrame->nb_samples,
                    (const uint8_t **) audioFrame->data, audioFrame->nb_samples);
            if (got_samples < 0) {
                throw avException("Failed to convert frames");
            }
            else if (got_samples > 0) {
                pts += audioOutputFrame->nb_samples;
                audioOutputFrame->pts = pts;
                if(aW.try_lock()) {
                    result = avcodec_send_frame(audioOutputCodecContext, audioOutputFrame.get());// Try to send a frame without waiting
                    if (result >= 0) audioWrt.notify_one();
                    aW.unlock();
                }
                else {
                    unique_lock<mutex> ul(aW);
                    audioWrt.notify_one();// Send sync signal to output writer thread if necessary
                    audioWrt.wait(ul);//Wait for writer thread signal
                    result = avcodec_send_frame(audioOutputCodecContext, audioOutputFrame.get());// Retry to send frame
                    if (result >= 0) audioWrt.notify_one();
                }
                if (result == AVERROR(EAGAIN)) { // while encoder buffer is full
                    unique_lock<mutex> ul(aW);
                    audioWrt.notify_one();// Send sync signal to output writer thread
                    audioWrt.wait(ul);// Wait for resume signal
                    result = avcodec_send_frame(audioOutputCodecContext, audioOutputFrame.get());// retry
                    if (result >= 0) audioWrt.notify_one();
                }
                if (result < 0 && result !=AVERROR(EAGAIN)) {
                    throw avException("Failed to send frame to encoder"); // Error ending frame to encoder
                }
                // Frame was sent successfully
                while (got_samples > 0) {
                    got_samples = swr_convert(swrContext, audioOutputFrame->data, audioOutputFrame->nb_samples, nullptr, 0);
                    if(got_samples >= 0) {
                        pts += audioOutputFrame->nb_samples;
                        audioOutputFrame->pts = pts;
                        if(aW.try_lock()) {
                            result = avcodec_send_frame(audioOutputCodecContext, audioOutputFrame.get());// Try to send a frame without waiting
                            if (result >= 0) audioWrt.notify_one();
                            aW.unlock();
                        }
                        else {
                            unique_lock<mutex> ul(aW);
                            audioWrt.notify_one();// Send sync signal to output writer thread if necessary
                            audioWrt.wait(ul);//Wait for writer thread signal
                            result = avcodec_send_frame(audioOutputCodecContext, audioOutputFrame.get());// Retry to send frame
                            if (result >= 0) audioWrt.notify_one();
                        }
                        if (result == AVERROR(EAGAIN)) { // while encoder buffer is full
                            unique_lock<mutex> ul(aW);
                            audioWrt.notify_one();// Send sync signal to output writer thread
                            audioWrt.wait(ul);// Wait for resume signal
                            result = avcodec_send_frame(audioOutputCodecContext, audioOutputFrame.get());// retry
                            if (result >= 0) audioWrt.notify_one();
                        }
                        if (result < 0 && result !=AVERROR(EAGAIN)) {
                            throw avException("Failed to send frame to encoder"); // Error ending frame to encoder
                        }
                        // Frame was sent successfully
                    }
                }
            }
            if(aD.try_lock()) {
                result = avcodec_receive_frame(audioInputCodecContext, audioFrame.get()); // Try to get a decoded frame without waiting
                if(result>=0) audioCnv.notify_one();// Signal demuxer thread to resume if halted
                aD.unlock();
            }
            else {
                std::unique_lock<std::mutex> ul(aD);
                audioCnv.notify_one();// Sync with demuxer thread if necessary
                audioCnv.wait(ul);// Wait for audio demuxer thread sync signal
                result = avcodec_receive_frame(audioInputCodecContext, audioFrame.get()); // Try to get a decoded frame
                if(result>=0) audioCnv.notify_one();// Signal demuxer thread to resume if halted
            }
        }
        if(result == AVERROR(EAGAIN)) {
            std::unique_lock<std::mutex> ul(aD);
            audioCnv.notify_one();// Sync with demuxer thread if necessary
            audioCnv.wait(ul);// Wait for audio demuxer thread sync signal
            if(finishedAudioDemux) {
                audioCnv.notify_one();// Sync with main thread if necessary
                break;
            }
            result = avcodec_receive_frame(audioInputCodecContext, audioFrame.get()); // Try to get a decoded frame
            if(result>=0) audioCnv.notify_one();// Signal demuxer thread to resume if halted
        }
        if(result < 0 && result != AVERROR(EAGAIN)) throw avException("Audio Converter/Writer threads syncronization error");
    }
    if (result < 0 && result != AVERROR_EOF && result != AVERROR(EAGAIN)) {
        // Decoder error
        throw avException("Failed to receive decoded packet");
    }
    // Free allocated memory
}

void ScreenRecorder::WriteAudioOutput(AVFormatContext *formatContext, AVRational bq, AVRational cq) {
    // Create encoder audio packet
    auto outputPacket = make_unique<AVPacket>(*alloc_packet());
    int result = AVERROR(EAGAIN);
    if(aW.try_lock()) {
        result = avcodec_receive_packet(audioOutputCodecContext, outputPacket.get()); // Try to receive a new packet without waiting
        if (result >= 0) audioWrt.notify_one();// Signal converter thread to resume if necessary
        aW.unlock();
    }
    else {
        unique_lock<mutex> ul(aW);
        audioWrt.notify_one();// Sync with converter thread if necessary
        audioWrt.wait(ul);// Wait for sync signal from converter thread
        result = avcodec_receive_packet(audioOutputCodecContext, outputPacket.get()); // Try to receive a new packet
        if (result >= 0) audioWrt.notify_one();// Signal converter thread to resume if necessary
    }
    while (result >= 0 || result == AVERROR(EAGAIN)) {
        if (result >= 0) {
            if (outputPacket->pts != AV_NOPTS_VALUE) {
                outputPacket->pts = av_rescale_q(outputPacket->pts, bq, cq);
            }
            if (outputPacket->dts != AV_NOPTS_VALUE) {
                outputPacket->dts = av_rescale_q(outputPacket->dts, bq, cq);
            }
            outputPacket->duration = av_rescale_q(1, bq, cq);
            // Write packet to file
            outputPacket->stream_index = 1;
            result = av_write_frame(formatContext, outputPacket.get());
            if (result != 0) {
                throw avException("Error in writing audio frame");
            }
            if(aW.try_lock()) {
                result = avcodec_receive_packet(audioOutputCodecContext, outputPacket.get()); // Try to receive a new packet without waiting
                if (result >= 0) audioWrt.notify_one();// Signal converter thread to resume if necessary
                aW.unlock();
            }
            else {
                unique_lock<mutex> ul(aW);
                audioWrt.notify_one();// Sync with converter thread if necessary
                audioWrt.wait(ul);// Wait for sync signal from converter thread
                result = avcodec_receive_packet(audioOutputCodecContext, outputPacket.get()); // Try to receive a new packet
                if (result >= 0) audioWrt.notify_one();// Signal converter thread to resume if necessary
            }
        }
        if (result == AVERROR(EAGAIN) ) {// Packet not available
            unique_lock<mutex> ul(aW);
            audioWrt.notify_one();// Sync with converter thread if necessary
            audioWrt.wait(ul);// Wait for sync signal from converter thread
            if (finishedAudioConversion) {
                audioWrt.notify_one();// Sync with main thread if necessary
                break;
            }
            result = avcodec_receive_packet(audioOutputCodecContext, outputPacket.get()); // Try to receive a new packet
            if (result >= 0) audioWrt.notify_one();// Signal converter thread to resume if necessary
        }
        if(result < 0 && result != AVERROR(EAGAIN)) throw avException("Audio Converter/Writer threads syncronization error");
    }
    if(result < 0 && result != AVERROR_EOF && result != AVERROR(EAGAIN)) {
        throw avException("Failed to receive frame from encoder"); // Error ending frame to encoder
    }
    //Free allocated memory
}
