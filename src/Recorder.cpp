//
// Created by gabriele on 31/10/21.
//

#include "Recorder.h"


/** Initialize ffmpeg codecs and devices */
Recorder::Recorder() {
	avdevice_register_all();
}


/**
 * Initialize ffmpeg parameters, audio/video stream and output file
 */
void Recorder::init(Screen params) {
	format.setup_source();

	auto audioPar = format.get_source_audio_codec();
	auto videoPar = format.get_source_video_codec();

	codec.set_source_audio_parameters(audioPar);
	codec.set_source_video_parameters(videoPar);
	codec.setup_source();

	// format
	auto dest_path = "../media/output.mp4";
	format.setup_destination(dest_path);

	auto audio_codec = "aac";
	auto video_codec = "libx264";
	codec.find_encoders(audio_codec, video_codec);

	stream = Stream{format, codec};
	stream.get_video()->time_base = {1, 30};
	stream.get_audio()->time_base = {1, codec.inputContext.get_audio()->sample_rate};

	codec.set_destination_audio_parameters(stream.get_audio()->codecpar);
	codec.set_destination_video_parameters(stream.get_video()->codecpar);

	codec.setup_destination();

	create_out_file(dest_path);
	rescaler.set_audio_scaler(codec);
	rescaler.set_video_scaler(codec);

	format.write_header(options);

	ref_time = get_ref_time(format.inputContext);

//	print_source_info();
//	print_destination_info(dest_path);
}


[[maybe_unused]]
void Recorder::print_source_info() const{
	// todo cambiare nomi device
	av_dump_format(format.inputContext.get_video(), 0 , DEFAULT_VIDEO_INPUT_DEVICE, 0);
	av_dump_format(format.inputContext.get_audio(), 0, DEFAULT_AUDIO_INPUT_DEVICE, 0);
}

[[maybe_unused]]
void Recorder::print_destination_info(const string &dest) const {
	av_dump_format(format.outputContext.get_video(), 0, dest.c_str(), 1);
}

/**
 * Create output media file
 */
void Recorder::create_out_file(const string &dest) const {
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

/**
 * Start the capture of the screen
 *
 * Release 2 threads for audio/video demuxing and 2 threads for audio/video frame conversion
 */
void Recorder::capture() {
	if(num_core > 2) {
		//	th_audio_demux = thread();
		//	th_audio_convert = thread();
		th_video_demux = thread{&Recorder::DemuxVideoInput, this};
		th_video_convert = thread{&Recorder::ConvertVideoFrames, this};
	}else{
		th_video = thread{&Recorder::CaptureVideoFrames, this};
		th_audio = thread{&Recorder::CaptureAudioFrames, this};
	}
}

void Recorder::capture_blocking(){}

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
	pausedVideo = true;
	pausedAudio = true;
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

	stopped = false;
}

/* Private functions */
void Recorder::join_all() {
	if(num_core > 2) {
		th_video_demux.join();
		th_video_convert.join();
//		th_audio_demux.join();
//		th_audio_convert.join();
	}else{
		th_video.join();
		th_audio.join();
	}
}

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

