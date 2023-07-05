#ifndef T_QUEUE_H_INCLUDED
#define T_QUEUE_H_INCLUDED

#include <deque>

// ==============================/  generic (templated) queue type   /==============================

// Using the template keyword, define a placeholder T for our "input" type for the template
template <class T>
class t_queue {

private:
    std::deque<T> dQueue;

public:
    t_queue() {}
    ~t_queue() { dQueue.clear(); }

    void push( T &r ) {
        dQueue.push_back( r );
    }

    T pop() {
        T result = dQueue.front();
        dQueue.pop_front();
        return result;
    }

    // element queries
    T &front() { return dQueue.front(); }
    T &back()  { return dQueue.back();  }
    // characteristics queries
    int  size()  { return dQueue.size();  }
    bool empty() { return dQueue.empty(); }
};

#endif // T_QUEUE_H_INCLUDED
