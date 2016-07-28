/*
 * Copyright 2014 CNRS.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @file Queue.hpp
 * @brief Simple blocking queue using boost::mutex and boost::condition variable. This a multi-producer multi-consumer implementation
 * @author Marc Dubois marc.dubois@ipbs.fr
 * @author Alexandre Burel alexandre.burel@unistra.fr
 */

#ifndef __QUEUE__
#define __QUEUE__

#include "deque"
#include "boost/thread/condition_variable.hpp"
#include "boost/thread/thread.hpp"
#include "boost/thread/mutex.hpp"

namespace mzdb {

template<typename T>
class BlockingQueue: private boost::noncopyable {

private:
    std::deque<T> buf;
    mutable boost::mutex mutex;
    boost::condition_variable cond;
    size_t maxSize;
    volatile bool m_isClosed;

public:
    typedef T value_type;

    inline explicit BlockingQueue(size_t max_size) : maxSize(max_size), m_isClosed(false) {}

    /** */
    inline void put(T& m) {
        boost::mutex::scoped_lock lock(mutex);
        while( buf.size() == maxSize ) {
            cond.wait(lock);
        }
        buf.push_back(std::move(m));
        cond.notify_one();
    }

    /** */
    inline void get(T& ret) {
        boost::mutex::scoped_lock lock(mutex);
        while ( buf.empty() && ! m_isClosed ) {
            cond.wait(lock);
        }

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