//void Recorder::DemuxAudioInput() {
//	// Create decoder audio packet
//	AVPacket *audioPacket = alloc_packet();
//	int result;
//	auto start = std::chrono::system_clock::now();
//	auto end = start;
//	bool synced = false;
//	bool sync = false;
////    bool last_pts_set = false;
////    int64_t last_pts=MAXINT64;
////    int64_t offset=50000000;
//#ifdef WIN32
//	//    ref_time=audioInputFormatContext->start_time*10+offset;
//#else
//	if (audioInputFormatContext->start_time == ref_time) {// If video started later
//		sync = true;// Video needs to set the ref_time value
//	}
//#endif
//	avformat_flush(audioInputFormatContext);
//	while (av_read_frame(audioInputFormatContext, audioPacket) >= 0) {
//		if (*stopped) {
////            if(!last_pts_set) {
////                last_pts=audioPacket->pts+offset;
////                last_pts_set=true;
////            }
//			lock_guard<mutex> ul(*vD);
//			*finishedAudioDemux = true;
//			audioCnv->notify_one(); // notify converter thread if halted
//			break;
//		}
//		if (!synced && sync && !*pausedAudio) {
//			if (audioPacket->pts > ref_time) ref_time = audioPacket->pts;
//			synced = true;
//		}
//		if (!*pausedAudio && audioPacket->pts >= ref_time) {
//			if (*pausedVideo) *pausedVideo = false;
//			end = std::chrono::system_clock::now();
//			std::chrono::duration<double> elapsed_seconds = end - start;
//			std::cout << "Received audio packet after " << elapsed_seconds.count() << " s\n";
//			//audioCnv.notify_one(); //signal converting thread to start if needed
//			// Send packet to decoder
//			if (aD->try_lock()) {
//				result =
//					avcodec_send_packet(audioInputCodecContext, audioPacket);// Try to send a packet without waiting
//				if (result >= 0) {
//					audioCnv->notify_one(); // notify converter thread if halted
//					av_packet_unref(audioPacket);
//				}
//				aD->unlock();
//			} else result = AVERROR(EAGAIN);
//			// Check result
//			if (result == AVERROR(EAGAIN)) {//buffer is full or could not acquire lock, wait and retry
//				unique_lock<mutex> ul(*aD);
//				audioCnv->notify_one();// Send sync signal to converter thread
//				audioCnv->wait(ul);// Wait for resume signal
//				result = avcodec_send_packet(audioInputCodecContext, audioPacket);
//				if (result >= 0) {
//					audioCnv->notify_one(); // notify converter thread if halted
//					av_packet_unref(audioPacket);
//				}
//			}
//			if (result < 0 && result != AVERROR_EOF && result != AVERROR(EAGAIN)) {
//				// Decoder error
//				throw avException("Failed to send packet to decoder");
//			}
//			//Packet sent
//			start = std::chrono::system_clock::now();
//		}
////        else if(audioPacket->pts >= last_pts){
////        }
//		else {
//			av_packet_unref(audioPacket);
//			if (!*pausedVideo) *pausedVideo = true;
////            if(synced) synced = false;
//		}
//	}
//	//Free allocated memory
//	av_packet_unref(audioPacket);
//	av_packet_free(&audioPacket);
//}
//
//void Recorder::ConvertAudioFrames() {
//	// Create decoder audio frame
//	AVFrame *audioFrame = alloc_audio_frame(audioInputCodecContext->frame_size, audioInputCodecContext->sample_fmt,
//	                                        audioInputCodecContext->channel_layout, 0);
//	// Create encoder audio frame
//	AVFrame *audioOutputFrame = av_frame_alloc();
//	if (!audioOutputFrame) {
//		throw avException("Error on output audio frame initialization");
//	}
//	// Create encoder audio packet
//	AVPacket *audioOutputPacket = alloc_packet();
//	int frame_size = 0;
//	int64_t pts = 0;
//	bool finished = false;
//	int result = AVERROR(EAGAIN);
//	if (aD->try_lock()) {
//		result =
//			avcodec_receive_frame(audioInputCodecContext, audioFrame); // Try to get a decoded frame without waiting
//		if (result >= 0) audioCnv->notify_one();// Signal demuxer thread to resume if halted
//		aD->unlock();
//	} else result = AVERROR(EAGAIN);
//	while (result >= 0 || result == AVERROR(EAGAIN)) {
//		if (result >= 0) {
//			//Convert frames and then write them to file
//			if (frame_size == 0) frame_size = audioFrame->nb_samples;
//			convertAndWriteAudioFrames(swrContext,
//			                           audioOutputCodecContext,
//			                           audioInputCodecContext,
//			                           audioStream,
//			                           outputFormatContext,
//			                           audioFrame,
//			                           audioOutputFrame,
//			                           audioOutputPacket,
//			                           &pts,
//			                           wR,
//			                           writeFrame);
//			av_frame_unref(audioFrame);
//			init_audio_frame(audioFrame, frame_size, audioInputCodecContext->sample_fmt,
//			                 audioInputCodecContext->channel_layout, 0);
//		}
//		if (aD->try_lock()) {
//			result =
//				avcodec_receive_frame(audioInputCodecContext, audioFrame); // Try to get a decoded frame without waiting
//			if (result >= 0) audioCnv->notify_one();// Signal demuxer thread to resume if halted
//			aD->unlock();
//		} else result = AVERROR(EAGAIN);
//		if (result == AVERROR(EAGAIN)) {//buffer is not ready or could not acquire lock, wait and retry
//			std::unique_lock<std::mutex> ul(*aD);
//			if (!*finishedAudioDemux) {
//				audioCnv->notify_one();// Signal demuxer thread if necessary
//				audioCnv->wait(ul);// Wait for resume signal
//				result = avcodec_receive_frame(audioInputCodecContext, audioFrame); // Try to get a decoded frame
//				if (result >= 0) audioCnv->notify_one();// Signal demuxer thread to resume if halted
//			} else if (finished) break;
//			finished = true;
//		}
////        else if(*finishedAudioDemux) {
//////                audioCnv->notify_one();// Sync with main thread if necessary
////            break;
////        }
//		if (result < 0 && result != AVERROR(EAGAIN))
//			throw avException("Audio Converter/Writer threads syncronization error");
//
//	}
//	if (result < 0 && result != AVERROR_EOF && result != AVERROR(EAGAIN)) {
//		// Decoder error
//		throw avException("Failed to receive decoded packet");
//	}
//	//Convert last frame and then write it to file
//	convertAndWriteLastAudioFrames(swrContext,
//	                               audioOutputCodecContext,
//	                               audioInputCodecContext,
//	                               audioStream,
//	                               outputFormatContext,
//	                               audioOutputFrame,
//	                               audioOutputPacket,
//	                               &pts,
//	                               wR,
//	                               writeFrame);
//	// Free allocated memory
//	av_frame_unref(audioFrame);
//	av_frame_free(&audioFrame);
//	av_frame_free(&audioOutputFrame);
//	av_packet_free(&audioOutputPacket);
//}

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

		if (stopped) {
			lock_guard<mutex> ul(vD);
			finishedVideoDemux = true;
			videoCnv.notify_one(); // notify converter thread if halted
			break;
		}

		if (!synced && sync && !pausedVideo) {
			if (pts > ref_time) ref_time = pts;
			synced = true;
		}

		if (!pausedVideo && pts >= ref_time) {
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
				unique_lock<mutex> ul(vD);
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

		in_frame.unref();

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
			if (!finishedVideoDemux) {
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




