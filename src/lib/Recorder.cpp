#include "Recorder.h"

/** Initialize lib codecs and devices */
Recorder::Recorder() {
	avdevice_register_all();
}

Recorder::~Recorder() {
	if (capturing) {
		terminate();
	}
	log_info("Recorder destroyed");
}

/**
 * Get audio layout
 * @return
 */
[[maybe_unused]] AudioLayout Recorder::get_audio_layout() {
	return audio_layout;
}

/**
 * Set audio layout [choices: MONO, STEREO]
 * @return
 */
[[maybe_unused]] void Recorder::set_audio_layout(AudioLayout layout) {
	audio_layout = layout;
	if (layout == NONE) resync_enabled = false;
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
[[maybe_unused]] void Recorder::set_audio_codec(const std::string &cod) {
	audio_codec = cod;
}

/**
 * Set output video codec
 * @param device
 */
[[maybe_unused]] void Recorder::set_video_codec(const std::string &cod) {
	video_codec = cod;
}

/**
 * Initialize lib parameters, audio/video stream and output file
 */

[[maybe_unused]] void Recorder::set_destination(const std::string &dest_path) {
	auto path = std::filesystem::path(dest_path);

	if (is_file_str(dest_path)) {
		std::filesystem::create_directories(path.parent_path());
		destination_path = dest_path;
	} else {
		std::filesystem::create_directories(path);
		destination_path = get_default_path(path);
	}
}

std::string Recorder::get_destination() {
	return destination_path;
}

void Recorder::set_screen_params(const Screen &params) {
	screen = params;
}

[[maybe_unused]] Screen Recorder::get_screen_params() {
	return screen;
}

[[maybe_unused]] bool Recorder::get_forced_resync() {
	return resync_enabled;
}

[[maybe_unused]] void Recorder::set_forced_resync(bool s) {
	resync_enabled = s;
}

[[maybe_unused]]
void Recorder::print_source_info() {
	av_dump_format(format.inputContext.get_audio(), 0, format.get_audio_device().c_str(), 0);
	av_dump_format(format.inputContext.get_video(), 0, format.get_video_device().c_str(), 0);
}

[[maybe_unused]]
void Recorder::print_destination_info(const std::string &dest) const {
	av_dump_format(format.outputContext.get_video(), 0, dest.c_str(), 1);
}

std::string Recorder::get_exec_error(bool &err) {
	std::lock_guard<std::mutex> lg(eM);
	if (rec_error) {
		err = true;
		return err_string;
	} else {
		err = false;
		return "No error";
	}
}

/**
 * Start the capture of the screen
 *
 * Release 2 threads for audio/video demuxing and 2 threads for audio/video frame conversion
 */
void Recorder::capture() {
	if (!capturing) {
		init();

		capturing = true;

		if (num_core > 2) {
			if (audio_layout != NONE) {
				th_audio_demux = std::thread{&Recorder::DemuxAudioInput, this};
				th_audio_convert = std::thread{&Recorder::ConvertAudioFrames, this};
			}
			th_video_demux = std::thread{&Recorder::DemuxVideoInput, this};
			th_video_convert = std::thread{&Recorder::ConvertVideoFrames, this};
		} else {
			th_video = std::thread{&Recorder::CaptureVideoFrames, this};
			if (audio_layout != NONE) th_audio = std::thread{&Recorder::CaptureAudioFrames, this};
		}
	}
}

/**
 * Pause the capture
 */
void Recorder::pause() {
	std::unique_lock<std::mutex> rl(r);
	pausing = true;
	if (audio_layout == NONE) pausedAudio = true;
	resumeWait.wait(rl, [this]() -> bool { return ((pausedVideo && pausedAudio) || !capturing || stopped); });
	pausing = false;
}

/**
 * Resume capture from pause
 */
void Recorder::resume() {
	std::unique_lock<std::mutex> rl(r);
	resuming = true;
	if (audio_layout == NONE) pausedAudio = false;
	resumeWait.wait(rl, [this]() -> bool { return ((!pausedVideo && !pausedAudio) || !capturing || stopped); });
	resuming = false;
}

/**
 * End the capture
 */
void Recorder::terminate() {
	if (capturing && !stopped) {
		stopped = true;

		// wait all threads
		join_all();

		auto outputFormatContext = format.outputContext.get_video();

		// Write file trailer data
		auto ret = av_write_trailer(outputFormatContext);
		if (ret < 0) {
			throw avException("Error in writing av trailer");
		}

		reset();
	}
}

void Recorder::reset() {
	// reset libav resources
	rescaler.reset();
	codec.reset();
	format.reset();

	stopped = false;
	pausedVideo = false;
	pausedAudio = false;
	finishedVideoDemux = false;
	finishedAudioDemux = false;
	capturing = false;
	resuming = false;
	pausing = false;
	resync_enabled = true;
	max_pts = 0;
	min_pts = 0;
	resync_enabled = false;
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
 * Set the minimum number of threads for capture
 */
[[maybe_unused]] void Recorder::set_low_profile() {
	set_threads(2);
}

/**
 * Set the maximum number of threads for capture
 */
[[maybe_unused]] void Recorder::set_high_profile() {
	set_threads(4);
}

/* Private functions */

/**
 * Initialize Format and Codec resources for the capture
 */
void Recorder::init() {
#ifdef WIN32
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif

	format.set_screen_params(screen);
	format.set_audio_layout(audio_layout);
	format.setup_source();
	format.setup_destination(destination_path);

	auto audioPar = format.get_source_audio_codec();
	auto videoPar = format.get_source_video_codec();

	codec.set_audio_layout(audio_layout);
	codec.set_source_audio_parameters(audioPar);
	codec.set_source_video_parameters(videoPar);

	codec.setup_source();

	codec.find_encoders(audio_codec, video_codec);
	codec.open_streams(format);
	codec.setup_destination();

	create_out_file(destination_path);
	rescaler.set_audio_layout(audio_layout);
	rescaler.set_audio_scaler(codec);
	rescaler.set_video_scaler(codec);

	format.write_header();

#ifdef WIN32
	CoUninitialize();
#endif
//    throw avException("Error");
}

void Recorder::join_all() {
	if (num_core > 2) {
		if (audio_layout != NONE) {
			th_audio_demux.join();
			th_audio_convert.join();
		}
		th_video_demux.join();
		th_video_convert.join();
	} else {
		if (audio_layout != NONE) th_audio.join();
		th_video.join();
	}
}

/**
 * Manually set number of working threads
 *
 * @param th number of threads
 */
[[maybe_unused]] void Recorder::set_threads(unsigned int th) {
	num_core = th;
}

/**
 * Create output media file
 */
void Recorder::create_out_file(const std::string &dest) const {
	auto ctx = format.outputContext.get_video();

	/* create empty video file */
	if (!(ctx->flags & AVFMT_NOFILE)) {
		auto ret = avio_open2(&(ctx->pb), dest.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
		if (ret < 0) {
			char buf[35];
			av_strerror(ret, buf, sizeof(buf));
			throw avException(buf);
		}
	}
}

void Recorder::handle_rec_error(const std::string &th_name, const unsigned int &th_num, const char *what) {
	std::lock_guard<std::mutex> lg(eM);
	if (capturing) capturing = false;
	rec_error = true;
	err_string = "Unexpected error during recording";
	if (!th_name.empty()) {
		err_string.append(" in ");
		err_string.append(th_name);
		err_string.append(" thread");
	}
	if (what != nullptr) {
		err_string.append(": ");
		err_string.append(what);
	}
	if (num_core > 2) {
		if (audio_layout != NONE) {
			if (th_num != 1) th_audio_demux.join();
			else {
				th_audio_demux.join();
				std::lock_guard<std::mutex> ul(aC);
				finishedAudioDemux = true;
				audioCnv.notify_one(); // notify thread if halted
			}
			if (th_num != 2) th_audio_convert.join();
			else {
				th_audio_convert.join();
				std::unique_lock<std::mutex> ul(aC);
				audioCnv.notify_one();// notify thread if halted
			}
		}
		if (th_num != 3) th_video_demux.join();
		else {
			th_video_demux.join();
			std::lock_guard<std::mutex> ul(vC);
			finishedVideoDemux = true;
			videoCnv.notify_one(); // notify thread if halted
		}
		if (th_num != 4) th_video_convert.join();
		else {
			th_video_convert.join();
			std::lock_guard<std::mutex> ul(vC);
			videoCnv.notify_one(); // notify thread if halted
		}
	} else {
		if (audio_layout != NONE) th_audio.join();
		th_video.join();
	}
	stopped = true;
}

/* single thread (de)muxing */
void Recorder::CaptureAudioFrames() {
	try {
		auto inputFormatContext = format.inputContext.get_audio();
		auto inputCodecContext = codec.inputContext.get_audio();
		auto outputCodecContext = codec.outputContext.get_audio();
		auto outputFormatContext = format.outputContext.get_audio();
		auto audioStream = codec.streams.get_audio();
		auto swrContext = rescaler.get_swr();

		int got_frame = 0;
		int64_t pts = 0;
		unsigned int frame_size = 0;

		// Handle audio input stream packets
		avformat_flush(inputFormatContext);

		auto read_frame = true;
		while (read_frame) {
			auto in_packet = Packet{};
			if (!capturing) {
				std::unique_lock<std::mutex> ul(aC);
				finishedAudioDemux = true;
				audioCnv.notify_one(); // notify converter thread if halted
				return;
			}
			if (stopped) {
				std::unique_lock<std::mutex> ul(aC);
				finishedAudioDemux = true;
				audioCnv.notify_one(); // notify converter thread if halted
				break;
			}

			std::unique_lock<std::mutex> rl(r);
			if (pausing) {
				pausedAudio = true;
				resumeWait.notify_all();
			} else if (resuming) {
				avformat_flush(inputFormatContext);
				avcodec_flush_buffers(inputCodecContext);
				pausedAudio = false;
				resumeWait.notify_all();
			}
			rl.unlock();
			read_frame = av_read_frame(inputFormatContext, in_packet.into()) >= 0;

			if (!pausedAudio) {
				if (pausing) pausedAudio = true;
				auto in_frame =
					Frame{inputCodecContext->frame_size, inputCodecContext->sample_fmt,
						inputCodecContext->channel_layout,
						0};

				// Send packet to decoder
				decode(inputCodecContext, in_packet.into(), in_frame.into(), &got_frame);

				// check if decoded frame is ready
				if (got_frame > 0) { // frame is ready
					//Update frame size to correct value if needed

					// Convert and write frames
					convertAndWriteAudioFrames(swrContext,
					                           outputCodecContext,
					                           inputCodecContext,
					                           audioStream,
					                           outputFormatContext,
					                           in_frame.into(),
					                           &pts,
					                           &wR,
					                           &r,
					                           &max_pts,
					                           &min_pts,
					                           &pausedAudio,
					                           resync_enabled.load());
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
		                               &wR);
	}
	catch (avException &e) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this, e] { handle_rec_error("audio", 1, e.what()); });
		t.detach();
	}
	catch (std::runtime_error &e) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this, e] { handle_rec_error("audio", 1, e.what()); });
		t.detach();
	}
	catch (std::exception &e) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this, e] { handle_rec_error("audio", 1, e.what()); });
		t.detach();
	}
	catch (...) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this] { handle_rec_error("audio", 1); });
		t.detach();
	}
}

