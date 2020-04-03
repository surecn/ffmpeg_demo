#include "packetqueue.h"


PacketQueue::PacketQueue()
{
    mutex = SDL_CreateMutex();
    cond = SDL_CreateCond();
    first_pkt = NULL;
    last_pkt = NULL;
    nb_packets = 0;
    size = 0;
}

int PacketQueue::packet_queue_put(AVPacket *pkt) {//put pkt to pktQueue
    AVPacketList *pkt1;

    pkt1 = (AVPacketList *)av_malloc(sizeof(AVPacketList));
    if (!pkt1) {
        return -1;
    }
    pkt1->pkt = *pkt;//put pkt to PacketList
    pkt1->next = NULL;

    SDL_LockMutex(mutex);

    if (!last_pkt) {//last_pkt = NULL, Queue element is  PacketList,we can not use packet directly
        first_pkt = pkt1;
    } else {//not first time
        last_pkt->next = pkt1;
    }

    last_pkt = pkt1;//update
    nb_packets++;//record nb_packet
    size += pkt1->pkt.size;//update pkt size in queue
    SDL_CondSignal(cond);
    SDL_UnlockMutex(mutex);
    return 0;
}

int PacketQueue::packet_queue_get(AVPacket *pkt, int block) {//get data from queue
    AVPacketList *pkt1;
    int ret;

    SDL_LockMutex(mutex);//lock

    for(;;) {
        if(quit) {//is or not quit firstly
            ret = -1;
            break;
        }//quit，跳出循环

        pkt1 = first_pkt;//queue: first in, first out
        if (pkt1) {
            first_pkt = pkt1->next;
            if (!first_pkt) {//if queue is over
                last_pkt = NULL;
            }
            nb_packets--;
            size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;

            av_free(pkt1);
            ret = 1;//
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            SDL_CondWait(cond, mutex);//until data enough
        }
    }
    SDL_UnlockMutex(mutex);
    return ret;
}

void PacketQueue::packet_queue_flush() {//flush queue
    AVPacketList *pkt, *pkt1;

    SDL_LockMutex(mutex);//lock
    for (pkt = first_pkt; pkt != NULL; pkt = pkt1) {
        pkt1 = pkt->next;
        av_free_packet(&pkt->pkt);
        av_freep(&pkt);
    }
    last_pkt = NULL;
    first_pkt = NULL;
    nb_packets = 0;
    size = 0;
    SDL_UnlockMutex(mutex);//unlock
}

