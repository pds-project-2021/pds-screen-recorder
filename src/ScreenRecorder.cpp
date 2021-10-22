#include "ScreenRecorder.h"
using namespace std;


static void fill_yuv_image(uint8_t *data[4], int linesize[4],
                           int width, int height, int frame_index)
{
    int x, y;

    /* Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            data[0][y * linesize[0] + x] = x + y + frame_index * 3;

    /* Cb and Cr */
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            data[1][y * linesize[1] + x] = 128 + y + frame_index * 2;
            data[2][y * linesize[2] + x] = 64 + x + frame_index * 5;
        }
    }
}

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

  options = nullptr;
  av_dict_set(&options, "framerate", "30", 0);
  av_dict_set(&options, "preset", "medium", 0);
  av_dict_set(&options, "offset_x", "0", 0);
  av_dict_set(&options, "offset_y", "0", 0);
  //av_dict_set(&options, "video_size", "1920x1080", 0);
  av_dict_set(&options, "show_region", "1", 0);

#ifdef _WIN32
#if USE_DSHOW
    options = nullptr;
    inputFormat = av_find_input_format("dshow");
    if (avformat_open_input(&inputFormatContext, "video=screen-capture-recorder", inputFormat, &options) != 0) {
        throw avException("Couldn't open input stream");
    }
#else
    options = nullptr;
    inputFormat = av_find_input_format("gdigrab");
    if (avformat_open_input(&inputFormatContext, "desktop", inputFormat, &options) != 0) {
        throw avException("Couldn't open input stream");
        }
#endif
#elif defined linux
    options = nullptr;
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
    throw avException("Unable to find the stream information");
  }

  auto index = av_find_best_stream(inputFormatContext, AVMEDIA_TYPE_VIDEO, -1,-1, nullptr, 0);

  if (index == -1) {
    throw avException("Unable to find the video stream index. (-1)");
  }

  inputCodecPar = inputFormatContext->streams[index]->codecpar;
  inputCodecPar->format = AV_PIX_FMT_RGB32;

  inputCodec = avcodec_find_decoder(inputCodecPar->codec_id);
  if (inputCodec == nullptr) {
    throw avException("Unable to find the decoder");
  }

  inputCodecContext = avcodec_alloc_context3(inputCodec);
  if (inputCodecContext == nullptr) {
    throw avException("Unable to get input codec context");
  }
  avcodec_parameters_to_context(inputCodecContext, inputCodecPar);

  ret = avcodec_open2(inputCodecContext, inputCodec,nullptr); // Initialize the AVCodecContext to use the given AVCodec.
  if (ret < 0) {
    throw avException("Unable to open the av codec");
  }
#ifdef _WIN32
    av_dump_format(inputFormatContext, 0, "desktop", 0);
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
        "Error in finding the av codecs. try again with correct codec");
  }

  avformat_alloc_output_context2(&outputFormatContext, nullptr, nullptr, output_file);
  if (!outputFormatContext) {
    throw avException("Error in allocating av format output context");
  }

  videoStream = avformat_new_stream(outputFormatContext, outputCodec);
  if (!videoStream) {
    throw avException("Error in creating a av format new stream");
  }
  videoStream->time_base = {1, 30};
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
    throw avException("Error in allocating the codec context");
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
  outputCodecPar->width = inputCodecPar->width;
  outputCodecPar->height = inputCodecPar->height;

  auto ret = avcodec_parameters_to_context(outputCodecContext,outputCodecPar);
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
    throw avException("Error in opening the avcodec");
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

  swsContext = sws_getCachedContext(swsContext, inputCodecPar->width, inputCodecPar->height,
                              (AVPixelFormat)inputCodecPar->format,
                              outputCodecPar->width, outputCodecPar->height,
                              (AVPixelFormat)outputCodecPar->format,
                              SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!swsContext) {
        throw avException("Impossible to create scale context for the conversion");
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
  int frameCount = 300;


  AVPacket *packet = av_packet_alloc();
  if (!packet) {
    throw avException("Error on packet initialization");
  }
  AVPacket *outPacket= av_packet_alloc();
  if (!outPacket) {
      throw avException("Error on packet initialization");
  }
  while (av_read_frame(inputFormatContext, packet) >= 0) {//Try to extract packet from input stream
    if (count++ == frameCount) {
    break;
    }

    if (packet->stream_index == videoStream->index) {
        //Send packet to decoder
         auto result = avcodec_send_packet(inputCodecContext, packet);
         //Check result
         if (result >=0) result = avcodec_receive_frame(inputCodecContext, frame); //Try to get a decoded frame
         else if (result == AVERROR(EAGAIN)) {//Buffer is full, cannot send new packet
             while(avcodec_send_packet(inputCodecContext, packet)== AVERROR(EAGAIN)){ //While decoder buffer is full
                 result = avcodec_receive_frame(inputCodecContext, frame); //Try to get a decoded frame
             }
         }
         else{
            throw avException("Failed to send packet to decoder");//Decoder error
          }
        if (result != AVERROR(EAGAIN)) {//check if decoded frame is ready
            if(result>=0) {//frame is ready
                //Convert frame picture format
                sws_scale(swsContext, frame->data, frame->linesize, 0,
                          inputCodecContext->height, outputFrame->data,
                          outputFrame->linesize);
                //Send converted frame to encoder
                outputFrame->pts=count-1;
                result = avcodec_send_frame(outputCodecContext, outputFrame);
                if(result >=0) result = avcodec_receive_packet(outputCodecContext, outPacket);//Try to receive packet
                else if(result == AVERROR(EAGAIN))  {//Buffer is full
                    while(result = avcodec_send_frame(outputCodecContext, outputFrame) == AVERROR(EAGAIN)){//while encoder buffer is full
                        result = avcodec_receive_packet(outputCodecContext, outPacket);//Try to receive packet
                    }
                }
                else throw avException("Failed to send frame to encoder");//Error ending frame to encoder
                //Frame was sent successfully
                if(result>=0) {//Packet received successfully
                    if (outPacket->pts != AV_NOPTS_VALUE) {
                        outPacket->pts =
                                av_rescale_q(outPacket->pts, outputCodecContext->time_base,videoStream->time_base);
                    }
                    if (outPacket->dts != AV_NOPTS_VALUE) {
                        outPacket->dts =
                                av_rescale_q(outPacket->dts, outputCodecContext->time_base,videoStream->time_base);
                    }
                    //Write packet to file
                    result = av_write_frame(outputFormatContext, outPacket);
                    if (result != 0) {
                        throw avException("Error in writing video frame");
                    }
                }
                else if(result != AVERROR(EAGAIN))  throw avException("Failed to encode frame");
                av_packet_unref(outPacket);
            }
            else throw avException("Failed to decode packet");
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
            throw exception("Failed to close file", err);
        }
    }

  // THIS WAS ADDED LATER
//    av_free(outBuffer);
  cout << "qua" << endl;

  av_dump_format(outputFormatContext, 0,"../output.txt", 1);


}


