#ifndef CONTAINERS_H
#define CONTAINERS_H

#include <types.h>

class pool_list_base
{
public:
    struct node_base
    {
        node_base* prev;
        node_base* next;
    };

    S32 size() const;
    S32 max_size() const;
    bool full() const;
    bool empty() const;

    void push_front();
    void push_back();
    void create(S32 node_size, S32 max_size, void* buffer);
    void reset(S32 node_size);
    node_base* insert(node_base* it);
    node_base* erase(node_base* it);

protected:
    S32 _size;
    S32 _max_size;
    node_base head;
    node_base tail;
    node_base* stack;
    void* buffer;

    node_base* alloc()
    {
        node_base* node = stack;

        stack = stack->next;

        return node;
    }

    void free(node_base* it)
    {
        it->next = stack;

        stack = it;
    }

    void link(node_base* a, node_base* b);
    void unlink(node_base* a, node_base* b);
};

class tier_queue_allocator
{
public:
    struct block_data
    {
        U8 prev;
        U8 next;
        U16 flags;
        void* data;
    };

private:
    block_data* blocks;
    U32 _unit_size;
    U32 _block_size;
    U32 _block_size_shift;
    U32 _max_blocks;
    U32 _max_blocks_shift;
    U8 head;
};

class tier_queue
{
private:
    U32 first;
    U32 _size;
    U32 wrap_mask;
    tier_queue_allocator* alloc;
    U8 blocks[32];
};

#endif