#ifndef ZQUEUE_H
#define ZQUEUE_H

#include "types.h"

template <class T, S32 max_size> class zQueue
{
public:
    S32 next_offset(S32 offset) { return (first + offset) % max_size; }

    S32 get_size_queue() const { return size; }

    void clear_queue()
    {
        first = size = 0;
    }

    T* fetch_queue_ptr(S32 offs)
    {
        if (size <= 0) {
            return NULL;
        }

        return &data[next_offset(offs)];
    }

    void get_queue(T& elem)
    {
        if (size <= 0) {
            return;
        }

        elem = data[first];
        first = next_offset(1);
        size--;
    }

    void add_queue(T& elem)
    {
        data[next_offset(size)] = elem;
        next();
    }

    void next()
    {
        if (size >= max_size) {
            first = next_offset(1);
        } else {
            size++;
        }
    }

protected:
    S32 first;
    S32 size;
    T data[max_size];
};

#endif
