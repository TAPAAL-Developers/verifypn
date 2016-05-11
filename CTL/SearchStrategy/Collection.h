#ifndef COLLECTION_H
#define COLLECTION_H

#include <stddef.h>
#include <queue>
#include <iostream>

namespace SearchStrategy {

template<class T, class Container>
class Collection {
public:
    Container c;
    Collection(Container S = Container()) : c(S){}
    void pop() { c.pop(); }
    T top() { return c.top(); }
    void push(T e) {c.push(e); }
    bool empty(){ return c.empty(); }
    void clear() {
        while(!c.empty())
            c.pop();
    }
    std::size_t size() { return c.size(); }
};

template<class T>
class Collection<T, std::queue<T>> {
    using Container = std::queue<T>;
public:
    Container c;
    Collection(Container S = Container()) : c(S){}
    void pop() { c.pop(); }
    T top() { return c.front(); }
    void push(T e) {c.push(e); }
    bool empty(){ return c.empty(); }
    void clear() {
        while(!c.empty())
            c.pop();
    }
    std::size_t size() { return c.size(); }
};

}

#endif // COLLECTION_H
