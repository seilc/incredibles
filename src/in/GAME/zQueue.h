#ifndef ZQUEUE_H
#define ZQUEUE_H

#include "types.h"

template <class T, S32 max_size> class zQueue
{
public:
    S32 get_size_queue() const
    {
        return size;
    }

    T* fetch_queue_ptr(S32 index)
    {
        if (size <= 0)
        {
            return NULL;
        }

        return &data[next_offset(index)];
    }

    void get_queue(T& d)
    {
        if (size <= 0)
        {
            return;
        }

        d = data[first];
        first = next_offset(1);
        size--;
    }

    S32 next_offset(S32 index);

private:
    S32 first;
    S32 size;
    T data[max_size];
};

#endif