/* For copyright information please refer to files in the COPYRIGHT directory
 */
#ifndef LIST_H
#include "region.h"

typedef struct listNode ListNode;
struct listNode {
    ListNode *next;
    void *value;
};

typedef struct list {
	int size;
    ListNode *head;
    ListNode *tail;
} List;

List *newList(Region *r);
List *newListNoRegion();
ListNode *newListNode(void *value, Region *r);
ListNode *newListNodeNoRegion(void *value);

void listRemoveNoRegion2(List *l, void *v);
void listAppendNoRegion(List *list, void *value);
void listAppend(List *list, void *value, Region *r);

void listAppendToNode(List *list, ListNode *node, void *value, Region *r);
void listRemove(List *list, ListNode *node);
void listRemoveNoRegion(List *list, ListNode *node);
void deleteListNoRegion(List *list);
void clearListNoRegion(List *list);

#define LIST_H
#endif
