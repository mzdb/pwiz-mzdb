#ifndef __QUEUE__
#define __QUEUE__

#include "deque"
#include "boost/thread/condition.hpp"
#include "boost/thread/thread.hpp"
#include "boost/thread/mutex.hpp"

namespace mzdb {

template<typename T>
class CycleCollectionQueue {

private:
    std::deque<T> buf;
    mutable boost::mutex mutex;
    boost::condition_variable cond;
    size_t maxSize;
    volatile bool m_isClosed;

public:

    inline explicit CycleCollectionQueue(size_t max_size) : maxSize(max_size), m_isClosed(false) {}

    /** */
    inline void put(T& m) {
        boost::mutex::scoped_lock lock(mutex);
        while( buf.size() == maxSize ) {
            cond.wait(lock);
        }
        buf.push_back(std::move(m)); // move semantics, done by default VC10 ?
        cond.notify_one();
    }

    /** */
    inline void get(T& ret) {
        boost::mutex::scoped_lock lock(mutex);
        while ( buf.empty() && ! m_isClosed ) {
            cond.wait(lock);
        }

        //if (buf.empty() && m_isClosed) { // other thread are waiting
        //    return 0;
        //}

        ret = std::move(buf.front());
        buf.pop_front(); //remove the cycle collection which is invalidated
        cond.notify_one();
    }

    /** */
    inline size_t size() const {
        boost::mutex::scoped_lock lock(mutex);
        return buf.size();
    }

    /** */
    inline T& back() {
        boost::mutex::scoped_lock lock(mutex);
        return buf.back();
    }

    /** */
    inline T& front() {
        boost::mutex::scoped_lock lock(mutex);
        return buf.front();
    }

    /** */
    inline bool empty() const {
        boost::mutex::scoped_lock lock(mutex);
        return buf.empty();
    }

    /** */
    inline std::deque<T>& getQueue() {return buf;}

    /** */
    inline void close() {
        boost::mutex::scoped_lock lock(mutex);
        m_isClosed = true;
        cond.notify_one(); // TODO: really need this one ?
    }

    /** */
    inline bool isClosed() const {
        boost::mutex::scoped_lock lock(mutex);
        return m_isClosed;
    }


};

}//end namespace
#endif