void Recorder::CaptureVideoFrames() {
	try {
		auto inputFormatContext = format.inputContext.get_video();
		auto inputCodecContext = codec.inputContext.get_video();
		auto outputCodecContext = codec.outputContext.get_video();
		auto outputFormatContext = format.outputContext.get_video();
		auto videoStream = codec.streams.get_video();
		auto swsContext = rescaler.get_sws();

		int64_t count = 0;
		int frameNum = 0; // frame number in a second
		int got_frame = 0;
		bool first_pause_frame = true;
		bool first_resume_frame = true;

		auto read_frame = true;
		while (read_frame) {
			auto in_packet = Packet{};

			if (!capturing) {
				std::unique_lock<std::mutex> ul(vC);
				finishedVideoDemux = true;
				videoCnv.notify_one(); // notify converter thread if halted
				return;
			}
			if (stopped) {
				std::unique_lock<std::mutex> ul(vC);
				finishedVideoDemux = true;
				videoCnv.notify_one(); // notify converter thread if halted
				break;
			}

			std::unique_lock<std::mutex> rl(r);
			if (pausing) {
				pausedVideo = true;
				resumeWait.notify_all();
			} else if (resuming) {
				avformat_flush(inputFormatContext);
				avcodec_flush_buffers(inputCodecContext);
				pausedVideo = false;
				resumeWait.notify_all();
			}
			rl.unlock();
			read_frame = av_read_frame(inputFormatContext, in_packet.into()) >= 0;

			if (!pausedVideo) {
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
						                          &r,
						                          &max_pts,
						                          &min_pts,
						                          &pausedVideo,
						                          resync_enabled.load());
					}
				}
			}
		}

		// Handle delayed frames
		convertAndWriteDelayedVideoFrames(outputCodecContext,
		                                  videoStream,
		                                  outputFormatContext,
		                                  &wR);
	}
	catch (avException &e) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this, e] { handle_rec_error("video", 0, e.what()); });
		t.detach();
	}
	catch (std::runtime_error &e) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this, e] { handle_rec_error("video", 0, e.what()); });
		t.detach();
	}
	catch (std::exception &e) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this, e] { handle_rec_error("video", 0, e.what()); });
		t.detach();
	}
	catch (...) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this] { handle_rec_error("video", 0); });
		t.detach();
	}
}

