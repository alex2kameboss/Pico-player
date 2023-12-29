#include "messages.h"
#include <cstring>

void intToBuf(int n, char* buf) {
    buf[0] = n >> 24;
    buf[1] = n >> 16;
    buf[2] = n >> 8;
    buf[3] = n;
}

int bufToInt(char* buf) {
    return int((unsigned char)(buf[0]) << 24 |
            (unsigned char)(buf[1]) << 16 |
            (unsigned char)(buf[2]) << 8 |
            (unsigned char)(buf[3]));
}

void strToBuf(char* s, char* buf) {
    strcpy(buf, s);
}

char* bufToStr(char* buf) {
    char* s = new char[strlen(buf) + 1];
    strcpy(s, buf);
    return s;
}

Message RequestSongMessage::serialize() {
    Message msg(8);
    intToBuf(MsgId::REQ_SONG, msg.data);
    intToBuf(0, msg.data + 4);

    return msg;
}

void RequestSongMessage::desearilize(const Message& msg) { }

MsgId RequestSongMessage::getType()
{
    return MsgId::REQ_SONG;
}

ResponseSongMessage::ResponseSongMessage() { }

ResponseSongMessage::ResponseSongMessage(const char *p)
{
    setSongName(p);
}

Message ResponseSongMessage::serialize()
{
    Message m(9 + strlen(songName)); // + null terminator
    intToBuf(MsgId::RES_SONG, m.data);
    intToBuf(strlen(songName) + 1, m.data + 4);
    strToBuf(songName, m.data + 8);

    return m;
}

char* ResponseSongMessage::getSongName() {
    return songName;
}

void ResponseSongMessage::setSongName(const char* p) {
    if (songName != nullptr) {
        delete[] songName;
    }

    songName = new char[strlen(p) + 1];
    strcpy(songName, p);
}

MsgId ResponseSongMessage::getType()
{
    return MsgId::RES_SONG;
}

ResponseSongMessage::~ResponseSongMessage()
{
    if (songName != nullptr)
    {
        delete[] songName;
    }
    
}

void ResponseSongMessage::desearilize(const Message& m) {
    songName = bufToStr(m.data + 8);
}

Message::Message(int _len) {
    len = _len;
    data = new char[_len];
}

Message::~Message() {
    delete[] data;
}
