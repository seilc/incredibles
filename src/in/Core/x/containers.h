#ifndef CONTAINERS_H
#define CONTAINERS_H

#include "xMemMgr.h"

template <class T>
class static_queue
{
public:
    class iterator
    {
    public:
        bool operator!=(const iterator& c) const { return _it != c._it; }
        iterator& operator++() { *this += 1; return *this; }

        iterator& operator+=(int i)
        {
            _it = _owner->mod_max_size(_it + i);
            return *this;
        }

        T& operator*() const { return _owner->_buffer[_it]; }
        
    public:
        size_t _it;
        static_queue* _owner;
    };

    void init(size_t size)
    {
        size_t power = 0;
        size++;
        while (size > 1) { size >>= 1; power++; }
        _max_size = (1 << power);
        _max_size_mask = _max_size - 1;
        _buffer = (T*)xMEMALLOC(_max_size * sizeof(T), 0);
        clear();
    }

    size_t size() const { return _size; }
    size_t max_size() const { return _max_size - 1; }
    bool empty() const { return size() == 0; }
    bool full() const { return size() == max_size(); }
    void clear() { _first = _size = 0; }
    T& push_front() { _size++; _first = mod_max_size(_first-1); return front(); }
    void pop_back() { _size--; }
    iterator begin() const { return create_iterator(_first); }
    iterator end() const { return create_iterator(mod_max_size(_first + _size)); }
    T& front() { return *begin(); }

    void erase(const iterator& first, const iterator& last)
    {
        if (first._it == _first) {
            size_t tail = _first + _size;
            _first = last._it;
            _size = mod_max_size(tail - _first);
            return;
        }

        size_t diff = mod_max_size(last._it - first._it);
        _size -= diff;
    }

private:
    iterator create_iterator(size_t i) const { iterator r; r._it = i; r._owner = (static_queue*)this; return r; }
    size_t mod_max_size(size_t v) const { return v & _max_size_mask; }

public:
    size_t _first;
    size_t _size;
    size_t _max_size;
    size_t _max_size_mask;
    T* _buffer;

    friend class iterator;
};

class pool_list_base
{
public:
    struct node_base
    {
        node_base* prev;
        node_base* next;
    };

    S32 size() const { return _size; }
    S32 max_size() const { return _max_size; }
    bool empty() const { return _size == 0; }
    bool full() const { return size() == max_size(); }

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

template <class T>
class pool_list : public pool_list_base
{
public:
    struct node_type
    {
        node_type* prev;
        node_type* next;
        T value;
    };

    enum { NODE_SIZE = sizeof(node_type) };

    void create(S32 max_size, void* buffer)
    {
        pool_list_base::create(NODE_SIZE, max_size, buffer);
    }

    void reset()
    {
        pool_list_base::reset(NODE_SIZE);
    }

    node_type* begin() const { return (node_type*)head.next; }
    node_type* end() const { return (node_type*)&tail; }
    T& back() { return ((node_type*)tail.prev)->value; }

    node_type* erase(node_type* it)
    {
        return (node_type*)pool_list_base::erase((node_base*)it);
    }

    node_type* get_iterator(const T* value)
    {
        U8* p = ((U8*)value - sizeof(node_base));
        return (node_type*)p;
    }

    void push_back() { pool_list_base::push_back(); }
};

template <class T>
class linear_pool
{
public:
    void create(S32 max_size, T* buffer)
    {
        _size = 0;
        _max_size = max_size;
        _buffer = buffer;
    }

    void create(S32 max_size) { create(max_size, new (eMemStaticType_0, 0) T[max_size]); }
    void clear() { _size = 0; }
    T* begin() { return _buffer; }
    T* end() { return _buffer + _size; }
    T& back() { return _buffer[_size-1]; }
    void push_back() { _size++; }

    void erase(T* it)
    {
        _size--;
        T* tail = _buffer + _size;
        if (it != tail)
            *it = *tail;
    }

private:
    S32 _size;
    S32 _max_size;
    T* _buffer;
};

class tier_queue_allocator
{
private:
    struct block_data
    {
        U8 prev;
        U8 next;
        U16 flags;
        void* data;
    };

public:
    enum { MAX_BLOCKS = 32, FLAG_TAKEN = (1<<0) };

    void init(size_t unit_size, size_t block_size, size_t max_blocks)
    {
        _unit_size = (unit_size + 3) & ~3;
        _block_size_shift = log2_ceil(block_size);
        _block_size = 1 << _block_size_shift;
        _max_blocks_shift = log2_ceil(max_blocks);
        _max_blocks = 1 << _max_blocks_shift;

        blocks = (block_data*)xMEMALLOC(_max_blocks * sizeof(block_data), 0);

        for (size_t i = 0, end = _max_blocks; i < end; i++)
            blocks[i].data = 0;
        clear();

        U8 arrayTempAllocs[31];
        for (S32 i = 0; i < MAX_BLOCKS-1; i++) {
            arrayTempAllocs[i] = alloc_block();
        }
        for (S32 i = 0; i < MAX_BLOCKS-1; i++) {
            free_block(arrayTempAllocs[i]);
        }
    }

