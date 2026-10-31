#include "xMemoryManagerGeneral.h"

void xMemoryManagerGeneral::Init(void* start, U32 size, Dir dir, Strategy strategy, bool debugging)
{
    DoInit(start, size, debugging);

    freeEnd = freeStart = dir == Up ? (FreeNode*)start : (FreeNode*)xMEMADVANCE(start, (S32)(size - sizeof(FreeNode)));

    freeStart->next = NULL;
    freeStart->prev = NULL;
    freeStart->size = size - sizeof(FreeNode);

    this->dir = dir;
    this->strategy = strategy;
}

void* xMemoryManagerGeneral::GetCurrentEnd() const
{
    if (dir == Up)
        return freeEnd == NULL ? GetArenaEnd() : NextBlockOfMemory(freeEnd);
    else
        return freeEnd == NULL ? GetArenaStart() : freeEnd + 1;
}

void* xMemoryManagerGeneral::DoAllocate(U32 size, U32 options)
{
    size = (size + 15) & ~15;

    FreeNode* node;

    if ((dir == Up && (options & 0x6) != 0x2) || dir == Down && (options & 0x6) != 0x4) {
        if (strategy == BestFit) {
            node = NULL;
            FreeNode* search = freeStart;
            while (search != NULL) {
                if (search->size >= size && (node == NULL || search->size < node->size))
                    node = search;
                search = search->next;
            }
        } else {
            node = freeStart;
            while (node != NULL) {
                if (node->size >= size)
                    break;
                node = node->next;
            }
        }

        if (node == NULL) {
            return node;
        }

        if (node->size - sizeof(FreeNode) > size) {
            FreeNode* newNode = dir == Up ? (FreeNode*)((U8*)node + sizeof(FreeNode) + size) : (FreeNode*)((U8*)node - sizeof(FreeNode) - size);
            newNode->next = node->next;
            if (newNode->next)
                newNode->next->prev = newNode;
            newNode->prev = node;
            node->next = newNode;
            newNode->size = node->size - size - sizeof(FreeNode);
            node->size = size;

            FreeNode* next = NextBlockOfMemory(node);

            if (node == freeEnd) {
                freeEnd = newNode;
            }
        }
    } else {
        if (strategy == BestFit) {
            node = NULL;
            FreeNode* search = freeEnd;
            while (search != NULL) {
                if (search->size >= size && (node == NULL || search->size < node->size))
                    node = search;
                search = search->prev;
            }
        } else {
            node = freeEnd;
            while (node != NULL) {
                if (node->size >= size)
                    break;
                node = node->prev;
            }
        }

        if (node == NULL) {
            return node;
        }

        if (node->size - sizeof(FreeNode) > size) {
            FreeNode* newNode = dir == Up ?
                (FreeNode*)((U8*)node + node->size - size) :
                (FreeNode*)((U8*)node - node->size + size);
            newNode->next = node->next;
            if (newNode->next)
                newNode->next->prev = newNode;
            newNode->prev = node;
            node->next = newNode;
            newNode->size = size;
            node->size -= size + sizeof(FreeNode);

            FreeNode* next = NextBlockOfMemory(node);

            if (node == freeEnd) {
                freeEnd = newNode;
            }

            FreeNode* swapNode = node;
            node = newNode;
        }
    }

    if (node->prev)
        node->prev->next = node->next;
    else
        freeStart = node->next;

    if (node->next)
        node->next->prev = node->prev;
    else
        freeEnd = node->prev;

    U32 nodeSize = node->size;

    AllocatedNode* allocatedNode = dir == Up ? (AllocatedNode*)node : (AllocatedNode*)xMEMADVANCE(node, -(S32)nodeSize);
    allocatedNode->size = nodeSize;

    return allocatedNode + 1;
}

