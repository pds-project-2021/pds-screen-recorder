//
// Created by gabriele on 31/10/21.
//

#include "Recorder.h"

/** Initialize lib codecs and devices */
Recorder::Recorder() {
	avdevice_register_all();
}

/**
 * Get output audio codec
 * @return
 */
[[maybe_unused]] std::string Recorder::get_audio_codec() {
	return audio_codec;
}

/**
 * Get output video codec
 * @return
 */
[[maybe_unused]] std::string Recorder::get_video_codec() {
	return video_codec;
}

/**
 * Set output audio codec
 * @param device
 */
void Recorder::set_audio_codec(const std::string &cod) {
	audio_codec = cod;
}

/**
 * Set output video codec
 * @param device
 */
void Recorder::set_video_codec(const std::string &cod) {
	video_codec = cod;
}

/**
 * Initialize lib parameters, audio/video stream and output file
 */

void Recorder::set_destination(const std::string& dest_path) {
    destination_path = dest_path;
}

std::string Recorder::get_destination() {
    return destination_path;
}


void Recorder::init(Screen params) {
#ifdef WIN32
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif

	format.set_screen_params(params);
	format.setup_source();

	auto audioPar = format.get_source_audio_codec();
	auto videoPar = format.get_source_video_codec();

	codec.set_source_audio_parameters(audioPar);
	codec.set_source_video_parameters(videoPar);
	codec.setup_source();

	// format
	format.setup_destination(destination_path);

	codec.find_encoders(audio_codec, video_codec);

	stream = Stream{format, codec};
	stream.get_video()->time_base = {1, 30};
	stream.get_audio()->time_base = {1, codec.inputContext.get_audio()->sample_rate};

	codec.set_destination_audio_parameters(stream.get_audio()->codecpar);
	codec.set_destination_video_parameters(stream.get_video()->codecpar);

	codec.setup_destination();

	create_out_file(destination_path);
	rescaler.set_audio_scaler(codec);
	rescaler.set_video_scaler(codec);

	format.write_header(options);

	ref_time = get_ref_time(format.inputContext);

#ifdef WIN32
    CoUninitialize();
#endif
}

[[maybe_unused]] [[maybe_unused]]
void Recorder::print_source_info() {
	av_dump_format(format.inputContext.get_audio(), 0, format.get_audio_device().c_str(), 0);
	av_dump_format(format.inputContext.get_video(), 0 , format.get_video_device().c_str(), 0);
}

[[maybe_unused]]
void Recorder::print_destination_info(const std::string &dest) const {
	av_dump_format(format.outputContext.get_video(), 0, dest.c_str(), 1);
}


/**
 * Start the capture of the screen
 *
 * Release 2 threads for audio/video demuxing and 2 threads for audio/video frame conversion
 */
void Recorder::capture() {
	capturing = true;

	if(num_core > 2) {
		th_audio_demux = std::thread{&Recorder::DemuxAudioInput, this};
		th_audio_convert = std::thread{&Recorder::ConvertAudioFrames, this};
		th_video_demux = std::thread{&Recorder::DemuxVideoInput, this};
		th_video_convert = std::thread{&Recorder::ConvertVideoFrames, this};
	}else{
		th_video = std::thread{&Recorder::CaptureVideoFrames, this};
		th_audio = std::thread{&Recorder::CaptureAudioFrames, this};
	}
}

/**
 * Pause the capture
 */
void Recorder::pause(){
	pausedVideo = true;
	pausedAudio = true;
}

/**
 * Resume capture from pause
 */
void Recorder::resume() {
	pausedVideo = false;
	pausedAudio = false;
}

/**
 * End the capture
 */
void Recorder::terminate(){
	stopped = true;

	// wait all threads
	join_all();

	auto outputFormatContext = format.outputContext.get_video();

	// Write file trailer data
	auto ret = av_write_trailer(outputFormatContext);
	if (ret < 0) {
		throw avException("Error in writing av trailer");
	}

	capturing = false;
}

