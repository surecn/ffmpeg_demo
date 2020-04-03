#ifndef PACKETQUEUE_H
#define PACKETQUEUE_H

extern "C" {
#include <libavformat/avformat.h>
#include "SDL2/SDL.h"
}


class PacketQueue
{
public:
    PacketQueue();
    int packet_queue_put(AVPacket *pkt);
    int packet_queue_get(AVPacket *pkt, int block);
    void packet_queue_flush();
    int quit;
    int size;
private:
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    SDL_mutex *mutex;
    SDL_cond *cond;
};

#endif // AUDIOPACKAGEQUEUE_H