/* multi thread (de)muxing */

void Recorder::DemuxAudioInput() {
	try {
		auto start = std::chrono::system_clock::now();
		auto end = start;
		bool synced = false;
		int result;

		auto inputFormatContext = format.inputContext.get_audio();
		auto inputCodecContext = codec.inputContext.get_audio();

		avformat_flush(inputFormatContext);
		auto read_packet = true;
		while (read_packet) {
			auto in_packet = Packet{};
			if (!capturing) {
				std::unique_lock<std::mutex> ul(aC);
				finishedAudioDemux = true;
				audioCnv.notify_one(); // notify converter thread if halted
				return;
			}
			if (stopped) {
				std::unique_lock<std::mutex> ul(aC);
				finishedAudioDemux = true;
				audioCnv.notify_one(); // notify converter thread if halted
				break;
			}

			std::unique_lock<std::mutex> rl(r);
			if (pausing) {
				pausedAudio = true;
				resumeWait.notify_all();
			} else if (resuming) {
				avformat_flush(inputFormatContext);
				avcodec_flush_buffers(inputCodecContext);
				pausedAudio = false;
				resumeWait.notify_all();
			}
			rl.unlock();
			read_packet = av_read_frame(inputFormatContext, in_packet.into()) >= 0;

			if (!pausedAudio) {
				end = std::chrono::system_clock::now();
				std::chrono::duration<double> elapsed_seconds = end - start;
				log_debug("Received audio packet after " + std::to_string(elapsed_seconds.count()) + " s");

				// Send packet to decoder
				std::unique_lock<std::mutex> ul(aC);
				result =
					avcodec_send_packet(inputCodecContext, in_packet.into()); // Try to send a packet without waiting
				if (result >= 0) {
					audioCnv.notify_one(); // notify converter thread if halted
					av_packet_unref(in_packet.into());
				}
				ul.unlock();

				// Check result
				if (result == AVERROR(EAGAIN)) {//buffer is full or could not acquire lock, wait and retry
					ul.lock();
					if (!capturing) {
						break;
					}
					audioCnv.notify_one(); // Send sync signal to converter thread
					audioCnv.wait(ul); // Wait for resume signal
					result = avcodec_send_packet(inputCodecContext, in_packet.into());
					if (result >= 0) {
						audioCnv.notify_one(); // notify converter thread if halted
					}
					ul.unlock();
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
	catch (avException &e) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this, e] { handle_rec_error("audio demux", 1, e.what()); });
		t.detach();
	}
	catch (std::runtime_error &e) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this, e] { handle_rec_error("audio demux", 1, e.what()); });
		t.detach();
	}
	catch (std::exception &e) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this, e] { handle_rec_error("audio demux", 1, e.what()); });
		t.detach();
	}
	catch (...) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this] { handle_rec_error("audio demux", 1); });
		t.detach();
	}
}