/**
 * Check if capture is paused
 */
bool Recorder::is_paused() {
	return pausedAudio && pausedVideo;
}

/**
 * Check if capture is running
 */
bool Recorder::is_capturing() {
	return capturing;
}

/**
 * Manually set number of working threads
 *
 * @param th number of threads
 */
void Recorder::set_threads(unsigned int th) {
	num_core = th;
}

/* Private functions */

void Recorder::join_all() {
	if(num_core > 2) {
		th_audio_demux.join();
		th_audio_convert.join();
		th_video_demux.join();
		th_video_convert.join();
	}else{
		th_audio.join();
		th_video.join();
	}
}

/**
 * Create output media file
 */
void Recorder::create_out_file(const std::string &dest) const {
	auto ctx = format.outputContext.get_video();

	/* create empty video file */
	if (!(ctx->flags & AVFMT_NOFILE)) {
		auto ret = avio_open2(&(ctx->pb), dest.c_str(), AVIO_FLAG_WRITE,nullptr, nullptr);
		if (ret < 0) {
			char buf[35];
			av_strerror(ret, buf, sizeof(buf));
			throw avException(buf);
		}
	}
}

/* single thread (de)muxing */
void Recorder::CaptureAudioFrames() {
	auto inputFormatContext = format.inputContext.get_audio();
	auto inputCodecContext= codec.inputContext.get_audio();
	auto outputCodecContext = codec.outputContext.get_audio();
	auto outputFormatContext = format.outputContext.get_audio();
	auto audioStream = stream.get_audio();
	auto swrContext = rescaler.get_swr();

	int frame_size = 0;
	int got_frame = 0;
	int64_t pts = 0;
	bool synced = false;
	bool sync = false;

	if (inputFormatContext->start_time == ref_time) { // If video started later
		sync = true; // Video needs to set the ref_time value
	}

	// Handle audio input stream packets
	avformat_flush(inputFormatContext);

	auto read_frame = true;
	while (read_frame) {
		auto in_packet = Packet{};
		read_frame = av_read_frame(inputFormatContext, in_packet.into()) >= 0;

		if (stopped.load()) {
			break;
		}

		if (!synced && sync && !pausedAudio.load()) {
			if (in_packet.into()->pts > ref_time) {
				ref_time = in_packet.into()->pts;
			}
			synced = true;
		}

		if (!pausedAudio.load() && in_packet.into()->pts >= ref_time) {
			auto in_frame = Frame{};

			// Send packet to decoder
			decode(inputCodecContext, in_packet.into(), in_frame.into(), &got_frame);

			// check if decoded frame is ready
			if (got_frame > 0) { // frame is ready

				// Convert and write frames
				if (frame_size == 0) {
					frame_size = in_frame.into()->nb_samples;
				}

				convertAndWriteAudioFrames(swrContext,
				                           outputCodecContext,
				                           inputCodecContext,
				                           audioStream,
				                           outputFormatContext,
				                           in_frame.into(),
				                           &pts,
				                           &wR,
				                           &writeFrame);
			} else {
				throw avException("Failed to decode packet");
			}
		}
	}

	// Convert and write last frame
	convertAndWriteLastAudioFrames(swrContext,
	                               outputCodecContext,
	                               inputCodecContext,
	                               audioStream,
	                               outputFormatContext,
	                               &pts,
	                               &wR,
								   &writeFrame);
}

