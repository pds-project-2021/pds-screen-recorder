	#include "ScreenRecorder.h"
	using namespace std;


	int decode(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt)
	{
        int ret;
        *got_frame = 0;

        if (pkt) {
        ret = avcodec_send_packet(avctx, pkt);
        // In particular, we don't expect AVERROR(EAGAIN), because we read all
        // decoded frames with avcodec_receive_frame() until done.
        if (ret < 0)
          return ret == AVERROR_EOF ? 0 : ret;
        }

        ret = avcodec_receive_frame(avctx, frame);
        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
        return ret;
        if (ret >= 0)
        *got_frame = 1;

        return 0;
	}

	int encode(AVCodecContext *avctx, AVPacket *pkt, int *got_packet, AVFrame *frame)
	{
		int ret;

		*got_packet = 0;

		ret = avcodec_send_frame(avctx, frame);
		if (ret < 0)
			return ret;

	   ret = avcodec_receive_packet(avctx, pkt);
		if (!ret)
		   *got_packet = 1;
		if (ret == AVERROR(EAGAIN))
			return 0;

		return ret;
	}

	/* initialize the resources*/
	ScreenRecorder::ScreenRecorder() {
	  av_register_all();
	  avcodec_register_all();
	  avdevice_register_all();
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

	int ScreenRecorder::init() {
        inputFormatContext = avformat_alloc_context(); // Allocate an AVFormatContext.
        audioInputFormatContext = avformat_alloc_context(); // Allocate an AVFormatContext.
        options = nullptr;

	#ifdef _WIN32
        CoInitializeEx(NULL, COINIT_MULTITHREADED);//Set COM to multithreaded model
        av_dict_set(&options, "rtbufsize", "10M", 0);
		audioInputFormat = av_find_input_format("dshow");
		if (avformat_open_input(&audioInputFormatContext, "audio=Microfono (GENERAL WEBCAM)", audioInputFormat, &options) != 0) {
			throw avException("Couldn't open input stream");
		}
		options = nullptr;
        av_dict_set(&options, "framerate", "30", 0);
        av_dict_set(&options, "preset", "medium", 0);
        av_dict_set(&options, "offset_x", "0", 0);
        av_dict_set(&options, "offset_y", "0", 0);
        //av_dict_set(&options, "video_size", "1920x1080", 0);
        av_dict_set(&options, "show_region", "1", 0);
		inputFormat = av_find_input_format("gdigrab");
		if (avformat_open_input(&inputFormatContext, "desktop", inputFormat, &options) != 0) {
			throw avException("Couldn't open input stream");
			}
	#elif defined linux
		options = nullptr;
        av_dict_set(&options, "framerate", "30", 0);
        av_dict_set(&options, "preset", "medium", 0);
        av_dict_set(&options, "offset_x", "0", 0);
        av_dict_set(&options, "offset_y", "0", 0);
        //av_dict_set(&options, "video_size", "1920x1080", 0);
        av_dict_set(&options, "show_region", "1", 0);
		inputFormat = av_find_input_format("x11grab");
		if (avformat_open_input(&inputFormatContext, ":0.0+0,0", inputFormat, &options) != 0) {
			throw avException("Couldn't open input stream");
		  }
	#else
		show_avfoundation_device();
		inputFormat = av_find_input_format("avfoundation");
		if (avformat_open_input(&inputFormatContext, "1", inputFormat, nullptr) != 0) {
			throw avException("Couldn't open input stream");
		  }
	#endif
        auto ret = avformat_find_stream_info(inputFormatContext, &options);
        if (ret < 0) {
        throw avException("Unable to find the video stream information");
        }
        ret = avformat_find_stream_info(audioInputFormatContext, NULL);
        if (ret < 0) {
            throw avException("Unable to find the audio stream information");
        }
        auto index = av_find_best_stream(inputFormatContext, AVMEDIA_TYPE_VIDEO, -1,-1, nullptr, 0);
        if (index == -1) {
            throw avException("Unable to find the video stream index. (-1)");
        }
        auto audioIndex = av_find_best_stream(audioInputFormatContext, AVMEDIA_TYPE_AUDIO, -1,-1, nullptr, 0);
        if (audioIndex == -1) {
            throw avException("Unable to find the audio stream index. (-1)");
        }

        inputCodecPar = inputFormatContext->streams[index]->codecpar;
        inputCodecPar->format = AV_PIX_FMT_RGB32;

        audioInputCodecPar = audioInputFormatContext->streams[audioIndex]->codecpar;
        audioInputCodecPar->format = AV_SAMPLE_FMT_S16;
        audioInputCodecPar->sample_rate = 44100;
        audioInputCodecPar->channel_layout = AV_CH_LAYOUT_STEREO;
        audioInputCodecPar->channels = 2;
        audioInputCodecPar->codec_id = AV_CODEC_ID_PCM_S16LE;
        audioInputCodecPar->codec_type = AVMEDIA_TYPE_AUDIO;
        //audioInputCodecPar->frame_size = audioInputCodecPar->bit_rate/(30*8);// set number of audio samples in each frame


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

        ret = avcodec_open2(inputCodecContext, inputCodec,nullptr); // Initialize the AVCodecContext to use the given video AVCodec.
        if (ret < 0) {
        throw avException("Unable to open the video av codec");
        }

        ret = avcodec_open2( audioInputCodecContext,  audioInputCodec,nullptr); // Initialize the AVCodecContext to use the given audio AVCodec.
        if (ret < 0) {
            throw avException("Unable to open the audio av codec");
        }
	#ifdef _WIN32
		av_dump_format(inputFormatContext, 0, "desktop", 0);
        av_dump_format(audioInputFormatContext, 1, "dshow-audio-device", 0);
	#elif defined linux
		av_dump_format(inputFormatContext, 0, ":0.0+0,0", 0);
	#else
		av_dump_format(inputFormatContext, 0, "1", 0);
	#endif
	}

	int ScreenRecorder::init_outputfile() {
		output_file = "../output.mp4";

		outputCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!outputCodec) {
			throw avException(
					"Error in finding the video av codecs. try again with correct codec");
		}
		audioOutputCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
		if (!audioOutputCodec) {
			throw avException(
					"Error in finding the audio av codecs. try again with correct codec");
		}
		avformat_alloc_output_context2(&outputFormatContext, nullptr, nullptr, output_file);
		if (!outputFormatContext) {
			throw avException("Error in allocating av format output context");
		}

		videoStream = avformat_new_stream(outputFormatContext, outputCodec);
		if (!videoStream) {
			throw avException("Error in creating a av format new video stream");
		}
		audioStream  = avformat_new_stream(outputFormatContext, audioOutputCodec);
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
		outputCodecPar->codec_id = AV_CODEC_ID_H264; // AV_CODEC_ID_MPEG4; AV_CODEC_ID_H264; AV_CODEC_ID_MPEG1VIDEO;
		outputCodecPar->codec_type = AVMEDIA_TYPE_VIDEO;
		outputCodecPar->format = AV_PIX_FMT_YUV420P;
		outputCodecPar->bit_rate = 2400000; // 2500000
		outputCodecPar->width = inputCodecContext->width;
		outputCodecPar->height = inputCodecContext->height;

		audioOutputCodecPar = audioStream->codecpar;
		audioOutputCodecPar->codec_id = AV_CODEC_ID_AAC;
		audioOutputCodecPar->codec_type = AVMEDIA_TYPE_AUDIO;
		audioOutputCodecPar->bit_rate = 128000;
		audioOutputCodecPar->channels = audioInputCodecContext->channels;
		audioOutputCodecPar->channel_layout = audioInputCodecContext->channel_layout;
		audioOutputCodecPar->sample_rate = audioInputCodecContext->sample_rate;
		audioOutputCodecPar->format = audioOutputCodec->sample_fmts[0];
        //audioOutputCodecPar->frame_size = (int) (audioOutputCodecPar->bit_rate/(30*8)+0.5);
		auto ret = avcodec_parameters_to_context(outputCodecContext,outputCodecPar);
		if (ret < 0) {
			throw avException("Unable to get output codec context");
		}
		ret = avcodec_parameters_to_context(audioOutputCodecContext,audioOutputCodecPar);
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

	//  if (!outputFormatContext->nb_streams) {
	//    throw avException("Output file dose not contain any stream");
	//  }
        //audioInputCodecContext->frame_size=audioOutputCodecContext->frame_size;
        swsContext = sws_getCachedContext(swsContext, inputCodecPar->width, inputCodecPar->height,
                                        (AVPixelFormat)inputCodecPar->format,
                                        outputCodecPar->width, outputCodecPar->height,
                                        (AVPixelFormat)outputCodecPar->format,
                                        SWS_BICUBIC, nullptr, nullptr, nullptr);
        if (!swsContext) {
            throw avException("Impossible to create scale context for video conversion");
        }
        swrContext = swr_alloc();
        swrContext = swr_alloc_set_opts(swrContext, audioOutputCodecContext->channel_layout, audioOutputCodecContext->sample_fmt,
                                        audioOutputCodecContext->sample_rate, audioInputCodecContext->channel_layout,
                                        audioInputCodecContext->sample_fmt, audioInputCodecContext->sample_rate,
                                        0, NULL);
        if (!swrContext) {
            throw avException("Impossible to create resample context for audio conversion");
        }
        if(swr_init(swrContext)<0) {
            throw avException("Impossible to initialize resample context for audio conversion");
        }
	  /* imp: mp4 container or some advanced container file required header
	   * information*/
	  ret = avformat_write_header(outputFormatContext, &options);
	  if (ret < 0) {
		throw avException("Error in writing the header context");
	  }

	  // file informations
	  av_dump_format(outputFormatContext, 0, output_file, 1);
	}

	/* function to capture and store data in frames by allocating required memory
	 * and auto deallocating the memory.   */
	int ScreenRecorder::CaptureVideoFrames() {
	  // decoder frame
	  AVFrame *frame = av_frame_alloc();
	  if (!frame) {
		throw avException("Unable to release the avframe resources");
	  }
		frame->data[0] = nullptr;
		frame->width = inputCodecPar->width;
		frame->height = inputCodecPar->height;
		frame->format = inputCodecPar->format;
		frame->pts = 0;
		// Setup the data pointers and linesizes based on the specified image
		// parameters and the provided array.
		if (av_image_alloc(frame->data, frame->linesize,
						   inputCodecContext->width, inputCodecContext->height,
						   (AVPixelFormat)inputCodecPar->format, 32) < 0) {
			throw avException("Error in allocating frame data");
		}
	  // encoder frame
	  AVFrame *outputFrame = av_frame_alloc();
	  if (!outputFrame) {
		throw avException(
			"Unable to release the avframe resources for outputFrame");
	  }
	  outputFrame->data[0] = nullptr;
	  outputFrame->width = outputCodecPar->width;
	  outputFrame->height = outputCodecPar->height;
	  outputFrame->format = outputCodecPar->format;
	  outputFrame->pts = 0;
		// Setup the data pointers and linesizes based on the specified image
		// parameters and the provided array.
		if (av_image_alloc(outputFrame->data, outputFrame->linesize,
						   outputCodecContext->width, outputCodecContext->height,
						   (AVPixelFormat)outputFrame->format, 32) < 0) {
			throw avException("Error in allocating frame data");
		}

        int count = 0;
        int frameNum = 0; //frame number in a second
        int audioCount = 0;
        int frameCount = 61;
        int audioCycle = 0;

	  AVPacket *packet = av_packet_alloc();
	  if (!packet) {
		throw avException("Error on packet initialization");
	  }
	  AVPacket *outPacket= av_packet_alloc();
	  if (!outPacket) {
		  throw avException("Error on packet initialization");
	  }
        AVFrame *audioFrame = av_frame_alloc();
        if (!audioFrame) {
            throw avException("Unable to release the audio avframe resources");
        }
        audioFrame->nb_samples     = 22050;
        audioFrame->format         = audioInputCodecContext->sample_fmt;
        audioFrame->channel_layout = audioInputCodecContext->channel_layout;
        if (av_frame_get_buffer(audioFrame, 0) < 0) {
            fprintf(stderr, "Could not allocate audio data buffers\n");
            exit(1);
        }
        AVFrame *audioOutputFrame = av_frame_alloc();
        if (!audioOutputFrame) {
            throw avException(
                    "Unable to release the audio avframe resources for outputFrame");
        }
        audioOutputFrame->nb_samples     = audioOutputCodecContext->frame_size;
        audioOutputFrame->format         = audioOutputCodecContext->sample_fmt;
        audioOutputFrame->channel_layout = audioOutputCodecContext->channel_layout;
        if (av_frame_get_buffer(audioOutputFrame, 0) < 0) {
            fprintf(stderr, "Could not allocate audio data buffers\n");
            exit(1);
        }
        AVPacket *audioPacket = av_packet_alloc();
        if (!audioPacket) {
            throw avException("Error on packet initialization");
        }
        AVPacket *audioOutputPacket = av_packet_alloc();
        if (!audioOutputPacket) {
            throw avException("Error on packet initialization");
        }
    while (av_read_frame(inputFormatContext, packet) >= 0) {//Try to extract packet from input stream
        if (count++ == frameCount) {
        break;
        }
        if (frameNum++ == 30) frameNum=0; //reset every fps frames
		if (packet->stream_index == videoStream->index) {
            //Send packet to decoder
            auto result = avcodec_send_packet(inputCodecContext, packet);
            //Check result
            if (result >= 0) result = avcodec_receive_frame(inputCodecContext, frame); //Try to get a decoded frame
            else if (result == AVERROR(EAGAIN)) {//Buffer is full, cannot send new packet
                while (avcodec_send_packet(inputCodecContext, packet) ==
                       AVERROR(EAGAIN)) { //While decoder buffer is full
                    result = avcodec_receive_frame(inputCodecContext, frame); //Try to get a decoded frame
                }
            } else {
                throw avException("Failed to send packet to decoder");//Decoder error
            }
            if (result != AVERROR(EAGAIN)) {//check if decoded frame is ready
                if (result >= 0) {//frame is ready
                    //Convert frame picture format
                    sws_scale(swsContext, frame->data, frame->linesize, 0,
                              inputCodecContext->height, outputFrame->data,
                              outputFrame->linesize);
                    //Send converted frame to encoder
                    outputFrame->pts = count - 1;
                    result = avcodec_send_frame(outputCodecContext, outputFrame);
                    if (result >= 0)
                        result = avcodec_receive_packet(outputCodecContext, outPacket);//Try to receive packet
                    else if (result == AVERROR(EAGAIN)) {//Buffer is full
                        while (result = avcodec_send_frame(outputCodecContext, outputFrame) ==
                                        AVERROR(EAGAIN)) {//while encoder buffer is full
                            result = avcodec_receive_packet(outputCodecContext, outPacket);//Try to receive packet
                        }
                    } else throw avException("Failed to send frame to encoder");//Error ending frame to encoder
                    //Frame was sent successfully
                    if (result >= 0) {//Packet received successfully
                        if (outPacket->pts != AV_NOPTS_VALUE) {
                            outPacket->pts =
                                    av_rescale_q(outPacket->pts, outputCodecContext->time_base, videoStream->time_base);
                        }
                        if (outPacket->dts != AV_NOPTS_VALUE) {
                            outPacket->dts =
                                    av_rescale_q(outPacket->dts, outputCodecContext->time_base, videoStream->time_base);
                        }
                        //Write packet to file
                        result = av_write_frame(outputFormatContext, outPacket);
                        if (result != 0) {
                            throw avException("Error in writing video frame");
                        }
                    } else if (result != AVERROR(EAGAIN)) throw avException("Failed to encode frame");
                    av_packet_unref(outPacket);
                } else throw avException("Failed to decode packet");
            }
        }
		//Handle audio input stream packets
        int sync = ((int) frameNum/7) - audioCycle;
        if (sync==1 && av_read_frame(audioInputFormatContext, audioPacket) >= 0) {
            if(audioCycle++ == 3) audioCycle=0;
            //Send packet to decoder
            auto result = avcodec_send_packet(audioInputCodecContext, audioPacket);
            //Check result
            if (result >=0) result = avcodec_receive_frame(audioInputCodecContext, audioFrame); //Try to get a decoded frame
            else if (result == AVERROR(EAGAIN)) {//Buffer is full, cannot send new packet
                while(avcodec_send_packet(audioInputCodecContext, audioPacket)== AVERROR(EAGAIN)){ //While decoder buffer is full
                    result = avcodec_receive_frame(audioInputCodecContext, audioFrame); //Try to get a decoded frame
                }
            }
            else throw avException("Failed to send packet to decoder");//Decoder error
            if (result != AVERROR(EAGAIN)) {//check if decoded frame is ready
                if(result>=0) {//frame is ready
                    //Convert frame picture format
                    int got_samples = swr_convert(swrContext, audioOutputFrame->data, audioOutputFrame->nb_samples,
                                (const uint8_t **)audioFrame->data, audioFrame->nb_samples);
                    if (got_samples < 0) {
                        fprintf(stderr, "error: swr_convert()\n");
                        exit(1);
                    }
                    audioCount++;
                    audioOutputFrame->pts= audioCount-1;
                    result = avcodec_send_frame(audioOutputCodecContext, audioOutputFrame);
                    if (result >= 0)
                        result = avcodec_receive_packet(audioOutputCodecContext, audioOutputPacket);//Try to receive packet
                    else if (result == AVERROR(EAGAIN)) {//Buffer is full
                        while (result = avcodec_send_frame(audioOutputCodecContext, audioOutputFrame) ==
                                        AVERROR(EAGAIN)) {//while encoder buffer is full
                            result = avcodec_receive_packet(audioOutputCodecContext, audioOutputPacket);//Try to receive packet
                        }
                    } else throw avException("Failed to send frame to encoder");//Error ending frame to encoder
                    //Frame was sent successfully
                    if (result >= 0) {//Packet received successfully
                        if (audioOutputPacket->pts != AV_NOPTS_VALUE) {
                            audioOutputPacket->pts =
                                    av_rescale_q(audioOutputPacket->pts, {1, audioInputCodecContext->sample_rate*audioInputCodecContext->channels/audioOutputFrame->nb_samples}, videoStream->time_base);
                        }
                        if (audioOutputPacket->dts != AV_NOPTS_VALUE) {
                            audioOutputPacket->dts =
                                    av_rescale_q(audioOutputPacket->dts, {1, audioInputCodecContext->sample_rate*audioInputCodecContext->channels/audioOutputFrame->nb_samples}, videoStream->time_base);
                        }
                        //Write packet to file
                        audioOutputPacket->stream_index=1;
                        result = av_write_frame(outputFormatContext, audioOutputPacket);
                        if (result != 0) {
                            throw avException("Error in writing video frame");
                        }
                    } else if (result != AVERROR(EAGAIN)) throw avException("Failed to encode frame");
                    av_packet_unref(audioOutputPacket);
                    while(got_samples > 0) {
                        got_samples = swr_convert(swrContext, audioOutputFrame->data, audioOutputFrame->nb_samples,
                                                  0, 0);
                        audioCount++;
                        audioOutputFrame->pts=audioCount-1;
                        result = avcodec_send_frame(audioOutputCodecContext, audioOutputFrame);
                        if (result >= 0)
                            result = avcodec_receive_packet(audioOutputCodecContext, audioOutputPacket);//Try to receive packet
                        else if (result == AVERROR(EAGAIN)) {//Buffer is full
                            while (result = avcodec_send_frame(audioOutputCodecContext, audioOutputFrame) ==
                                            AVERROR(EAGAIN)) {//while encoder buffer is full
                                result = avcodec_receive_packet(audioOutputCodecContext, audioOutputPacket);//Try to receive packet
                            }
                        } else throw avException("Failed to send frame to encoder");//Error ending frame to encoder
                        //Frame was sent successfully
                        if (result >= 0) {//Packet received successfully
                            if (audioOutputPacket->pts != AV_NOPTS_VALUE) {
                                audioOutputPacket->pts =
                                        av_rescale_q(audioOutputPacket->pts, {1, audioInputCodecContext->sample_rate*audioInputCodecContext->channels/audioOutputFrame->nb_samples}, videoStream->time_base);
                            }
                            if (audioOutputPacket->dts != AV_NOPTS_VALUE) {
                                audioOutputPacket->dts =
                                        av_rescale_q(audioOutputPacket->dts, {1, audioInputCodecContext->sample_rate*audioInputCodecContext->channels/audioOutputFrame->nb_samples}, videoStream->time_base);
                            }
                            //Write packet to file
                            audioOutputPacket->stream_index=1;
                            result = av_write_frame(outputFormatContext, audioOutputPacket);
                            if (result != 0) {
                                throw avException("Error in writing video frame");
                            }
                        } else if (result != AVERROR(EAGAIN)) throw avException("Failed to encode frame");
                        av_packet_unref(audioOutputPacket);
                    }
                    //Send converted frame to encoder

                } else throw avException("Failed to decode packet");
            }
        }
	  } // End of while-loop
		//Handle delayed frames
		for (int result;;) {
			//avcodec_send_frame(outputCodecContext, NULL);
			if (avcodec_receive_packet(outputCodecContext, outPacket) == 0) {//Try to get packet
				if (outPacket->pts != AV_NOPTS_VALUE) {
					outPacket->pts =
							av_rescale_q(outPacket->pts, outputCodecContext->time_base,videoStream->time_base);
				}
				if (outPacket->dts != AV_NOPTS_VALUE) {
					outPacket->dts =
							av_rescale_q(outPacket->dts, outputCodecContext->time_base,videoStream->time_base);
				}
				result = av_write_frame(outputFormatContext, outPacket);//Write packet to file
				if (result != 0) {
					throw avException("Error in writing video frame");
				}
				av_packet_unref(outPacket);
			}
			else {//No remaining frames to handle
				break;
			}
		}
		//Write video file trailer data
		auto ret = av_write_trailer(outputFormatContext);
		if (ret < 0) {
			throw avException("Error in writing av trailer");
		}
		if (!(outputFormatContext->flags & AVFMT_NOFILE)) {
			int err = avio_close(outputFormatContext->pb);
			if (err < 0) {
				throw "Failed to close file";
			}
		}

	  // THIS WAS ADDED LATER
	//    av_free(outBuffer);
	  cout << "qua" << endl;
	}


