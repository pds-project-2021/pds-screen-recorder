#include "Packet.h"

Packet::Packet() {
	inner = av_packet_alloc();

	if (!inner) {
		throw avException("Error on packet initialization");
	}

}

Packet::~Packet() {
	if (inner != nullptr) {
		av_packet_unref(inner);
		av_packet_free(&inner);
	}

}

void Packet::unref() {
	av_packet_unref(inner);
}