void xMemoryManagerGeneral::DoFree(void* pointer)
{
    AllocatedNode* allocatedNode = (AllocatedNode*)pointer - 1;
    U32 size = allocatedNode->size;

    if (dir == Down) {
        allocatedNode = xMEMADVANCE(allocatedNode, size);
    }

    FreeNode* node = (FreeNode*)allocatedNode;
    node->prev = NULL;
    node->next = NULL;
    node->size = size;

    if (freeStart == NULL) {
        freeStart = freeEnd = node;
    } else {
        FreeNode* insertBefore = freeStart;
        if (dir == Up) {
            while (insertBefore != NULL && node > insertBefore)
                insertBefore = insertBefore->next;
        } else {
            while (insertBefore != NULL && node < insertBefore)
                insertBefore = insertBefore->next;
        }

        if (insertBefore) {
            node->prev = insertBefore->prev;
            node->next = insertBefore;
            insertBefore->prev = node;
            if (node->prev == NULL)
                freeStart = node;
            else
                node->prev->next = node;
        } else {
            freeEnd->next = node;
            node->prev = freeEnd;
            freeEnd = node;
        }

        MergeNodes(node);
    }
}

void* xMemoryManagerGeneral::DoReallocate(void* pointer, U32 size, U32 options)
{
    size = xALIGN(size, 16);

    AllocatedNode* allocatedNode = (AllocatedNode*)pointer - 1;
    U32 nodeSize = allocatedNode->size;

    if (size < nodeSize && nodeSize - size >= 32) {
        allocatedNode = ShortenNode(allocatedNode, size);

        return allocatedNode + 1;
    }

    if (size <= nodeSize) {
        return pointer;
    }

    FreeNode* afterNode = freeStart;
    if (dir == Up) {
        while (afterNode != NULL && (FreeNode*)allocatedNode > afterNode)
            afterNode = afterNode->next;
    } else {
        while (afterNode != NULL && (FreeNode*)allocatedNode < afterNode)
            afterNode = afterNode->next;
    }

    FreeNode* beforeNode = afterNode ? afterNode->prev : freeEnd;
    U32 sizeBeforeNode = 0;
    U32 sizeAfterNode = 0;
    if (dir == Up) {
        if (beforeNode && NextBlockOfMemory(beforeNode) == (FreeNode*)allocatedNode) {
            sizeBeforeNode = beforeNode->size;
        }
        if (afterNode && (FreeNode*)xMEMADVANCE(allocatedNode + 1, nodeSize) == afterNode)
            sizeAfterNode = afterNode->size;
    } else {
        if (beforeNode && (FreeNode*)xMEMADVANCE(allocatedNode + 1, nodeSize) == xMEMADVANCE(beforeNode, -(S32)beforeNode->size)) {
            sizeBeforeNode = beforeNode->size;
        }
        if (afterNode && afterNode + 1 == (FreeNode*)allocatedNode) {
            sizeAfterNode = afterNode->size;
        }
    }

    if (sizeAfterNode != 0 && nodeSize + sizeAfterNode + sizeof(FreeNode) >= size)
        sizeBeforeNode = 0;
    else if (sizeBeforeNode != 0 && nodeSize + sizeBeforeNode + sizeof(FreeNode) >= size)
        sizeAfterNode = 0;
    else if (nodeSize + sizeAfterNode + sizeBeforeNode + sizeof(FreeNode) < size)
        sizeAfterNode = sizeBeforeNode = 0;

    if (sizeAfterNode == 0 && sizeBeforeNode == 0) {
        void* newPointer = DoAllocate(size, options);
        if (newPointer == NULL)
            return NULL;

        xMemoryCopyUpA128(newPointer, pointer, nodeSize);
        DoFree(pointer);

        return newPointer;
    }

    const void* dataStart = allocatedNode + 1;

    U32 neededSize = size - nodeSize;

    if ((sizeAfterNode != 0 && dir == Up) || (sizeBeforeNode != 0 && dir == Down)) {
        FreeNode* working = (dir == Up ? afterNode : beforeNode);
        if (neededSize < working->size) {
            allocatedNode->size += neededSize;
            if (dir == Up) {
                FreeNode* newNode = xMEMADVANCE(working, neededSize);
                *newNode = *working;
                newNode->size -= neededSize;
                if (newNode->prev)
                    newNode->prev->next = newNode;
                else
                    freeStart = newNode;
                if (newNode->next)
                    newNode->next->prev = newNode;
                else
                    freeEnd = newNode;
            } else {
                working->size -= neededSize;
            }
            neededSize = 0;
        } else {
            allocatedNode->size += working->size + sizeof(AllocatedNode);
            if (working->prev)
                working->prev->next = working->next;
            else
                freeStart = working->next;
            if (working->next)
                working->next->prev = working->prev;
            else
                freeEnd = working->prev;
        }
    }

    if (neededSize != 0 && ((sizeBeforeNode != 0 && dir == Up) || (sizeAfterNode != 0 && dir == Down))) {
        FreeNode* working = (dir == Up ? beforeNode : afterNode);
        AllocatedNode* newAllocatedNode;
        U32 newSpace;

        if (neededSize < working->size) {
            working->size -= neededSize;
            newSpace = neededSize;
            newAllocatedNode = xMEMADVANCE(allocatedNode, -(S32)neededSize);
            if (dir == Down) {
                FreeNode* newNode = xMEMADVANCE(working, -(S32)neededSize);
                *newNode = *working;
                if (newNode->next)
                    newNode->next->prev = newNode;
                else
                    freeEnd = newNode;
                if (newNode->prev)
                    newNode->prev->next = newNode;
                else
                    freeStart = newNode;
            }
        } else {
            if (working->prev)
                working->prev->next = working->next;
            else
                freeStart = working->next;
            if (working->next)
                working->next->prev = working->prev;
            else
                freeEnd = working->prev;
            newSpace = working->size + sizeof(AllocatedNode);
            if (dir == Up)
                newAllocatedNode = (AllocatedNode*)working;
            else
                newAllocatedNode = (AllocatedNode*)xMEMADVANCE(working, -(S32)working->size);
        }

        *newAllocatedNode = *allocatedNode;
        allocatedNode = newAllocatedNode;
        allocatedNode->size += newSpace;
    }

    void* dataDest = allocatedNode + 1;
    if (dataDest < dataStart)
        xMemoryCopyUpA128(dataDest, dataStart, size);
    else if (dataDest > dataStart)
        xMemoryCopyDownA128(dataDest, dataStart, size);

    return allocatedNode + 1;
}