void Recorder::ConvertAudioFrames() {
	try {
		auto inputCodecContext = codec.inputContext.get_audio();
		auto outputCodecContext = codec.outputContext.get_audio();
		auto outputFormatContext = format.outputContext.get_audio();
		auto audioStream = codec.streams.get_audio();
		auto swrContext = rescaler.get_swr();

		int64_t pts = 0;
		int result = AVERROR(EAGAIN);
		int frame_size = 0;

		while (result >= 0 || result == AVERROR(EAGAIN)) {
			if (!capturing) {
				return;
			}
			auto in_frame = Frame{inputCodecContext->frame_size, inputCodecContext->sample_fmt,
				inputCodecContext->channel_layout, 0};

			std::unique_lock<std::mutex> ul(aC);
			result =
				avcodec_receive_frame(inputCodecContext, in_frame.into()); // Try to get a decoded frame without waiting
			if (result >= 0) {
				audioCnv.notify_one(); // Signal demuxer thread to resume if halted
			}
			ul.unlock();

			if (result >= 0) {
				//Update frame size to correct value if needed

				//Convert frames and then write them to file
				convertAndWriteAudioFrames(swrContext, outputCodecContext, inputCodecContext,
				                           audioStream, outputFormatContext, in_frame.into(),
				                           &pts, &wR, &r, &max_pts,
				                           &min_pts, &pausedAudio, resync_enabled.load());
			}

			if (result == AVERROR(EAGAIN)) {//buffer is not ready, wait and retry
				ul.lock();
				if (!capturing) {
					break;
				}
				if (!finishedAudioDemux.load()) {
					audioCnv.notify_one();// Signal demuxer thread if necessary
					audioCnv.wait(ul);// Wait for resume signal
					ul.unlock();
				} else {
					auto frame = Frame{inputCodecContext->frame_size, inputCodecContext->sample_fmt,
						inputCodecContext->channel_layout, 0};
					result = avcodec_receive_frame(inputCodecContext, frame.into()); // Try to get a decoded frame
					if (result == AVERROR(EAGAIN)) break;
					ul.unlock();
					if (result >= 0) {
						convertAndWriteAudioFrames(swrContext, outputCodecContext, inputCodecContext,
						                           audioStream, outputFormatContext, frame.into(),
						                           &pts, &wR, &r, &max_pts,
						                           &min_pts, &pausedAudio, resync_enabled.load());
					}
				}
			}

			if (result < 0 && result != AVERROR(EAGAIN)) {
				throw avException("Audio Converter/Writer threads syncronization error");
			}
		}

		if (result < 0 && result != AVERROR_EOF && result != AVERROR(EAGAIN)) {
			// Decoder error
			throw avException("Failed to receive decoded packet");
		}

		//Convert last frames and then write them to file
		convertAndWriteLastAudioFrames(swrContext,
		                               outputCodecContext,
		                               inputCodecContext,
		                               audioStream,
		                               outputFormatContext,
		                               &pts,
		                               &wR);
	}
	catch (avException &e) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this, e] { handle_rec_error("audio converter", 2, e.what()); });
		t.detach();
	}
	catch (std::runtime_error &e) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this, e] { handle_rec_error("audio converter", 2, e.what()); });
		t.detach();
	}
	catch (std::exception &e) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this, e] { handle_rec_error("audio converter", 2, e.what()); });
		t.detach();
	}
	catch (...) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this] { handle_rec_error("audio converter", 2); });
		t.detach();
	}
}

