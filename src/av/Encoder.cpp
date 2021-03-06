#include "av/Encoder.hpp"

#include "av/Packet.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace av {

Encoder::Encoder(int width, int height, std::function<void(bool)> completion):
    m_width(width),
    m_height(height),
    m_completion(completion),
    m_outputFormat(nullptr),
    m_codec(nullptr),
    m_formatContext(nullptr) {}

Encoder::~Encoder() {}

void Encoder::finishEncode(bool valid) {
    spdlog::debug("Encoder finishing encode, valid: {}", valid);

    int ret = 0;
    if (valid) {
        // Flush the codec.
        ret = avcodec_send_frame(m_codecContext, nullptr);
        if (ret < 0 && ret != AVERROR_EOF) {
            spdlog::error("Encoder failed flushing encoder.");
            valid = false;
        }
    }

    spdlog::debug("Encoder through avcodec_send_frame flush, valid: {}", valid);


    if (valid) {
        Packet packet;
        packet.create();
        ret = avcodec_receive_packet(m_codecContext, packet.get());
        while (ret == 0) {
            if (av_write_frame(m_formatContext, packet.get()) < 0) {
                spdlog::error("Encoder failed writing a packet during flush.");
                valid = false;
                break;
            }
        }
        if (valid && ret != AVERROR_EOF) {
            spdlog::error("Encoder failed receiving flushed packet.");
            valid = false;
        }
    }

    spdlog::debug("Encoder through avcodec_receive_packet flush, valid: {}", valid);

    if (valid) {
        // Flush the output format.
        ret = av_write_frame(m_formatContext, nullptr);
        if (ret != 1) {
            spdlog::error("got return of {} on flusing output format.", ret);
            valid = false;
        }
    }

    if (valid) {
        ret = av_write_trailer(m_formatContext);
        if (ret != 0) {
            spdlog::error("Encoder failed to write output trailer.");
            valid = false;
        }
    }

    avio_closep(&m_formatContext->pb);
    m_completion(valid);
}

} // namespace av

} // namespace scin
