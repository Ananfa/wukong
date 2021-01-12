//
//  timelink.h
//  BattleServer
//
//  Created by Xianke Liu on 2018/11/9.
//  Copyright © 2018年 Dena. All rights reserved.
//

#ifndef timelink_h
#define timelink_h

#include <memory>

template <typename T>
class TimeLink {
public:
    class Node {
    public:
        std::shared_ptr<T> data;
        time_t time;
        
        Node *next;
        Node *prev;
    };
    
public:
    TimeLink(): _head(nullptr), _tail(nullptr) {}
    
    void push(Node *node);
    Node* pop();
    void erase(Node *node);
    void moveToTail(Node *node);
    void clear();
    
    Node* getHead() { return _head; }
    Node* getTail() { return _tail; }
    
private:
    Node *_head;
    Node *_tail;
};

template <typename T>
void TimeLink<T>::push(Node *node) {
    node->time = time(nullptr);
    
    if (_head) {
        assert(_tail && !_tail->next);
        node->next = nullptr;
        node->prev = _tail;
        _tail->next = node;
        _tail = node;
    } else {
        assert(!_tail);
        node->next = nullptr;
        node->prev = nullptr;
        _head = node;
        _tail = node;
    }
}

template <typename T>
typename TimeLink<T>::Node* TimeLink<T>::pop() {
    if (!_head) {
        return nullptr;
    }
    
    Node *node = _head;
    _head = _head->next;
    if (_head) {
        _head->prev = nullptr;
    } else {
        assert(_tail == node);
        _tail = nullptr;
    }
    
    node->next = nullptr;
    node->prev = nullptr;
    
    return node;
}

template <typename T>
void TimeLink<T>::erase(Node *node) {
    Node *prev = node->prev;
    Node *next = node->next;
    
    if (prev) {
        prev->next = next;
    }
    
    if (next) {
        next->prev = prev;
    }
    
    if (_head == node) {
        _head = next;
    }
    
    if (_tail == node) {
        _tail = prev;
    }
    
    delete node;
}

template <typename T>
void TimeLink<T>::moveToTail(Node *node) {
    Node *next = node->next;
    
    node->time = time(nullptr);
    
    assert(next || _tail == node);
    
    if (next) {
        Node *prev = node->prev;
        next->prev = prev;
        
        if (prev) {
            prev->next = next;
        }
        
        if (_head == node) {
            _head = next;
        }
        
        node->next = nullptr;
        node->prev = _tail;
        _tail->next = node;
        _tail = node;
    }
}

template <typename T>
void TimeLink<T>::clear() {
    Node *node = _head;
    while (node) {
        Node *next = node->next;
        delete node;
        node = next;
    }

    _head = nullptr;
    _tail = nullptr;
}

#endif /* timelink_h */