void Recorder::CaptureVideoFrames() {
	auto inputFormatContext = format.inputContext.get_video();
	auto inputCodecContext= codec.inputContext.get_video();
	auto outputCodecContext = codec.outputContext.get_video();
	auto outputFormatContext = format.outputContext.get_video();
	auto videoStream = stream.get_video();
	auto swsContext = rescaler.get_sws();

	int64_t count = 0;
	int frameNum = 0; // frame number in a second

	int got_frame = 0;
	bool synced = false;
	bool sync = false;

	if (inputFormatContext->start_time == ref_time) {// If video started later
		sync = true;// Video needs to set the ref_time value
	}

	auto read_frame = true;
	while (read_frame) {
		auto in_packet = Packet{};
		read_frame = av_read_frame(inputFormatContext, in_packet.into()) >= 0;

		if (!synced && sync && !pausedVideo.load()) {
			if (in_packet.into()->pts > ref_time) ref_time = in_packet.into()->pts;
			synced = true;
		}

		if (stopped.load()) {
			break;
		}

		if (!pausedVideo.load() && in_packet.into()->pts >= ref_time) {
			if (frameNum++ == 30)
				frameNum = 0; // reset every fps frames

			if (in_packet.into()->stream_index == videoStream->index) {
				auto in_frame = Frame{};

				// Send packet to decoder
				decode(inputCodecContext, in_packet.into(), in_frame.into(), &got_frame);

				// check if decoded frame is ready
				if (got_frame) { // frame is ready
					// Convert frame picture format and write frame to file
					count++;
					convertAndWriteVideoFrame(swsContext,
					                          outputCodecContext,
					                          inputCodecContext,
					                          videoStream,
					                          outputFormatContext,
					                          in_frame.into(),
					                          &count,
					                          &wR,
					                          &writeFrame);
				}
			}
		}
	}

	// Handle delayed frames
	convertAndWriteDelayedVideoFrames(outputCodecContext,
	                                  videoStream,
	                                  outputFormatContext,
	                                  &wR,
	                                  &writeFrame);
}

/* multi thread (de)muxing */

void Recorder::DemuxAudioInput() {
	auto start = std::chrono::system_clock::now();
	auto end = start;
	bool synced = false;
	bool sync = false;
	int result;

	auto inputFormatContext = format.inputContext.get_audio();
	auto inputCodecContext = codec.inputContext.get_audio();

	if (inputFormatContext->start_time == ref_time) {// If video started later
		sync = true;// Video needs to set the ref_time value
	}

	avformat_flush(inputFormatContext);
	auto read_packet = true;
	while (read_packet) {
		auto in_packet = Packet{};
		read_packet = av_read_frame(inputFormatContext, in_packet.into()) >= 0;

		if (stopped.load()) {
            std::lock_guard<std::mutex> ul(vD);
			finishedAudioDemux = true;
			audioCnv.notify_one(); // notify converter thread if halted
			break;
		}

		if (!synced && sync && !pausedAudio.load()) {
			if (in_packet.into()->pts > ref_time) ref_time = in_packet.into()->pts;
			synced = true;
		}

		if (!pausedAudio.load() && in_packet.into()->pts >= ref_time) {

			end = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds = end - start;
//			std::cout << "Received audio packet after " << elapsed_seconds.count() << " s\n";

			// Send packet to decoder
			if (aD.try_lock()) {
				result = avcodec_send_packet(inputCodecContext, in_packet.into()); // Try to send a packet without waiting
				if (result >= 0) {
					audioCnv.notify_one(); // notify converter thread if halted
					av_packet_unref(in_packet.into());
				}
				aD.unlock();
			} else {
				result = AVERROR(EAGAIN);
			}

			// Check result
			if (result == AVERROR(EAGAIN)) {//buffer is full or could not acquire lock, wait and retry
                std::unique_lock<std::mutex> ul(aD);
				audioCnv.notify_one(); // Send sync signal to converter thread
				audioCnv.wait(ul); // Wait for resume signal
				result = avcodec_send_packet(inputCodecContext, in_packet.into());
				if (result >= 0) {
					audioCnv.notify_one(); // notify converter thread if halted
				}
			}

			if (result < 0 && result != AVERROR_EOF && result != AVERROR(EAGAIN)) {
				// Decoder error
				throw avException("Failed to send packet to decoder");
			}
			//Packet sent
			start = std::chrono::system_clock::now();
		}
	}
}

