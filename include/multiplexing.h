#ifndef WY_MULTIPLEXING_H
#define WY_MULTIPLEXING_H

#include "common.h"

namespace wynet
{

constexpr int MP_READABLE = 1;
constexpr int MP_WRITABLE = 2;
constexpr int MP_FILE_EVENTS = 1;
constexpr int MP_TIME_EVENTS = 2;
constexpr int MP_ALL_EVENTS = MP_FILE_EVENTS | MP_TIME_EVENTS;
constexpr int MP_DONT_WAIT = 4;
constexpr int MP_CALL_AFTER_SLEEP = 8;
constexpr int MP_HALT = -1;
constexpr int MP_NO_MASK = 0;
constexpr int MP_OK = 0;
constexpr int MP_ERR = -1;
constexpr int DELETED_TIME_EVENT = -1;

class MpEventLoop;

using MpFileProc = std::function<void(MpEventLoop *eventLoop, int fd, void *clientData, int mask)>;

using MpTimeProc = std::function<int(MpEventLoop *eventLoop, long long id, void *clientData)>;

using MpEventFinalizerProc = std::function<void(MpEventLoop *eventLoop, void *clientData)>;

using MpBeforeSleepProc = std::function<void(MpEventLoop *eventLoop)>;

struct MpFileEvent
{
    MpFileEvent() : mask(MP_NO_MASK),
                    rfileProc(nullptr),
                    wfileProc(nullptr),
                    clientData(nullptr)
    {
    }
    int mask;
    MpFileProc rfileProc;
    MpFileProc wfileProc;
    void *clientData;
};

struct MpTimeEvent
{
    long long id;
    long when_sec;
    long when_ms;
    MpTimeProc timeProc;
    MpEventFinalizerProc finalizerProc;
    void *clientData;
    bool operator<(const MpTimeEvent &rhs) const;
};

typedef std::shared_ptr<MpTimeEvent> MpTimeEventPtr;

struct Compare
{
    bool operator()(const MpTimeEventPtr &a, const MpTimeEventPtr &b)
    {
        return *a < *b;
    }
};

struct MpFiredEvent
{
    MpFiredEvent() : fd(0),
                     mask(0)
    {
    }
    int fd;
    int mask;
};

// not thread-safe
class MpEventLoop
{
  public:
    MpEventLoop(int setsize);

    ~MpEventLoop();

    void stop();

    bool isStopped() { return m_stop; }

    int resizeSetSize(int setsize);

    int createFileEvent(int fd, int mask,
                        MpFileProc proc, void *clientData);

    int deleteFileEvent(int fd, int mask);

    int getFileEvents(int fd);

    long long createTimeEvent(long long milliseconds,
                              MpTimeProc proc, void *clientData,
                              MpEventFinalizerProc finalizerProc);

    int deleteTimeEvent(long long id);

    int processEvents(int flags);

    const MpTimeEventPtr &searchNearestTimer();

    int wait(int fd, int mask, long long milliseconds);

    void setBeforeSleepProc(MpBeforeSleepProc beforesleep);

    void setAfterSleepProc(MpBeforeSleepProc aftersleep);

    const char *getApiName();

    inline int getSetSize() { return m_setsize; }

    inline void *getApiData() { return m_apidata; }

    inline void setApiData(void *apidata) { m_apidata = apidata; }

    inline int getMaxFd() { return m_maxfd; }

    inline std::vector<MpFileEvent> &getEvents() { return m_events; }

    inline std::vector<MpFiredEvent> &getFiredEvents() { return m_fired; }

    void debugInfo();

  private:
    int processTimeEvents();

    void cleanDeletedTimeEvents();

  private:
    int m_maxfd;
    int m_setsize;
    long long m_teNextId;
    time_t m_lastTime;
    std::vector<MpFileEvent> m_events;
    std::vector<MpFiredEvent> m_fired;
    MpTimeEventPtr m_teNearest;
    bool m_stop;
    void *m_apidata;
    std::multiset<MpTimeEventPtr, Compare> m_teSet;
    std::vector<MpTimeEventPtr> m_teListDeleted;
    MpBeforeSleepProc m_beforesleep;
    MpBeforeSleepProc m_aftersleep;
};

}; // namespace wynet
#endif
