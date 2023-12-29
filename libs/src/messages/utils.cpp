#include "utils.h"
#include <lwip/sockets.h>

void s(int fd, IMessage* msg) {
    Message m = msg->serialize();
    send(fd, m.data, m.len, 0);
}

IMessage* r(int fd) {
    Message m(1024);

    m.len = recv(fd, m.data, m.len, 0);

    IMessage* msg = NULL;

    if (m.len < 8) {
        return new RequestSongMessage();
    }

    switch (bufToInt(m.data)) {
    case MsgId::RES_SONG:
        msg = new ResponseSongMessage();
        break;
    default:
        msg = new RequestSongMessage();
        break;
    }

    msg->desearilize(m);

    return msg;
}