    bool full() const
    {
        return head == blocks[head].next;
    }

    void clear()
    {
        head = 0;

        for (size_t imask = _max_blocks - 1, i = 0, end = _max_blocks; i < end; i++) {
            blocks[i].prev = (i-1) & imask;
            blocks[i].next = (i+1) & imask;
        }
    }

    U8 alloc_block()
    {
        U8 index = head;
        block_data& block = blocks[index];

        head = block.next;

        blocks[block.prev].next = block.next;
        blocks[block.next].prev = block.prev;

        if (!block.data) block.data = alloc_block_data();

        return index;
    }

    void free_block(U8 index)
    {
        block_data& block = blocks[index];

        block.next = head;
        block.prev = blocks[head].prev;

        blocks[block.prev].next = index;
        blocks[block.next].prev = index;

        head = index;
    }

    void* get_block(size_t at) const
    {
        return blocks[at].data;
    }

    size_t block_size() const { return _block_size; }
    size_t block_size_shift() const { return _block_size_shift; }
    size_t mod_block_size(size_t i) const { return i & (_block_size-1); }

private:
    size_t log2_ceil(size_t v) const
    {
        size_t power = 0;
        while (v > 1) { v >>= 1; power++; }
        return power;
    }

    void* alloc_block_data() const
    {
        return xMEMALLOC(_block_size * _unit_size, 0);
    }

private:
    block_data* blocks;
    size_t _unit_size;
    size_t _block_size;
    size_t _block_size_shift;
    size_t _max_blocks;
    size_t _max_blocks_shift;
    U8 head;
};

template <class T>
class tier_queue
{
public:
    class iterator
    {
    public:
        bool operator==(const iterator& c) const { return it == c.it; }
        bool operator!=(const iterator& c) const { return it != c.it; }
        iterator& operator++() { *this += 1; return *this; }
        iterator& operator--() { *this -= 1; return *this; }
        iterator& operator+=(S32 i) { it = owner->wrap_index(it + i); return *this; }
        iterator operator+(S32 i) const { iterator c = *this; c += i; return c; }
        iterator operator-=(S32 i) { return (*this += -i); }
        iterator operator-(S32 i) const { iterator c = *this; c -= i; return c; }
        T* operator->() const { return &(**this); }
        T& operator*() const { return owner->get_at(it); }
        size_t global_index() const { return it; }

    private:
        size_t it;
        tier_queue* owner;

        friend class tier_queue;
    };

    void init(tier_queue_allocator& alloc)
    {
        first = 0;
        _size = 0;
        wrap_mask = (alloc.block_size() << 5) - 1;
        this->alloc = &alloc;
    }

    bool front_full() const
    {
        if (!alloc->full()) return false;
        return alloc->mod_block_size(first) == 0;
    }

    size_t size() const { return _size; }
    bool empty() const { return _size == 0; }

    void clear()
    {
        size_t i = get_block(first);
        size_t end = wrap_block(i + get_block(alloc->block_size() + _size - 1));
        for (; i != end; i = wrap_block(i+1))
            alloc->free_block(blocks[i]);
        first = _size = 0;
    }

    void push_front()
    {
        _size++;

        size_t oldfirst = first;
        first = wrap_index(first-1);

        if (!alloc->mod_block_size(oldfirst)) {
            size_t block_index = get_block(first);
            blocks[block_index] = alloc->alloc_block();
        }
    }

    void pop_back()
    {
        if (_size <= 1) { clear(); return; }

        _size--;

        size_t last = wrap_index(first + _size);
        if (alloc->mod_block_size(last) == 0) {
            size_t block_index = get_block(last);
            alloc->free_block(blocks[block_index]);
        }
    }

    iterator begin() const { return create_iterator(first); }
    iterator end() const { return create_iterator(wrap_index(first + _size)); }
    T& front() { return *begin(); }
    T& back() { return *(end() - 1); }
    T& operator[](int i) { return get_at(wrap_index(first + i)); }

    static iterator dummy_iterator()
    {
        iterator it;
        it.it = 0;
        it.owner = 0;
        return it;
    }

private:
    T& get_at(size_t i) const
    {
        size_t block_index = blocks[get_block(i)];

        T* data = (T*)alloc->get_block(block_index);

        size_t unit_index = alloc->mod_block_size(i);
        return data[unit_index];
    }

    size_t wrap_index(size_t i) const { return i & wrap_mask; }
    size_t wrap_block(size_t i) const { return i % 256; }
    size_t get_block(size_t i) const { return i >> alloc->block_size_shift(); }

    iterator create_iterator(size_t i) const
    {
        iterator it;
        it.it = i;
        it.owner = (tier_queue*)this;
        return it;
    }

private:
    size_t first;
    size_t _size;
    size_t wrap_mask;
    tier_queue_allocator* alloc;
    U8 blocks[tier_queue_allocator::MAX_BLOCKS];

    friend class iterator;
};

#endif