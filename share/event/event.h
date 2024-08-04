#ifndef wukong_event_h
#define wukong_event_h

#include <map>
#include <list>
#include <string>
#include <functional>

/* Messages Event data */
class EventData
{
public:
    virtual ~EventData() {}
};

template<class T>
class EventDataImpl : public EventData
{
public:
    EventDataImpl(const T &v) : v_(v) {}
    T v_;
};

class EventParam
{
public:
    ~EventParam() {
        delete data_;
    }

    EventParam(EventParam &&ep)
    {
        data_ = ep.data_;
        ep.data_ = nullptr;
    }

    template<class T >
    EventParam(const T &v)
    {
        data_ = new EventDataImpl<T>(v);
    }

    template<class T>
    void getData(T &v) const
    {
        if (data_) {
            v = static_cast<EventDataImpl<T>*>(data_)->v_;
        }
    }

public:
    EventData *data_ = nullptr;
};

class Event
{
public:
    Event(const char* name): name_(name) {}

    template<class T>
    void setParam(const std::string &k, const T &v)
    {
        EventParam param(v);
        dataMap_.insert(std::make_pair(k, std::move(param)));
    }

    template<class T>
    bool getParam(const std::string &k, T &v) const
    {
        auto it = dataMap_.find(k);
        if (it != dataMap_.end()) {
            it->second.getData(v);
            return true;
        }
        return false;
    }

    const std::string &getName() const { return name_; }

private:
    std::string name_;
    std::map<std::string, EventParam> dataMap_;

private:
    Event() = default;                          // ctor hidden
    Event(Event const&) = delete;               // copy ctor hidden
    Event(Event &&) = delete;                   // move ctor hidden
    Event& operator=(Event const&) = delete;    // assign op. hidden
    Event& operator=(Event &&) = delete;        // move assign op. hidden
};

typedef std::function<void (const Event &)> EventHandle;

/* Messages Event handle */
class EventEmitter
{
public:
    EventEmitter() {}
    ~EventEmitter() {
        clear();
    }

    void regEventHandle(const std::string &name, EventHandle handle);

    void clear();

    void fireEvent(const Event &event);

private:
    std::map<std::string, std::list<EventHandle>> eventHandles_;
};

#endif /* wukong_event_h */
