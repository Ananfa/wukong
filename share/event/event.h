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
    EventDataImpl(const T &v) : _v(v) {}
    T _v;
};

class EventParam
{
public:
    ~EventParam() {
        delete _data;
    }

    EventParam(EventParam &&ep)
    {
        _data = ep._data;
        ep._data = nullptr;
    }

    template<class T >
    EventParam(const T &v)
    {
        _data = new EventDataImpl<T>(v);
    }

    template<class T>
    void getData(T &v) const
    {
        if (_data) {
            v = static_cast<EventDataImpl<T>*>(_data)->_v;
        }
    }

public:
    EventData *_data = nullptr;
};

class Event
{
public:
    Event(const char* name): _name(name) {}

    template<class T>
    void setParam(const std::string &k, const T &v)
    {
        EventParam param(v);
        _dataMap.insert(std::make_pair(k, std::move(param)));
    }

    template<class T>
    bool getParam(const std::string &k, T &v) const
    {
        auto it = _dataMap.find(k);
        if (it != _dataMap.end()) {
            it->second.getData(v);
            return true;
        }
        return false;
    }

    const std::string &getName() const { return _name; }

private:
    std::string _name;
    std::map<std::string, EventParam> _dataMap;

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

    uint32_t addEventHandle(const std::string &name, EventHandle handle); // 注册事件处理并返回事件处理号

    void removeEventHandle(uint32_t id); 

    void clear();

    void fireEvent(const Event &event);

private:
    uint32_t _idGen = 0; // 事件处理号生成器
    std::map<uint32_t, std::string> _m1; // 事件处理号到事件名称的表
    std::map<std::string, std::map<uint32_t, EventHandle>> _m2; // 事件名称到事件处理表的表，其中事件处理表是事件处理号到事件处理的表
};

#endif /* wukong_event_h */