void Recorder::DemuxVideoInput() {
	try {
		// frame number in a second
		int frameNum = 0;
		int result;
		auto synced = false;
		bool first_pause_frame = true;
		bool first_resume_frame = true;
		auto start = std::chrono::system_clock::now();
		auto inputFormatContext = format.inputContext.get_video();
		auto inputCodecContext = codec.inputContext.get_video();

		auto read_frame = true;
		while (read_frame) {
			auto packet = Packet{};

			if (!capturing) {
				std::unique_lock<std::mutex> ul(vC);
				finishedVideoDemux = true;
				videoCnv.notify_one(); // notify converter thread if halted
				return;
			}
			if (stopped) {
				std::unique_lock<std::mutex> ul(vC);
				finishedVideoDemux = true;
				videoCnv.notify_one(); // notify converter thread if halted
				break;
			}

			std::unique_lock<std::mutex> rl(r);
			if (pausing) {
				pausedVideo = true;
				resumeWait.notify_all();
			} else if (resuming) {
				avformat_flush(inputFormatContext);
				avcodec_flush_buffers(inputCodecContext);
				pausedVideo = false;
				resumeWait.notify_all();
			}
			rl.unlock();
			read_frame = av_read_frame(inputFormatContext, packet.into()) >= 0;

			if (!pausedVideo) {
				if (frameNum == 30) {
					frameNum = 0; // reset every fps frames
					auto end = std::chrono::system_clock::now();
					std::chrono::duration<double> elapsed_seconds = end - start;
					log_debug("Received 30 video packets in " + std::to_string(elapsed_seconds.count()) + " s");
					start = std::chrono::system_clock::now();
				}

				// Send packet to decoder
				std::unique_lock<std::mutex> ul(vC);
				result = avcodec_send_packet(inputCodecContext, packet.into());// Try to send a packet without waiting
				if (result >= 0) {
					videoCnv.notify_one(); // notify converter thread if halted
					packet.unref();
				}
				ul.unlock();
				// Check result
				if (result == AVERROR(EAGAIN)) { //buffer is full or could not acquire lock, wait and retry
					ul.lock();
					if (!capturing) {
						break;
					}
					videoCnv.notify_one();// Send sync signal to converter thread
					videoCnv.wait(ul);// Wait for resume signal
					result = avcodec_send_packet(inputCodecContext, packet.into());
					if (result >= 0) {
						videoCnv.notify_one(); // notify converter thread if halted
					}
					ul.unlock();
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
	catch (avException &e) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this, e] { handle_rec_error("video demux", 3, e.what()); });
		t.detach();
	}
	catch (std::runtime_error &e) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this, e] { handle_rec_error("video demux", 3, e.what()); });
		t.detach();
	}
	catch (std::exception &e) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this, e] { handle_rec_error("video demux", 3, e.what()); });
		t.detach();
	}
	catch (...) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this] { handle_rec_error("video demux", 3); });
		t.detach();
	}
}

