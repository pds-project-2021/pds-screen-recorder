//
// Created by gabriele on 23/12/21.
//

#pragma once

#include "ffmpegc.h"
#include "Tracker.h"
#include "exceptions.h"

template <typename T>
class ptr_wrapper: public Tracker<ptr_wrapper<T>>{
  protected:
	T* inner = nullptr;

  public:
	ptr_wrapper();
	~ptr_wrapper();

	void unref();
	T* into();
};

/* General template functions */

template<typename T>
ptr_wrapper<T>::ptr_wrapper() = default;

template<typename T>
ptr_wrapper<T>::~ptr_wrapper() = default;

template<typename T>
T *ptr_wrapper<T>::into() {
	return inner;
}

/* Specialized template functions */

template<> inline
ptr_wrapper<AVPacket>::~ptr_wrapper() {
	if(inner != nullptr) {
		av_packet_unref(inner);
		av_packet_free(&inner);
	}
}

template<> inline
ptr_wrapper<AVFrame>::~ptr_wrapper() {

	if(inner != nullptr) {
		if(inner->data[1] != nullptr){
			av_freep(&inner->data[0]);
		}

		av_frame_free(&inner);
	}
}

template<> inline
ptr_wrapper<AVPacket>::ptr_wrapper() {
	inner = av_packet_alloc();

	if(!inner){
		throw avException("Error on packet initialization");
	}
}

template<> inline
void ptr_wrapper<AVPacket>::unref() {
	av_packet_unref(inner);
}

template<> inline
void ptr_wrapper<AVFrame>::unref() {
	av_frame_unref(inner);
}
