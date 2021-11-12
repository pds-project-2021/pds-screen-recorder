#include "ScreenRecorder.h"
using namespace std;

#define AUDIO 1
#define AUDIO_INPUT 0
#define AUDIO_CHANNELS 1
#define AUDIO_SAMPLE_RATE 44100
#define VIDEO_MT 1
#define AUDIO_MT 1
#ifdef WIN32
#define VIDEO_CODEC 27 //27 H264; 2 MPEG2;
#else
#define VIDEO_CODEC 2 //MPEG2, but H264 can be used if libx264-dev is installed
#endif
#define VIDEO_BITRATE 8000000
//#define FRAME_COUNT 280
#define AUDIO_CODEC 86018 //86017 MP3; 86018 AAC;
#define AUDIO_BITRATE 128000



/* initialize the resources*/
ScreenRecorder::ScreenRecorder() {
//	av_register_all();
//	avcodec_register_all();
	avdevice_register_all();
//init variables for thread synchronization
    vD = new mutex;
    aD = new mutex;
    vP = new mutex;
    aP = new mutex;
    wR = new mutex;
    videoCnv = new condition_variable;
    audioCnv = new condition_variable;
    videoDmx = new condition_variable;
    audioDmx = new condition_variable;
    writeFrame = new condition_variable;
	recordVideo = false;
//    paused = new atomic_bool;
//    *paused = false;
    pausedVideo = new atomic_bool;
    *pausedVideo = false;
    pausedAudio = new atomic_bool;
    *pausedAudio = false;
    stopped = new atomic_bool;
    *stopped = false;
    frameCount = 0;
    audio_devices = new vector<string>;
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
//free allocated memory of variables for thread synchronization
    free(vD);
    free(aD);
    free(vP);
    free(aP);
    free(wR);
    free(videoCnv);
    free(audioCnv);
    free(videoDmx);
    free(audioDmx);
    free(writeFrame);
//    free(paused);
    free(pausedVideo);
    free(pausedAudio);
    free(stopped);
    free(audio_devices);
	cout << "clean all" << endl;
}

void show_avfoundation_device() {
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	AVDictionary *options = nullptr;
	av_dict_set(&options, "list_devices", "true", 0);
	AVInputFormat *iformat = av_find_input_format("avfoundation");
	printf("==AVFoundation Device Info===\n");
	avformat_open_input(&pFormatCtx, "", iformat, &options);
	printf("=============================\n");
}

#ifdef WIN32
HRESULT enumerateDshowDevices(REFGUID category, IEnumMoniker **ppEnum)
{
    // Create the System Device Enumerator.
    ICreateDevEnum *pDevEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr,
                                  CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

    if (SUCCEEDED(hr))
    {
        // Create an enumerator for the category.
        hr = pDevEnum->CreateClassEnumerator(category, ppEnum, 0);
        if (hr == S_FALSE)
        {
            hr = VFW_E_NOT_FOUND;  // The category is empty. Treat as an error.
        }
        pDevEnum->Release();
    }
    return hr;
}

void getDshowDeviceInformation(IEnumMoniker *pEnum, std::vector<std::string> *audioDevices) {
    IMoniker *pMoniker = nullptr;
    if (audioDevices == nullptr) throw avException("Devices names vector was not initialized");
    while (pEnum->Next(1, &pMoniker, nullptr) == S_OK) {
        IPropertyBag *pPropBag;
        HRESULT hr = pMoniker->BindToStorage(nullptr, nullptr, IID_PPV_ARGS(&pPropBag));
        if (FAILED(hr)) {
            pMoniker->Release();
            continue;
        }
        VARIANT var;
        VariantInit(&var);
        // Get description or friendly name.
        hr = pPropBag->Read(L"Description", &var, nullptr);
        if (FAILED(hr)) {
            hr = pPropBag->Read(L"FriendlyName", &var, nullptr);
        }
        if (SUCCEEDED(hr)) {
//            printf("%S\n", var.bstrVal);
            wstring ws(var.bstrVal);// Convert to wstring
            string device_name(ws.begin(), ws.end());// Convert to string
            audioDevices->push_back(device_name);// Add to device names vector
            VariantClear(&var);
        }
        pPropBag->Release();
        pMoniker->Release();
    }
}
#endif