void Recorder::ConvertVideoFrames() {
	try {
		auto inputCodecContext = codec.inputContext.get_video();
		auto outputCodecContext = codec.outputContext.get_video();
		auto outputFormatContext = format.outputContext.get_video();
		auto videoStream = codec.streams.get_video();
		auto swsContext = rescaler.get_sws();

		int64_t count = 0;
		int result = AVERROR(EAGAIN);

		while (result >= 0 || result == AVERROR(EAGAIN)) {
			if (!capturing) {
				return;
			}
			auto in_frame = Frame{};
			std::unique_lock<std::mutex> ul(vC);
			result =
				avcodec_receive_frame(inputCodecContext, in_frame.into()); // Try to get a decoded frame without waiting
			if (result >= 0) {
				videoCnv.notify_one();// Signal demuxer thread to resume if halted
			}
			ul.unlock();

			if (result >= 0) {
				//Convert frames and then write them to file
				count++;
				convertAndWriteVideoFrame(swsContext, outputCodecContext, inputCodecContext,
				                          videoStream, outputFormatContext, in_frame.into(),
				                          &count, &wR, &r, &max_pts,
				                          &min_pts, &pausedVideo, resync_enabled.load());
			}
			if (result == AVERROR(EAGAIN)) {//buffer is not ready or could not acquire lock, wait and retry
				ul.lock();
				if (!capturing) {
					break;
				}
				if (!finishedVideoDemux) {
					videoCnv.notify_one();// Signal demuxer thread if necessary
					videoCnv.wait(ul);// Wait for resume signal
					ul.unlock();
				} else {
					auto frame = Frame{};
					result = avcodec_receive_frame(inputCodecContext, frame.into()); // Try to get a decoded frame
					if (result == AVERROR(EAGAIN)) break;
					ul.unlock();
					if (result >= 0) {
						//Convert frames and then write them to file
						count++;
						convertAndWriteVideoFrame(swsContext, outputCodecContext, inputCodecContext,
						                          videoStream, outputFormatContext, frame.into(),
						                          &count, &wR, &r, &max_pts,
						                          &min_pts, &pausedVideo, resync_enabled.load());
					}
				}
			}

			if (result < 0 && result != AVERROR_EOF && result != AVERROR(EAGAIN)) {
				throw avException("Audio Converter/Writer threads synchronization error");
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
		                                  &wR);
	}
	catch (avException &e) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this, e] { handle_rec_error("video converter", 4, e.what()); });
		t.detach();
	}
	catch (std::runtime_error &e) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this, e] { handle_rec_error("video converter", 4, e.what()); });
		t.detach();
	}
	catch (std::exception &e) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this, e] { handle_rec_error("video converter", 4, e.what()); });
		t.detach();
	}
	catch (...) {
		std::lock_guard<std::mutex> lg(eM);
		if (!capturing) return;
		capturing = false;
		std::thread t = std::thread([this] { handle_rec_error("video converter", 4); });
		t.detach();
	}
}