void Recorder::ConvertAudioFrames() {
	auto inputCodecContext= codec.inputContext.get_audio();
	auto outputCodecContext = codec.outputContext.get_audio();
	auto outputFormatContext = format.outputContext.get_audio();
	auto audioStream = stream.get_audio();
	auto swrContext = rescaler.get_swr();

	int frame_size = 0;
	int64_t pts = 0;
	bool finished = false;
	int result = AVERROR(EAGAIN);

	auto in_frame = Frame{};

	if (aD.try_lock()) {
		result = avcodec_receive_frame(inputCodecContext, in_frame.into()); // Try to get a decoded frame without waiting
		if (result >= 0) {
			audioCnv.notify_one(); // Signal demuxer thread to resume if halted
		}
		aD.unlock();
	} else {
		result = AVERROR(EAGAIN);
	}

	while (result >= 0 || result == AVERROR(EAGAIN)) {
		if (result >= 0) {
			//Convert frames and then write them to file
			if (frame_size == 0) {
				frame_size = in_frame.into()->nb_samples;
			}

			convertAndWriteAudioFrames(swrContext,
			                           outputCodecContext,
			                           inputCodecContext,
			                           audioStream,
			                           outputFormatContext,
			                           in_frame.into(),
			                           &pts,
			                           &wR,
			                           &writeFrame);
//			in_frame.unref();
		}
		if (aD.try_lock()) {
			result = avcodec_receive_frame(inputCodecContext, in_frame.into()); // Try to get a decoded frame without waiting
			if (result >= 0) {
				audioCnv.notify_one();// Signal demuxer thread to resume if halted
			}
			aD.unlock();
		} else {
			result = AVERROR(EAGAIN);
		}

		if (result == AVERROR(EAGAIN)) {//buffer is not ready or could not acquire lock, wait and retry
			std::unique_lock<std::mutex> ul(aD);
			if (!finishedAudioDemux.load()) {
				audioCnv.notify_one();// Signal demuxer thread if necessary
				audioCnv.wait(ul);// Wait for resume signal
				result = avcodec_receive_frame(inputCodecContext, in_frame.into()); // Try to get a decoded frame
				if (result >= 0) {
					audioCnv.notify_one();// Signal demuxer thread to resume if halted
				}
			} else if (finished) {
				break;
			}
			finished = true;
		}

		if (result < 0 && result != AVERROR(EAGAIN)) {
			throw avException("Audio Converter/Writer threads syncronization error");
		}
	}

	if (result < 0 && result != AVERROR_EOF && result != AVERROR(EAGAIN)) {
		// Decoder error
		throw avException("Failed to receive decoded packet");
	}

	//Convert last frame and then write it to file
	convertAndWriteLastAudioFrames(swrContext,
	                               outputCodecContext,
	                               inputCodecContext,
	                               audioStream,
	                               outputFormatContext,
	                               &pts,
	                               &wR,
	                               &writeFrame);
}

void Recorder::DemuxVideoInput() {
	// frame number in a second
	int frameNum = 0;
	int result;
	auto synced = false;
	auto sync = false;

	auto start = std::chrono::system_clock::now();
	auto inputFormatContext = format.inputContext.get_video();
	auto inputCodecContext = codec.inputContext.get_video();

	if (inputFormatContext->start_time == ref_time) { // If video started later
		sync = true;// Video needs to set the ref_time value
	}

	auto read_frame = true;
	while (read_frame) {
		auto packet = Packet{};
		read_frame = av_read_frame(inputFormatContext, packet.into()) >= 0;

		auto pts = packet.into()->pts;

		if (stopped.load()) {
            std::lock_guard<std::mutex> ul(vD);
			finishedVideoDemux = true;
			videoCnv.notify_one(); // notify converter thread if halted
			break;
		}

		if (!synced && sync && !pausedVideo.load()) {
			if (pts > ref_time) ref_time = pts;
			synced = true;
		}

		if (!pausedVideo.load() && pts >= ref_time) {
			if (frameNum == 30) {
				frameNum = 0; // reset every fps frames
				auto end = std::chrono::system_clock::now();
				std::chrono::duration<double> elapsed_seconds = end - start;
//				std::cout << "Received 30 video packets in " << elapsed_seconds.count() << " s\n";
				start = std::chrono::system_clock::now();
			}

			// Send packet to decoder
			if (vD.try_lock()) {
				result = avcodec_send_packet(inputCodecContext, packet.into());// Try to send a packet without waiting
				if (result >= 0) {
					videoCnv.notify_one(); // notify converter thread if halted
					packet.unref();
				}
				vD.unlock();
			} else {
				result = AVERROR(EAGAIN);
			}

			// Check result
			if (result == AVERROR(EAGAIN)) { //buffer is full or could not acquire lock, wait and retry
                std::unique_lock<std::mutex> ul(vD);
				videoCnv.notify_one();// Send sync signal to converter thread
				videoCnv.wait(ul);// Wait for resume signal
				result = avcodec_send_packet(inputCodecContext, packet.into());
				if (result >= 0) {
					videoCnv.notify_one(); // notify converter thread if halted
				}
			}
			if (result < 0 && result != AVERROR_EOF && result != AVERROR(EAGAIN)) {
				// Decoder error
				throw avException("Failed to send packet to decoder");
			}
			//Packet sent
			frameNum++;
		}
	}
}