int ScreenRecorder::init() {
	inputFormatContext = avformat_alloc_context(); // Allocate an AVFormatContext.
	audioInputFormatContext = avformat_alloc_context(); // Allocate an AVFormatContext.
	options = nullptr;

#ifdef _WIN32
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED); // Set COM to multithreaded model
    if (SUCCEEDED(hr))
    {
        IEnumMoniker *pEnum;
        //Get device enumerator for audio devices
        hr = enumerateDshowDevices(CLSID_AudioInputDeviceCategory, &pEnum);
        if (SUCCEEDED(hr))
        {
            //Get audio devices names
            getDshowDeviceInformation(pEnum, audio_devices);
            pEnum->Release();
        }
    }
    else throw avException("Error during thread initialization");
    // Prepare dshow command input audio device parameter string
    string audioInputName;
    audioInputName.append("audio=") ;
    auto curr_name = audio_devices->begin();
    for (int i = 0; curr_name != audio_devices->end(); i++) {// Select first available audio device
        if(i==AUDIO_INPUT) {
            audioInputName.append(curr_name->c_str());// Write value to string
            break;
        }
        curr_name++;
    }
	av_dict_set(&options, "rtbufsize", "3M", 0);
    audioInputFormat = make_unique<AVInputFormat>(*av_find_input_format("dshow"));
    av_dict_set(&options, "sample_rate", to_string(AUDIO_SAMPLE_RATE).c_str(), 0);
    av_dict_set(&options, "channels", to_string(AUDIO_CHANNELS).c_str(), 0);
    // Open audio input device
    cout << "Selected dshow audio input device: " << audioInputName.substr(6, 35) << endl;
	auto ret = avformat_open_input(&audioInputFormatContext,
                                   audioInputName.c_str(), audioInputFormat.get(),
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
	inputFormat = make_unique<AVInputFormat>(*av_find_input_format("gdigrab"));
	ret = avformat_open_input(&inputFormatContext, "desktop", inputFormat.get(), &options);
	if (ret != 0) {
	  throw avException("Couldn't open input stream");
	}
#elif defined linux
	//av_dict_set(&options, "rtbufsize", "10M", 0);
	audioInputFormat = make_unique<AVInputFormat>(*av_find_input_format("pulse"));
    av_dict_set(&options, "sample_rate", to_string(AUDIO_SAMPLE_RATE).c_str(), 0);
    av_dict_set(&options, "channels", to_string(AUDIO_CHANNELS).c_str(), 0);
	auto ret = avformat_open_input(&audioInputFormatContext, "default", audioInputFormat.get(), &options);
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

	inputFormat = make_unique<AVInputFormat>(*av_find_input_format("x11grab"));
	ret = avformat_open_input(&inputFormatContext, ":0.0", inputFormat.get(), &options);
    if (ret != 0) {
        throw avException("Couldn't open input stream");
    }
#else
	//show_avfoundation_device();
	inputFormat = make_unique<AVInputFormat>(*av_find_input_format("avfoundation"));
	av_dict_set(&options, "framerate", "30", 0);
	//av_dict_set(&options, "preset", "medium", 0);
	av_dict_set(&options, "offset_x", "0", 0);
	av_dict_set(&options, "offset_y", "0", 0);
	//	av_dict_set(&options, "video_size", "1920x1080", 0);
	//av_dict_set(&options, "capture_cursor", "1", 0);
	av_dict_set(&options, "pixel_format", "bgr0", 0);
	//av_dict_set(&options, "video_device_index", "1", 0);

	auto ret = avformat_open_input(&inputFormatContext, "1", inputFormat.get(), &options);
	if (ret != 0) {
	  throw avException("Couldn't open input stream");
	}
	audioInputFormat = make_unique<AVInputFormat>(*av_find_input_format("avfoundation"));
    av_dict_free(&options);
	av_dict_set(&options, "audio_device_index", "AUDIO_INPUT", 0);
	ret = avformat_open_input(&audioInputFormatContext, "", audioInputFormat.get(), &options);
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
	inputCodecPar->format = AV_PIX_FMT_BGR0;

	audioInputCodecPar = audioInputFormatContext->streams[audioIndex]->codecpar;
	audioInputCodecPar->format = AV_SAMPLE_FMT_S16;
	audioInputCodecPar->sample_rate = AUDIO_SAMPLE_RATE;
	audioInputCodecPar->channel_layout = AUDIO_CHANNELS==2?AV_CH_LAYOUT_STEREO:AV_CH_LAYOUT_MONO;
	audioInputCodecPar->channels = AUDIO_CHANNELS;
	audioInputCodecPar->codec_id = AV_CODEC_ID_PCM_S16LE;
	audioInputCodecPar->codec_type = AVMEDIA_TYPE_AUDIO;
	audioInputCodecPar->frame_size = 22050; // set number of audio samples in each frame
//    audioInputCodecPar->bit_rate = AUDIO_CHANNELS*705600;

	auto inputCodec = avcodec_find_decoder(inputCodecPar->codec_id);
	if (inputCodec == nullptr) {
		throw avException("Unable to find the video decoder");
	}

	auto audioInputCodec = avcodec_find_decoder(audioInputCodecPar->codec_id);
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
    CoUninitialize();
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

void ScreenRecorder::close() {
    CloseMediaFile();
    avcodec_close(inputCodecContext);
    avcodec_free_context(&inputCodecContext);
    avcodec_close(audioInputCodecContext);
    avcodec_free_context(&audioInputCodecContext);
    avformat_close_input(&inputFormatContext);
    avformat_free_context(inputFormatContext);
    avformat_close_input(&audioInputFormatContext);
    avformat_free_context(audioInputFormatContext);
    avcodec_close(outputCodecContext);
    avcodec_free_context(&outputCodecContext);
    avcodec_close(audioOutputCodecContext);
    avcodec_free_context(&audioOutputCodecContext);
    avformat_free_context(outputFormatContext);
    avformat_free_context(audioOutputFormatContext);
}

int ScreenRecorder::init_outputfile() {
	output_file = "../media/output.mp4";
//    frameCount = FRAME_COUNT;
	auto outputCodec = avcodec_find_encoder((AVCodecID) VIDEO_CODEC);
	if (!outputCodec) {
		throw avException(
			"Error in finding the video av codecs. try again with correct codec");
	}
	auto audioOutputCodec = avcodec_find_encoder((AVCodecID) AUDIO_CODEC);
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
	outputFormat = make_unique<AVOutputFormat>(*av_guess_format(nullptr, output_file, nullptr));
	if (!outputFormat.get()) {
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
	outputCodecContext->gop_size = 15;
	outputCodecContext->max_b_frames = 10;
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

void init_video_frame(AVFrame *frame, int width, int height, AVPixelFormat format, int align) {
    if (frame == nullptr) {
        frame = av_frame_alloc();
        if (!frame) {
            throw avException("Unable to release the audio avframe resources");
        }
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
}

void init_audio_frame(AVFrame *audioFrame, int nb_samples, AVSampleFormat format, uint64_t channel_layout, int align) {
    if (audioFrame == nullptr) {
        audioFrame = av_frame_alloc();
        if (!audioFrame) {
            throw avException("Unable to release the audio avframe resources");
        }
    }
    audioFrame->nb_samples = nb_samples;
    audioFrame->format = format;
    audioFrame->channel_layout = channel_layout;
    if (av_frame_get_buffer(audioFrame, align) < 0) {
        throw avException("Could not allocate audio data buffers");
    }
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
	if (pkt != nullptr) {
		// Send packet to decoder
		result = avcodec_send_packet(avctx, pkt);
		// In particular, we don't expect AVERROR(EAGAIN), because we read all
		// decoded frames with avcodec_receive_frame() until done.
		// Check result
		if (result < 0 && result != AVERROR_EOF) {
			// Decoder error
			throw avException("Failed to send packet to decoder");
//			return result;
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


void pauseStream(mutex *m, condition_variable *cv) {
    unique_lock<mutex> ul(*m);
    cv->wait(ul);// Wait for resume signal
}
bool ScreenRecorder::isPaused() {
    if(*pausedVideo) return true;
    return false;

}

void ScreenRecorder::PauseCapture(){
//    *paused = true;
//    *pausedVideo = true;
    *pausedAudio = true;
//    while(*pausedVideo || *pausedAudio) {
//        this_thread::sleep_for(std::chrono::milliseconds (25));
//    }
}

void ScreenRecorder::ResumeCapture(){
//    *paused = false;
    avformat_flush(inputFormatContext);
    avformat_flush(audioInputFormatContext);
    *pausedVideo = false;
    *pausedAudio = false;
//    while(!vP->try_lock()) {
//        this_thread::sleep_for(std::chrono::milliseconds (5));
//    }
//    videoDmx->notify_one();// Send sync signal to handler thread
//    vP->unlock();
//    while(!aP->try_lock()) {
//        this_thread::sleep_for(std::chrono::milliseconds (5));
//    }
//    audioDmx->notify_one();// Send sync signal to handler thread
//    aP->unlock();

}

int ScreenRecorder::CloseMediaFile() {
    *stopped = true;
#if (VIDEO_MT==1)
    videoDemux->join();
    unique_lock<mutex> ulvC(*vD);
    finishedVideoDemux = true;
    videoCnv->notify_one();// Send sync signal to video converter thread
    videoCnv->wait(ulvC);// Wait for resume signal
    videoCnv->notify_one();// Send sync signal to video converter thread
    videoConvert->join();
#else
    video->join();
#endif
#if (AUDIO==1)
#if (AUDIO_MT==1)
    audioDemux->join();
    unique_lock<mutex> ulC(*aD);
    finishedAudioDemux = true;
    audioCnv->notify_one();// Send sync signal to converter thread
    audioCnv->wait(ulC);// Wait for resume signal
    audioCnv->notify_one();// Send sync signal to converter thread
    audioConvert->join();
#else
    audio->join();
#endif
#endif
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
//#ifdef WIN32
//    CoUninitialize();
//#endif
    *stopped = false;
    return ret;
}

int ScreenRecorder::initThreads() {
    *stopped = false;
//    *paused = false;
    *pausedVideo = false;
    *pausedAudio = false;
#if (VIDEO_MT==1)
    finishedVideoDemux = false;
    videoDemux = new thread(&ScreenRecorder::DemuxVideoInput, this);
    videoConvert = new thread(&ScreenRecorder::ConvertVideoFrames, this);
#else
	video = new thread(&ScreenRecorder::CaptureVideoFrames, this);
#endif
#if (AUDIO==1)
#if (AUDIO_MT==1)
    finishedAudioDemux = false;
    audioDemux = new thread(&ScreenRecorder::DemuxAudioInput, this);
    audioConvert = new thread(&ScreenRecorder::ConvertAudioFrames, this);
#else
    audio = new thread(&ScreenRecorder::CaptureAudioFrames, this);
#endif
#endif
	return 0;
}

int ScreenRecorder::CaptureStart() { return initThreads(); }

void writeFrameToOutput(AVFormatContext *outputFormatContext, AVPacket *outPacket, mutex *wR, condition_variable *writeFrame) {
    int result= 0;
    if(outputFormatContext == nullptr || outPacket == nullptr){
        throw avException("Provided null output values, data could not be written");
    }
    if(wR->try_lock()) {
        result = av_write_frame(outputFormatContext,
                                outPacket); // Write packet to file
        if (result < 0) throw avException("Error in writing video frame");
        else writeFrame->notify_one(); // Notify other writer thread to resume if halted
        wR->unlock();
    }
    else {//could not acquire lock, wait and retry
        unique_lock<mutex> ul(*wR);
//        writeFrame->notify_one();// Notify other writer thread to resume if halted
        writeFrame->wait(ul);// Wait for resume signal
        result = av_write_frame(outputFormatContext,
                                outPacket); // Write packet to file
        if (result < 0) throw avException("Error in writing video frame");
        else writeFrame->notify_one(); // Notify other writer thread to resume if halted
    }
}

static int convertAndWriteVideoFrame(SwsContext *swsContext, AVCodecContext *outputCodecContext, AVCodecContext *inputCodecContext, AVStream *videoStream,
                                     AVFormatContext *outputFormatContext, AVFrame *frame, int64_t *pts_p, mutex *wR, condition_variable *writeFrame) {
    // Create encoder video frame
    AVFrame *outputFrame = alloc_video_frame(outputCodecContext->width, outputCodecContext->height,
                                                                        (AVPixelFormat) outputCodecContext->pix_fmt, 32);
//    AVFrame *audioOutputFrame = alloc_audio_frame(
//            audioOutputCodecContext->frame_size, audioOutputCodecContext->sample_fmt,
//            audioOutputCodecContext->channel_layout, 0);
    // Create encoder audio packet
    AVPacket *outputPacket =  alloc_packet();
//    AVPacket *audioOutputPacket = alloc_packet();
    int got_packet = 0;
    // Convert frame picture format
    sws_scale(swsContext, frame->data, frame->linesize, 0,
              inputCodecContext->height, outputFrame->data,
              outputFrame->linesize);
    // Send converted frame to encoder
    outputFrame->pts = *pts_p -1;
    encode(outputCodecContext, outputPacket, &got_packet, outputFrame);
    // Frame was sent successfully
    if (got_packet > 0) { // Packet received successfully
        if (outputPacket->pts != AV_NOPTS_VALUE) {
            outputPacket->pts =
                    av_rescale_q(outputPacket->pts, outputCodecContext->time_base,
                                 videoStream->time_base);
        }
        if (outputPacket->dts != AV_NOPTS_VALUE) {
            outputPacket->dts =
                    av_rescale_q(outputPacket->dts, outputCodecContext->time_base,
                                 videoStream->time_base);
        }
        outputPacket->duration =
                av_rescale_q(1, outputCodecContext->time_base,
                             videoStream->time_base);
        // Write packet to file
        writeFrameToOutput(outputFormatContext, outputPacket, wR, writeFrame);
    }
    av_packet_unref(outputPacket);
    av_frame_unref(outputFrame);
    av_packet_free(&outputPacket);
    av_frame_free(&outputFrame);
    return 0;
}

static int convertAndWriteDelayedVideoFrames(AVCodecContext *outputCodecContext, AVStream *videoStream,
                                             AVFormatContext *outputFormatContext, mutex *wR, condition_variable *writeFrame) {
    // Create encoder audio packet
    AVPacket *outPacket = alloc_packet();
    for (int result;;) {
        avcodec_send_frame(outputCodecContext, nullptr);
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
            writeFrameToOutput(outputFormatContext, outPacket, wR, writeFrame);
            av_packet_unref(outPacket);
        } else { // No remaining frames to handle
            break;
        }
    }
    av_packet_unref(outPacket);
    av_packet_free(&outPacket);
    return 0;
}

static int convertAndWriteDelayedAudioFrames(AVCodecContext *inputCodecContext, AVCodecContext *outputCodecContext, AVStream *audioStream,
                                             AVFormatContext *outputFormatContext, int finalSize, mutex *wR, condition_variable *writeFrame) {
    // Create encoder audio packet
    AVPacket *outPacket = alloc_packet();
    AVPacket *nextOutPacket = alloc_packet();
    AVPacket *prevPacket = nullptr;
    int result;
    avcodec_send_frame(outputCodecContext, nullptr);
    auto receive = avcodec_receive_packet(outputCodecContext, outPacket);
    while (receive >= 0) {// Try to get packet
        avcodec_send_frame(outputCodecContext, nullptr);
        receive = avcodec_receive_packet(outputCodecContext, nextOutPacket);
        if(receive>=0) {//if this is not the last packet
            if (outPacket->pts != AV_NOPTS_VALUE) {
                outPacket->pts =
                        av_rescale_q(outPacket->pts,
                                     {1,
                                      inputCodecContext->sample_rate *
                                      inputCodecContext->channels},
                                     audioStream->time_base);
            }
            if (outPacket->dts != AV_NOPTS_VALUE) {
                outPacket->dts =
                        av_rescale_q(outPacket->dts,
                                     {1,
                                      inputCodecContext->sample_rate *
                                      inputCodecContext->channels},
                                     audioStream->time_base);
            }//frame size equals codec frame size
            outPacket->duration =
                    av_rescale_q(outputCodecContext->frame_size,
                                 {1,
                                  inputCodecContext->sample_rate *
                                  inputCodecContext->channels},
                                 audioStream->time_base);
            // Write packet to file
            outPacket->stream_index = 1;
            writeFrameToOutput(outputFormatContext, outPacket, wR, writeFrame);
            av_packet_unref(outPacket);
            prevPacket = outPacket;
            outPacket = nextOutPacket;
            nextOutPacket =prevPacket;
        }
        else {//if this is the last packet
            if (outPacket->pts != AV_NOPTS_VALUE) {
                outPacket->pts =
                        av_rescale_q(outPacket->pts,
                                     {1,
                                      inputCodecContext->sample_rate *
                                      inputCodecContext->channels},
                                     audioStream->time_base);
            }
            if (outPacket->dts != AV_NOPTS_VALUE) {
                outPacket->dts =
                        av_rescale_q(outPacket->dts,
                                     {1,
                                      inputCodecContext->sample_rate *
                                      inputCodecContext->channels},
                                     audioStream->time_base);
            }//frame size equals finalSize
            outPacket->duration =
                    av_rescale_q(finalSize,
                                 {1,
                                  inputCodecContext->sample_rate *
                                  inputCodecContext->channels},
                                 audioStream->time_base);
            // Write packet to file
            outPacket->stream_index = 1;
            writeFrameToOutput(outputFormatContext, outPacket, wR, writeFrame);
            cout << "Final pts value is: " << outPacket->pts << endl;
            av_packet_unref(outPacket);
            av_packet_unref(nextOutPacket);
            av_packet_free(&outPacket);
            av_packet_free(&nextOutPacket);
        }
    }
    return 0;
}

void ScreenRecorder::CaptureVideoFrames() {
	// video thread started
	recordVideo = true;
	// Create decoder frame
	AVFrame *frame =
		alloc_video_frame(inputCodecPar->width, inputCodecPar->height,
		                  (AVPixelFormat) inputCodecPar->format, 32);
	// init cycle variables
    int64_t count = 0;
	int frameNum = 0; // frame number in a second
	int got_frame = 0;
	// Create decoder packet
	AVPacket *packet = alloc_packet();
	// Try to extract packet from input stream
    avformat_flush(inputFormatContext);
	while (av_read_frame(inputFormatContext, packet) >= 0) {
        if (*stopped) {
            break;
        }
//        if(*pausedVideo) {// Check if capture has been paused
//            unique_lock<mutex> ul(*vP);
//            *pausedVideo = false;// Sync signal to handler thread
//            videoDmx->wait(ul);// Wait for resume signal
//        }
        if(!*pausedVideo) {
            if (frameNum++ == 30)
                frameNum = 0; // reset every fps frames

            if (packet->stream_index == videoStream->index) {
                // Send packet to decoder
                decode(inputCodecContext, frame, &got_frame, packet);

                // check if decoded frame is ready
                if (got_frame) { // frame is ready
                    // Convert frame picture format and write frame to file
                    count++;
                    convertAndWriteVideoFrame(swsContext, outputCodecContext, inputCodecContext, videoStream, outputFormatContext, frame, &count, wR, writeFrame);
                    av_frame_unref(frame);
                    init_video_frame(frame, inputCodecPar->width, inputCodecPar->height,
                                     (AVPixelFormat) inputCodecPar->format, 32);
                }
            }
        }
        av_packet_unref(packet);
	} // End of while-loop
	// Handle delayed frames
    convertAndWriteDelayedVideoFrames(outputCodecContext, videoStream, outputFormatContext, wR, writeFrame);
    av_frame_unref(frame);
    av_packet_unref(packet);
    av_frame_free(&frame);
    av_packet_free(&packet);
}

void ScreenRecorder::DemuxVideoInput() {
    // Create decoder video packet
    AVPacket *packet = alloc_packet();
//    int count = 0;
    int frameNum = 0; // frame number in a second
    int result;
    auto start = std::chrono::system_clock::now();
    avformat_flush(inputFormatContext);
    while (av_read_frame(inputFormatContext, packet) >= 0) {
        if (*stopped) {
            break;
        }
//        if(*pausedVideo) {// Check if capture has been paused
//            unique_lock<mutex> ul(*vP);
//            *pausedVideo = false;// Sync signal to handler thread
//            videoDmx->wait(ul);// Wait for resume signal
//        }
        if(!*pausedVideo) {
            if (frameNum == 30) {
                frameNum = 0; // reset every fps frames
                auto end = std::chrono::system_clock::now();
                std::chrono::duration<double> elapsed_seconds = end-start;
                std::cout << "Received 30 video packets in " << elapsed_seconds.count() << " s\n";
                start = std::chrono::system_clock::now();
            }
            // Send packet to decoder
            if(vD->try_lock()) {
                result = avcodec_send_packet(inputCodecContext, packet);// Try to send a packet without waiting
                if (result >= 0) {
                    videoCnv->notify_one(); // notify converter thread if halted
                    av_packet_unref(packet);
                }
                vD->unlock();
            }
            else result = AVERROR(EAGAIN);
            // Check result
            if(result == AVERROR(EAGAIN)) {//buffer is full or could not acquire lock, wait and retry
                unique_lock<mutex> ul(*vD);
                videoCnv->notify_one();// Send sync signal to converter thread
                videoCnv->wait(ul);// Wait for resume signal
                result = avcodec_send_packet(inputCodecContext, packet);
                if (result >= 0) {
                    videoCnv->notify_one(); // notify converter thread if halted
                    av_packet_unref(packet);
                }
            }
            if (result < 0 && result != AVERROR_EOF && result != AVERROR(EAGAIN)) {
                // Decoder error
                throw avException("Failed to send packet to decoder");
            }
            //Packet sent
            frameNum++;
        }
        else av_packet_unref(packet);
    }
    //Free allocated memory
    av_packet_unref(packet);
    av_packet_free(&packet);
}

void ScreenRecorder::ConvertVideoFrames() {
    // Create decoder audio frame
    AVFrame *frame = alloc_video_frame(inputCodecPar->width, inputCodecPar->height,
                                                         (AVPixelFormat) inputCodecPar->format, 32);
    // Create encoder audio frame
    int64_t count = 0;
    int result = AVERROR(EAGAIN);
    if(vD->try_lock()) {
        result = avcodec_receive_frame(inputCodecContext, frame); // Try to get a decoded frame without waiting
        if(result>=0) videoCnv->notify_one();// Signal demuxer thread to resume if halted
        vD->unlock();
    }
    else result = AVERROR(EAGAIN);
    while(result >= 0 || result == AVERROR(EAGAIN)) {
        if(result >= 0) {
            //Convert frames and then write them to file
            count++;
            convertAndWriteVideoFrame(swsContext, outputCodecContext, inputCodecContext, videoStream, outputFormatContext, frame, &count, wR, writeFrame);
            av_frame_unref(frame);
            init_video_frame(frame, inputCodecPar->width, inputCodecPar->height,
                             (AVPixelFormat) inputCodecPar->format, 32);
        }
        if(vD->try_lock()) {
            result = avcodec_receive_frame(inputCodecContext, frame); // Try to get a decoded frame without waiting
            if(result>=0) videoCnv->notify_one();// Signal demuxer thread to resume if halted
            vD->unlock();
        }
        else result = AVERROR(EAGAIN);
        if(result == AVERROR(EAGAIN)) {//buffer is not ready or could not acquire lock, wait and retry
            std::unique_lock<std::mutex> ul(*vD);
            videoCnv->notify_one();// Signal demuxer thread if necessary
            videoCnv->wait(ul);// Wait for resume signal
            if(finishedVideoDemux) {
                //Convert last frame and then write it to file
                convertAndWriteDelayedVideoFrames(outputCodecContext, videoStream, outputFormatContext, wR, writeFrame);
                videoCnv->notify_one();// Sync with main thread if necessary
                break;
            }
            result = avcodec_receive_frame(inputCodecContext, frame); // Try to get a decoded frame
            if(result>=0) videoCnv->notify_one();// Signal demuxer thread to resume if halted
        }
        if(result < 0 && result != AVERROR_EOF && result != AVERROR(EAGAIN)) throw avException("Audio Converter/Writer threads syncronization error");
    }
    if (result < 0 && result != AVERROR_EOF && result != AVERROR(EAGAIN)) {
        // Decoder error
        throw avException("Failed to receive decoded packet");
    }
    // Free allocated memory
    av_frame_unref(frame);
    av_frame_free(&frame);
}

static int convertAndWriteAudioFrames(SwrContext *swrContext, AVCodecContext *audioOutputCodecContext, AVCodecContext *audioInputCodecContext, AVStream *audioStream, AVFormatContext *outputFormatContext, AVFrame *audioFrame, int64_t *pts_p, mutex *wR, condition_variable *writeFrame) {
    // Create encoder audio frame
//    auto audioOutputFrame = make_unique<AVFrame>(*alloc_audio_frame(
//            audioOutputCodecContext->frame_size, audioOutputCodecContext->sample_fmt,
//            audioOutputCodecContext->channel_layout, 0));
    AVFrame *audioOutputFrame = alloc_audio_frame(
            audioOutputCodecContext->frame_size, audioOutputCodecContext->sample_fmt,
            audioOutputCodecContext->channel_layout, 0);
    // Create encoder audio packet
//    auto audioOutputPacket = make_unique<AVPacket>(*alloc_packet());
    AVPacket *audioOutputPacket = alloc_packet();
    int got_packet = 0;
    int got_samples = swr_convert(
            swrContext, audioOutputFrame->data, audioOutputFrame->nb_samples,
            (const uint8_t **) audioFrame->data, audioFrame->nb_samples);
    if (got_samples < 0) {
        throw avException("error: swr_convert()");
    }
    else if (got_samples > 0) {
        *pts_p += got_samples;
        audioOutputFrame->nb_samples=got_samples;
        audioOutputFrame->pts = *pts_p;
        encode(audioOutputCodecContext, audioOutputPacket, &got_packet,
               audioOutputFrame);
        av_frame_unref(audioOutputFrame);
        init_audio_frame(audioOutputFrame, audioOutputCodecContext->frame_size, audioOutputCodecContext->sample_fmt,
                         audioOutputCodecContext->channel_layout, 0);
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
            audioOutputPacket->duration =
                    av_rescale_q(audioOutputCodecContext->frame_size,
                                 {1,
                                  audioInputCodecContext->sample_rate *
                                  audioInputCodecContext->channels},
                                 audioStream->time_base);
            // Write packet to file
            audioOutputPacket->stream_index = 1;
            writeFrameToOutput(outputFormatContext, audioOutputPacket, wR, writeFrame);
            av_packet_unref(audioOutputPacket);
        }
    }
    while (swr_get_out_samples(swrContext, 0) >= audioOutputCodecContext->frame_size) {
        got_samples = swr_convert(swrContext, audioOutputFrame->data, audioOutputFrame->nb_samples, nullptr, 0);
        *pts_p += got_samples;
        //audioOutputFrame->nb_samples=got_samples;
        audioOutputFrame->pts = *pts_p;
        encode(audioOutputCodecContext, audioOutputPacket, &got_packet,
               audioOutputFrame);
        av_frame_unref(audioOutputFrame);
        init_audio_frame(audioOutputFrame, audioOutputCodecContext->frame_size, audioOutputCodecContext->sample_fmt,
                         audioOutputCodecContext->channel_layout, 0);
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
            audioOutputPacket->duration =
                    av_rescale_q(audioOutputCodecContext->frame_size,
                                 {1,
                                  audioInputCodecContext->sample_rate *
                                  audioInputCodecContext->channels},
                                 audioStream->time_base);
            // Write packet to file
            audioOutputPacket->stream_index = 1;
            writeFrameToOutput(outputFormatContext, audioOutputPacket, wR, writeFrame);
            av_packet_unref(audioOutputPacket);
        }
    }
    av_packet_unref(audioOutputPacket);
    av_frame_unref(audioOutputFrame);
    av_frame_free(&audioOutputFrame);
    av_packet_free(&audioOutputPacket);
    return 0;
}

static int convertAndWriteLastAudioFrames(SwrContext *swrContext, AVCodecContext *audioOutputCodecContext, AVCodecContext *audioInputCodecContext, AVStream *audioStream, AVFormatContext *outputFormatContext, int64_t *pts_p, mutex *wR, condition_variable *writeFrame) {
    // Create encoder audio frame
//    auto audioOutputFrame = make_unique<AVFrame>(*alloc_audio_frame(
//            audioOutputCodecContext->frame_size, audioOutputCodecContext->sample_fmt,
//            audioOutputCodecContext->channel_layout, 0));
    AVFrame *audioOutputFrame = alloc_audio_frame(
            audioOutputCodecContext->frame_size, audioOutputCodecContext->sample_fmt,
            audioOutputCodecContext->channel_layout, 0);
    // Create encoder audio packet
//    auto audioOutputPacket = make_unique<AVPacket>(*alloc_packet());
    AVPacket *audioOutputPacket = alloc_packet();
    int got_packet = 0;
    int got_samples = swr_convert(swrContext, audioOutputFrame->data, audioOutputFrame->nb_samples, nullptr, 0);
    if (got_samples < 0) {
        throw avException("error: swr_convert()");
    }
    else if (got_samples > 0) {
        *pts_p += got_samples;
        audioOutputFrame->nb_samples=got_samples;
        audioOutputFrame->pts = *pts_p;
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
            audioOutputPacket->duration =
                    av_rescale_q(audioOutputFrame->nb_samples,
                                 {1,
                                  audioInputCodecContext->sample_rate *
                                  audioInputCodecContext->channels},
                                 audioStream->time_base);
            // Write packet to file
            audioOutputPacket->stream_index = 1;
            auto result = av_write_frame(outputFormatContext, audioOutputPacket);
            if (result != 0) {
                throw avException("Error in writing video frame");
            }
        }
        convertAndWriteDelayedAudioFrames(audioInputCodecContext, audioOutputCodecContext, audioStream, outputFormatContext, got_samples, wR, writeFrame);
    }
    av_packet_unref(audioOutputPacket);
    av_frame_unref(audioOutputFrame);
    av_packet_free(&audioOutputPacket);
    av_frame_free(&audioOutputFrame);
    return 0;
}

void ScreenRecorder::CaptureAudioFrames() {
	// audio thread started
	// Create decoder audio frame
	AVFrame *audioFrame =
		alloc_audio_frame( 22050, audioInputCodecContext->sample_fmt,
		                  audioInputCodecContext->channel_layout, 0);
    int frame_size = 0;
	// Create decoder audio packet
	AVPacket *audioPacket = alloc_packet();
	int got_frame = 0;
    int64_t pts = 0;
	// Handle audio input stream packets
    avformat_flush(audioInputFormatContext);
	while (av_read_frame(audioInputFormatContext, audioPacket) >= 0) {
        if (*stopped) {
            break;
        }
//        if(*pausedAudio) {// Check if capture has been paused
//            unique_lock<mutex> ul(*aP);
//            *pausedAudio = false;// Sync signal to handler thread
//            audioDmx->wait(ul);// Wait for resume signal
//        }
        if(!*pausedAudio) {
            // Send packet to decoder
            decode(audioInputCodecContext, audioFrame, &got_frame, audioPacket);
            // check if decoded frame is ready
            if (got_frame > 0) { // frame is ready
                // Convert and write frames
                if(frame_size == 0) frame_size = audioFrame->nb_samples;
                convertAndWriteAudioFrames(swrContext, audioOutputCodecContext, audioInputCodecContext, audioStream, outputFormatContext, audioFrame, &pts, wR, writeFrame);
                av_frame_unref(audioFrame);
                init_audio_frame(audioFrame,  frame_size, audioInputCodecContext->sample_fmt,
                                 audioInputCodecContext->channel_layout, 0);
            } else
                throw avException("Failed to decode packet");
        }
        else if(!*pausedVideo) *pausedVideo = true;
        av_packet_unref(audioPacket);
        if (*stopped) {
            break;
        }
	}
    // Convert and write last frame
    convertAndWriteLastAudioFrames(swrContext, audioOutputCodecContext, audioInputCodecContext, audioStream, outputFormatContext, &pts, wR, writeFrame);
    av_packet_unref(audioPacket);
    av_frame_unref(audioFrame);
    av_frame_free(&audioFrame);
    av_packet_free(&audioPacket);
}

void ScreenRecorder::DemuxAudioInput(){
    // Create decoder audio packet
    AVPacket *audioPacket = alloc_packet();
    int result;
//    int count = 0;
//    double avsyncD = (frameCount+1.00)/30*AUDIO_SAMPLE_RATE/audioInputCodecContext->frame_size;
//    int avsyncI = (int) (frameCount+1.00)/30*AUDIO_SAMPLE_RATE/audioInputCodecContext->frame_size;
//    int audioCount = avsyncI+((avsyncD-avsyncI)>=0.5?1:0);
    auto start = std::chrono::system_clock::now();
    auto end = start;
    avformat_flush(audioInputFormatContext);
    while (av_read_frame(audioInputFormatContext, audioPacket) >= 0) {
        if (*stopped) {
            break;
        }
//        if(*pausedAudio) {// Check if capture has been paused
//            unique_lock<mutex> ul(*aP);
//            *pausedAudio = false;// Sync signal to handler thread
//            audioDmx->wait(ul);// Wait for resume signal
//        }
        if(!*pausedAudio) {
            end = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsed_seconds = end-start;
            std::cout << "Received audio packet after " << elapsed_seconds.count() << " s\n";
            //audioCnv.notify_one(); //signal converting thread to start if needed
            // Send packet to decoder
            if(aD->try_lock()) {
                result = avcodec_send_packet(audioInputCodecContext, audioPacket);// Try to send a packet without waiting
                if (result >= 0) {
                    audioCnv->notify_one(); // notify converter thread if halted
                    av_packet_unref(audioPacket);
                }
                aD->unlock();
            }
            else result = AVERROR(EAGAIN);
            // Check result
            if(result == AVERROR(EAGAIN)) {//buffer is full or could not acquire lock, wait and retry
                unique_lock<mutex> ul(*aD);
                audioCnv->notify_one();// Send sync signal to converter thread
                audioCnv->wait(ul);// Wait for resume signal
                result = avcodec_send_packet(audioInputCodecContext, audioPacket);
                if (result >= 0) {
                    audioCnv->notify_one(); // notify converter thread if halted
                    av_packet_unref(audioPacket);
                }
            }
            if (result < 0 && result != AVERROR_EOF && result != AVERROR(EAGAIN)) {
                // Decoder error
                throw avException("Failed to send packet to decoder");
            }
            //Packet sent
            start = std::chrono::system_clock::now();
        }
        else {
            av_packet_unref(audioPacket);
            if(!*pausedVideo) *pausedVideo = true;
        }
        if (*stopped) {
            break;
        }
    }
    //Free allocated memory
    av_packet_unref(audioPacket);
    av_packet_free(&audioPacket);
}

void ScreenRecorder::ConvertAudioFrames() {
    // Create decoder audio frame
    AVFrame *audioFrame =alloc_audio_frame( audioInputCodecContext->frame_size, audioInputCodecContext->sample_fmt,
                                                      audioInputCodecContext->channel_layout, 0);
    // Create encoder audio frame
    int frame_size = 0;
    int64_t pts = 0;
    int result = AVERROR(EAGAIN);
    if(aD->try_lock()) {
        result = avcodec_receive_frame(audioInputCodecContext, audioFrame); // Try to get a decoded frame without waiting
        if(result>=0) audioCnv->notify_one();// Signal demuxer thread to resume if halted
        aD->unlock();
    }
    else result = AVERROR(EAGAIN);
    while(result >= 0 || result == AVERROR(EAGAIN)) {
        if(result >= 0) {
            //Convert frames and then write them to file
            if(frame_size == 0) frame_size = audioFrame->nb_samples;
            convertAndWriteAudioFrames(swrContext, audioOutputCodecContext, audioInputCodecContext, audioStream, outputFormatContext, audioFrame, &pts, wR, writeFrame);
            av_frame_unref(audioFrame);
            init_audio_frame(audioFrame, frame_size, audioInputCodecContext->sample_fmt,
                             audioInputCodecContext->channel_layout, 0);
        }
        if(aD->try_lock()) {
            result = avcodec_receive_frame(audioInputCodecContext, audioFrame); // Try to get a decoded frame without waiting
            if(result>=0) audioCnv->notify_one();// Signal demuxer thread to resume if halted
            aD->unlock();
        }
        else result = AVERROR(EAGAIN);
        if(result == AVERROR(EAGAIN)) {//buffer is not ready or could not acquire lock, wait and retry
            std::unique_lock<std::mutex> ul(*aD);
            audioCnv->notify_one();// Signal demuxer thread if necessary
            audioCnv->wait(ul);// Wait for resume signal
            if(finishedAudioDemux) {
                //Convert last frame and then write it to file
                convertAndWriteLastAudioFrames(swrContext, audioOutputCodecContext, audioInputCodecContext, audioStream, outputFormatContext, &pts, wR, writeFrame);
                audioCnv->notify_one();// Sync with main thread if necessary
                break;
            }
            result = avcodec_receive_frame(audioInputCodecContext, audioFrame); // Try to get a decoded frame
            if(result>=0) audioCnv->notify_one();// Signal demuxer thread to resume if halted
        }
        if(result < 0 && result != AVERROR(EAGAIN)) throw avException("Audio Converter/Writer threads syncronization error");
    }
    if (result < 0 && result != AVERROR_EOF && result != AVERROR(EAGAIN)) {
        // Decoder error
        throw avException("Failed to receive decoded packet");
    }
    // Free allocated memory
    av_frame_unref(audioFrame);
    av_frame_free(&audioFrame);
}

//void ScreenRecorder::WriteAudioOutput(AVFormatContext *formatContext, AVRational bq, AVRational cq) {
//    // Create encoder audio packet
//    auto outputPacket = make_unique<AVPacket>(*alloc_packet());
//    int result = AVERROR(EAGAIN);
//    if(aW.try_lock()) {
//        result = avcodec_receive_packet(audioOutputCodecContext, outputPacket.get()); // Try to receive a new packet without waiting
//        if (result >= 0) audioWrt.notify_one();// Signal converter thread to resume if necessary
//        aW.unlock();
//    }
//    else {
//        unique_lock<mutex> ul(aW);
//        audioWrt.notify_one();// Sync with converter thread if necessary
//        audioWrt.wait(ul);// Wait for sync signal from converter thread
//        result = avcodec_receive_packet(audioOutputCodecContext, outputPacket.get()); // Try to receive a new packet
//        if (result >= 0) audioWrt.notify_one();// Signal converter thread to resume if necessary
//    }
//    while (result >= 0 || result == AVERROR(EAGAIN)) {
//        if (result >= 0) {
//            if (outputPacket->pts != AV_NOPTS_VALUE) {
//                outputPacket->pts = av_rescale_q(outputPacket->pts, bq, cq);
//            }
//            if (outputPacket->dts != AV_NOPTS_VALUE) {
//                outputPacket->dts = av_rescale_q(outputPacket->dts, bq, cq);
//            }
//            outputPacket->duration = av_rescale_q(1, bq, cq);
//            // Write packet to file
//            outputPacket->stream_index = 1;
//            result = av_write_frame(formatContext, outputPacket.get());
//            if (result != 0) {
//                throw avException("Error in writing audio frame");
//            }
//            if(aW.try_lock()) {
//                result = avcodec_receive_packet(audioOutputCodecContext, outputPacket.get()); // Try to receive a new packet without waiting
//                if (result >= 0) audioWrt.notify_one();// Signal converter thread to resume if necessary
//                aW.unlock();
//            }
//            else {
//                unique_lock<mutex> ul(aW);
//                audioWrt.notify_one();// Sync with converter thread if necessary
//                audioWrt.wait(ul);// Wait for sync signal from converter thread
//                result = avcodec_receive_packet(audioOutputCodecContext, outputPacket.get()); // Try to receive a new packet
//                if (result >= 0) audioWrt.notify_one();// Signal converter thread to resume if necessary
//            }
//        }
//        if (result == AVERROR(EAGAIN) ) {// Packet not available
//            unique_lock<mutex> ul(aW);
//            audioWrt.notify_one();// Sync with converter thread if necessary
//            audioWrt.wait(ul);// Wait for sync signal from converter thread
//            if (finishedAudioConversion) {
//                audioWrt.notify_one();// Sync with main thread if necessary
//                break;
//            }
//            result = avcodec_receive_packet(audioOutputCodecContext, outputPacket.get()); // Try to receive a new packet
//            if (result >= 0) audioWrt.notify_one();// Signal converter thread to resume if necessary
//        }
//        if(result < 0 && result != AVERROR(EAGAIN)) throw avException("Audio Converter/Writer threads syncronization error");
//    }
//    if(result < 0 && result != AVERROR_EOF && result != AVERROR(EAGAIN)) {
//        throw avException("Failed to receive frame from encoder"); // Error ending frame to encoder
//    }
//    //Free allocated memory
//}

//void ScreenRecorder::ConvertAudioFrames() {
//    // Create decoder audio frame
//    auto audioFrame = make_unique<AVFrame>(*alloc_audio_frame( audioInputCodecContext->frame_size, audioInputCodecContext->sample_fmt,
//                                                               audioInputCodecContext->channel_layout, 0));
//    // Create encoder audio frame
//    auto audioOutputFrame = make_unique<AVFrame>(*alloc_audio_frame(
//            audioOutputCodecContext->frame_size, audioOutputCodecContext->sample_fmt,
//            audioOutputCodecContext->channel_layout, 0));
////    AVAudioFifo* audioFifo = av_audio_fifo_alloc(audioInputCodecContext->sample_fmt, audioInputCodecContext->channels, 22050);
//    int audioFrameNum = 0;
//    int64_t pts = 0;
//    int result = AVERROR(EAGAIN);
//    if(aD.try_lock()) {
//        result = avcodec_receive_frame(audioInputCodecContext, audioFrame.get()); // Try to get a decoded frame without waiting
//        if(result>=0) audioCnv.notify_one();// Signal demuxer thread to resume if halted
//        aD.unlock();
//    }
//    else {
//        std::unique_lock<std::mutex> ul(aD);
//        audioCnv.notify_one();// Sync with demuxer thread if necessary
//        audioCnv.wait(ul);// Wait for audio demuxer thread sync signal
//        result = avcodec_receive_frame(audioInputCodecContext, audioFrame.get()); // Try to get a decoded frame
//        if(result>=0) audioCnv.notify_one();// Signal demuxer thread to resume if halted
//    }
//    // frame
//    while(result >= 0 || result == AVERROR(EAGAIN)) {
//        if(result >= 0) {
//            int got_samples = swr_convert(
//                    swrContext, audioOutputFrame->data, audioOutputFrame->nb_samples,
//                    (const uint8_t **) audioFrame->data, audioFrame->nb_samples);
//            if (got_samples < 0) {
//                throw avException("Failed to convert frames");
//            }
//            else if (got_samples > 0) {
//                pts += audioOutputFrame->nb_samples;
//                audioOutputFrame->pts = pts;
//                if(aW.try_lock()) {
//                    result = avcodec_send_frame(audioOutputCodecContext, audioOutputFrame.get());// Try to send a frame without waiting
//                    if (result >= 0) audioWrt.notify_one();
//                    aW.unlock();
//                }
//                else {
//                    unique_lock<mutex> ul(aW);
//                    audioWrt.notify_one();// Send sync signal to output writer thread if necessary
//                    audioWrt.wait(ul);//Wait for writer thread signal
//                    result = avcodec_send_frame(audioOutputCodecContext, audioOutputFrame.get());// Retry to send frame
//                    if (result >= 0) audioWrt.notify_one();
//                }
//                if (result == AVERROR(EAGAIN)) { // while encoder buffer is full
//                    unique_lock<mutex> ul(aW);
//                    audioWrt.notify_one();// Send sync signal to output writer thread
//                    audioWrt.wait(ul);// Wait for resume signal
//                    result = avcodec_send_frame(audioOutputCodecContext, audioOutputFrame.get());// retry
//                    if (result >= 0) audioWrt.notify_one();
//                }
//                if (result < 0 && result !=AVERROR(EAGAIN)) {
//                    throw avException("Failed to send frame to encoder"); // Error ending frame to encoder
//                }
//                // Frame was sent successfully
//                while (got_samples > 0) {
//                    got_samples = swr_convert(swrContext, audioOutputFrame->data, audioOutputFrame->nb_samples, nullptr, 0);
//                    if(got_samples >= 0) {
//                        pts += audioOutputFrame->nb_samples;
//                        audioOutputFrame->pts = pts;
//                        if(aW.try_lock()) {
//                            result = avcodec_send_frame(audioOutputCodecContext, audioOutputFrame.get());// Try to send a frame without waiting
//                            if (result >= 0) audioWrt.notify_one();
//                            aW.unlock();
//                        }
//                        else {
//                            unique_lock<mutex> ul(aW);
//                            audioWrt.notify_one();// Send sync signal to output writer thread if necessary
//                            audioWrt.wait(ul);//Wait for writer thread signal
//                            result = avcodec_send_frame(audioOutputCodecContext, audioOutputFrame.get());// Retry to send frame
//                            if (result >= 0) audioWrt.notify_one();
//                        }
//                        if (result == AVERROR(EAGAIN)) { // while encoder buffer is full
//                            unique_lock<mutex> ul(aW);
//                            audioWrt.notify_one();// Send sync signal to output writer thread
//                            audioWrt.wait(ul);// Wait for resume signal
//                            result = avcodec_send_frame(audioOutputCodecContext, audioOutputFrame.get());// retry
//                            if (result >= 0) audioWrt.notify_one();
//                        }
//                        if (result < 0 && result !=AVERROR(EAGAIN)) {
//                            throw avException("Failed to send frame to encoder"); // Error ending frame to encoder
//                        }
//                        // Frame was sent successfully
//                    }
//                }
//            }
//            if(aD.try_lock()) {
//                result = avcodec_receive_frame(audioInputCodecContext, audioFrame.get()); // Try to get a decoded frame without waiting
//                if(result>=0) audioCnv.notify_one();// Signal demuxer thread to resume if halted
//                aD.unlock();
//            }
//            else {
//                std::unique_lock<std::mutex> ul(aD);
//                audioCnv.notify_one();// Sync with demuxer thread if necessary
//                audioCnv.wait(ul);// Wait for audio demuxer thread sync signal
//                result = avcodec_receive_frame(audioInputCodecContext, audioFrame.get()); // Try to get a decoded frame
//                if(result>=0) audioCnv.notify_one();// Signal demuxer thread to resume if halted
//            }
//        }
//        if(result == AVERROR(EAGAIN)) {
//            std::unique_lock<std::mutex> ul(aD);
//            audioCnv.notify_one();// Sync with demuxer thread if necessary
//            audioCnv.wait(ul);// Wait for audio demuxer thread sync signal
//            if(finishedAudioDemux) {
//                audioCnv.notify_one();// Sync with main thread if necessary
//                break;
//            }
//            result = avcodec_receive_frame(audioInputCodecContext, audioFrame.get()); // Try to get a decoded frame
//            if(result>=0) audioCnv.notify_one();// Signal demuxer thread to resume if halted
//        }
//        if(result < 0 && result != AVERROR(EAGAIN)) throw avException("Audio Converter/Writer threads syncronization error");
//    }
//    if (result < 0 && result != AVERROR_EOF && result != AVERROR(EAGAIN)) {
//        // Decoder error
//        throw avException("Failed to receive decoded packet");
//    }
//    // Free allocated memory
//}
