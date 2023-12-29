#ifndef MESSAGES_MESSAGES
#define MESSAGES_MESSAGES

void intToBuf(int, char*);
int bufToInt(char*);
void strToBuf(char*, char*);
char* bufToStr(char*);

enum MsgId {REQ_SONG, RES_SONG};

struct Message {
    int len;
    char* data;

    Message(int);
    ~Message();
};

class IMessage {
    public:
        virtual Message serialize() = 0;
        virtual void desearilize(const Message&) = 0;
        virtual MsgId getType() = 0;
};

class RequestSongMessage: public IMessage {
    public:
        Message serialize();
        void desearilize(const Message&);
        MsgId getType();
};

class ResponseSongMessage: public IMessage {
    public:
        ResponseSongMessage();
        ResponseSongMessage(const char*);
        Message serialize();
        void desearilize(const Message&);
        char* getSongName();
        void setSongName(const char*);
        MsgId getType();
        ~ResponseSongMessage();
    protected:
        char* songName = nullptr;
};

#endif