void Recorder::ConvertVideoFrames() {
	auto inputCodecContext= codec.inputContext.get_video();
	auto outputCodecContext = codec.outputContext.get_video();
	auto outputFormatContext = format.outputContext.get_video();
	auto videoStream = stream.get_video();
	auto swsContext = rescaler.get_sws();


	int64_t count = 0;
	int result = AVERROR(EAGAIN);
	bool finished = false;

	auto in_frame = Frame{};

	if (vD.try_lock()) {
		result = avcodec_receive_frame(inputCodecContext, in_frame.into()); // Try to get a decoded frame without waiting
		if (result >= 0) {
			videoCnv.notify_one();// Signal demuxer thread to resume if halted
		}
		vD.unlock();
	} else {
		result = AVERROR(EAGAIN);
	}

	while (result >= 0 || result == AVERROR(EAGAIN)) {
		if (result >= 0) {
			//Convert frames and then write them to file
			count++;

			convertAndWriteVideoFrame(swsContext,
			                          outputCodecContext,
			                          inputCodecContext,
			                          videoStream,
			                          outputFormatContext,
									  in_frame.into(),
			                          &count,
			                          &wR,
			                          &writeFrame);
		}

//		in_frame.unref();

		if (vD.try_lock()) {
			result = avcodec_receive_frame(inputCodecContext, in_frame.into()); // Try to get a decoded frame without waiting
			if (result >= 0) {
				videoCnv.notify_one(); // Signal demuxer thread to resume if halted
			}
			vD.unlock();
		} else {
			result = AVERROR(EAGAIN);
		}

		if (result == AVERROR(EAGAIN)) {//buffer is not ready or could not acquire lock, wait and retry
			std::unique_lock<std::mutex> ul(vD);
			if (!finishedVideoDemux.load()) {
				videoCnv.notify_one();// Signal demuxer thread if necessary
				videoCnv.wait(ul);// Wait for resume signal
				result = avcodec_receive_frame(inputCodecContext, in_frame.into()); // Try to get a decoded frame
				if (result >= 0) videoCnv.notify_one();// Signal demuxer thread to resume if halted
			} else if (finished) {
				break;
			}
			finished = true;
		}

		if (result < 0 && result != AVERROR_EOF && result != AVERROR(EAGAIN)) {
			throw avException("Audio Converter/Writer threads syncronization error");
		}
	}

	if (result < 0 && result != AVERROR_EOF && result != AVERROR(EAGAIN)) {
		// Decoder error
		throw avException("Failed to receive decoded packet");
	}

	//Convert last frame and then write it to file
	convertAndWriteDelayedVideoFrames(outputCodecContext,
	                                  videoStream,
	                                  outputFormatContext,
	                                  &wR,
	                                  &writeFrame);
}





