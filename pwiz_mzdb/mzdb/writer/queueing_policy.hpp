#ifndef PRODUCER_CONSUMER_H
#define PRODUCER_CONSUMER_H

#include "../threading/SpScQueue.hpp"
#include "../threading/Queue.hpp"

using namespace std;

namespace mzdb {


template<class Derived, class QueueT>
class mzAbstractQueueingPolicy {

protected:
    typedef typename QueueT::value_type UPtr;
    typedef typename UPtr::element_type Obj;

    typedef typename Obj::h_mz_t h_mz_t;
    typedef typename Obj::h_int_t h_int_t;
    typedef typename Obj::l_mz_t l_mz_t;
    typedef typename Obj::l_int_t l_int_t;

    QueueT& m_queue;

public:

    mzAbstractQueueingPolicy(QueueT& queue): m_queue(queue) {}

    void put(typename QueueT::value_type & element) {
        static_cast<Derived*>(this)->put(element);
    }

    void get(typename QueueT::value_type& element) {
        static_cast<Derived*>(this)->get(element);
    }

    size_t size() {
        return static_cast<Derived*>(this)->size();
    }
};

template<class T>
class FollyQueueingPolicy: public mzAbstractQueueingPolicy<FollyQueueingPolicy<T>,
                                                                                        folly::ProducerConsumerQueue<T> >{
public:

    typedef folly::ProducerConsumerQueue<T> QueueType;


    FollyQueueingPolicy( folly::ProducerConsumerQueue<T>& queue):
        mzAbstractQueueingPolicy<FollyQueueingPolicy<T>,
                                                folly::ProducerConsumerQueue<T> >(queue) {}


    void put(T& element) {
        while( ! m_queue.write(std::move(element)))
            continue;
    }

    void get(T& element) {
        while(! m_queue.read(element))
            continue;
    }

    size_t size() {
        return m_queue.sizeGuess();
    }
};


template<class T>
class BlockingQueueingPolicy: public mzAbstractQueueingPolicy<BlockingQueueingPolicy<T>,
                                                                                             BlockingQueue<T> >{
public:

    typedef BlockingQueue<T> QueueType;

    BlockingQueueingPolicy(BlockingQueue<T>& queue):
        mzAbstractQueueingPolicy<BlockingQueueingPolicy<T>, BlockingQueue<T> >(queue) {}


    void put(T& element) {
        m_queue.put(std::move(element));
    }

    void get(T& element) {
       m_queue.get(element);
    }

    size_t size() {
        return m_queue.size();
    }
};

}
#endif // PRODUCER_H
