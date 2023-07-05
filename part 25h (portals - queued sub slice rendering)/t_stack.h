#ifndef T_STACK_H_INCLUDED
#define T_STACK_H_INCLUDED

#include <deque>

// ==============================/  generic (templated) stack type   /==============================

// Using the template keyword, define a placeholder T for our "input" type for the template
template <class T>
class t_stack {

private:
    std::deque<T> dStack;

public:
    t_stack() {}
    ~t_stack() { dStack.clear(); }

    void push( T &r ) {
        dStack.push_back( r );
    }

    T pop() {
        T result = dStack.back();
        dStack.pop_back();
        return result;
    }

    // element queries
    T &top()  { return dStack.back();  }
    // characteristics queries
    int  size()  { return dStack.size();  }
    bool empty() { return dStack.empty(); }
};

#endif // T_STACK_H_INCLUDED
