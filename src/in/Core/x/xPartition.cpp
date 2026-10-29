#include "xPartition.h"

#include "xMemMgr.h"

#include <string.h>

// func_8004FFBC
void xPartitionReset()
{
}

// func_8004FFC0
static PartLink* PartitionGetFreeLink()
{
    return (PartLink*)xMEMALLOC(sizeof(PartLink), 0);
}

// func_8004FFEC
static void PartitionSpaceReset(PartSpace* space)
{
    memset(space, 0, sizeof(PartSpace));
}

// func_80050014
static void PartitionSpaceInsert(PartSpace* space, void* data)
{
    space->total++;
    PartLink* head = &space->head;
    PartLink* tmp = head;
    while (tmp->next != NULL)
    {
        tmp = tmp->next;
    }
    head = PartitionGetFreeLink();
    tmp->next = head;
    tmp->next->data = data;
    tmp->next->next = NULL;
}

