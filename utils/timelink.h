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
    TimeLink(): head_(nullptr), tail_(nullptr) {}
    
    void push(Node *node);
    Node* pop();
    void erase(Node *node);
    void moveToTail(Node *node);
    void clear();
    
    Node* getHead() { return head_; }
    Node* getTail() { return tail_; }
    
private:
    Node *head_;
    Node *tail_;
};

template <typename T>
void TimeLink<T>::push(Node *node) {
    node->time = time(nullptr);
    
    if (head_) {
        assert(tail_ && !tail_->next);
        node->next = nullptr;
        node->prev = tail_;
        tail_->next = node;
        tail_ = node;
    } else {
        assert(!tail_);
        node->next = nullptr;
        node->prev = nullptr;
        head_ = node;
        tail_ = node;
    }
}

template <typename T>
typename TimeLink<T>::Node* TimeLink<T>::pop() {
    if (!head_) {
        return nullptr;
    }
    
    Node *node = head_;
    head_ = head_->next;
    if (head_) {
        head_->prev = nullptr;
    } else {
        assert(tail_ == node);
        tail_ = nullptr;
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
    
    if (head_ == node) {
        head_ = next;
    }
    
    if (tail_ == node) {
        tail_ = prev;
    }
    
    delete node;
}

template <typename T>
void TimeLink<T>::moveToTail(Node *node) {
    Node *next = node->next;
    
    node->time = time(nullptr);
    
    assert(next || tail_ == node);
    
    if (next) {
        Node *prev = node->prev;
        next->prev = prev;
        
        if (prev) {
            prev->next = next;
        }
        
        if (head_ == node) {
            head_ = next;
        }
        
        node->next = nullptr;
        node->prev = tail_;
        tail_->next = node;
        tail_ = node;
    }
}

template <typename T>
void TimeLink<T>::clear() {
    Node *node = head_;
    while (node) {
        Node *next = node->next;
        delete node;
        node = next;
    }

    head_ = nullptr;
    tail_ = nullptr;
}

#endif /* timelink_h */