U32 xMemoryManagerGeneral::DoGetBlockSize(void* pointer) const
{
    return ((AllocatedNode*)pointer - 1)->size;
}

void xMemoryManagerGeneral::MergeNodes(FreeNode* node)
{
    if (node->prev && NextBlockOfMemory(node->prev) == node) {
        node->prev->size += node->size + sizeof(FreeNode);
        node->prev->next = node->next;
        if (node->next)
            node->next->prev = node->prev;
        else
            freeEnd = node->prev;
        node = node->prev;
    }

    if (node->next && NextBlockOfMemory(node) == node->next) {
        node->size += node->next->size + sizeof(FreeNode);
        node->next = node->next->next;
        if (node->next)
            node->next->prev = node;
        else
            freeEnd = node;
    }
}

xMemoryManagerGeneral::AllocatedNode* xMemoryManagerGeneral::ShortenNode(AllocatedNode* allocatedNode, U32 size)
{
    U32 nodeSize = allocatedNode->size;
    FreeNode* newNode;

    if (dir == Up)
        newNode = (FreeNode*)xMEMADVANCE(allocatedNode + 1, size);
    else {
        AllocatedNode* oldAllocatedNode = allocatedNode;
        allocatedNode = xMEMADVANCE(allocatedNode, nodeSize - size);
        newNode = (FreeNode*)(allocatedNode - 1);
        xMemoryCopyDownA128(allocatedNode, oldAllocatedNode, size + sizeof(AllocatedNode));
    }

    allocatedNode->size = size;

    newNode->next = NULL;
    newNode->prev = NULL;
    newNode->size = nodeSize - size - sizeof(FreeNode);

    if (freeStart == NULL)
        freeStart = freeEnd = newNode;
    else {
        FreeNode* insertBefore = freeStart;
        if (dir == Up) {
            while (insertBefore != NULL && insertBefore < newNode)
                insertBefore = insertBefore->next;
        } else {
            while (insertBefore != NULL && insertBefore > newNode)
                insertBefore = insertBefore->next;
        }
        if (insertBefore == NULL) {
            freeEnd->next = newNode;
            newNode->prev = freeEnd;
            freeEnd = newNode;
        } else {
            if (insertBefore->prev == NULL)
                freeStart = newNode;
            else
                insertBefore->prev->next = newNode;
            newNode->prev = insertBefore->prev;
            newNode->next = insertBefore;
            insertBefore->prev = newNode;
        }
    }

    MergeNodes(newNode);

    return allocatedNode;
}
