/* For copyright information please refer to files in the COPYRIGHT directory
 */
#include "list.h"

List *newList(Region *r) {
    List *l = (List *)region_alloc(r, sizeof (List));
    l->head = l->tail = NULL;
    l->size = 0;
    return l;
}

List *newListNoRegion() {
	List *l = (List *)malloc(sizeof (List));
	    l->head = l->tail = NULL;
	    l->size = 0;
	    return l;
}

ListNode *newListNodeNoRegion(void *value) {
    ListNode *l = (ListNode *)malloc(sizeof (ListNode));
    l->next = NULL;
    l->value = value;
    return l;
}
ListNode *newListNode(void *value, Region *r) {
    ListNode *l = (ListNode *)region_alloc(r, sizeof (ListNode));
    l->next = NULL;
    l->value = value;
    return l;
}


void listAppendNoRegion(List *list, void *value) {
    ListNode *ln = newListNodeNoRegion(value);
    if(list->head != NULL) {
        list->tail = list->tail->next = ln;
    } else {
        list->head = list->tail = ln;

    }
    list->size++;
}
void listAppend(List *list, void *value, Region *r) {
    ListNode *ln = newListNode(value, r);
    if(list->head != NULL) {
        list->tail = list->tail->next = ln;
    } else {
        list->head = list->tail = ln;
    }
    list->size++;
}

void listAppendToNode(List *list, ListNode *node, void *value, Region *r) {
    ListNode *ln = newListNode(value, r);
    if(node->next != NULL) {
        ln->next = node->next;
        node->next = ln;
    } else {
        node->next = list->tail = ln;
    }
    list->size++;
}

void listRemove(List *list, ListNode *node) {
    ListNode *prev = NULL, *curr = list->head;
    while(curr != NULL) {
        if(curr == node) {
            if(prev == NULL) {
                list->head = node->next;
            } else {
                prev->next = node->next;
            }
            /*free(node); */
            break;
        }
        prev = curr;
        curr = curr->next;
    }
    if(list->tail == node) {
        list->tail = prev;
    }
    list->size--;

}
void listRemoveNoRegion2(List *l, void *v) {
	ListNode *node = l->head;
	while(node!=NULL) {
		if(node->value == v) {
			listRemoveNoRegion(l, node);
			break;
		}
		node = node->next;
	}
}
void listRemoveNoRegion(List *list, ListNode *node) {
    ListNode *prev = NULL, *curr = list->head;
    while(curr != NULL) {
        if(curr == node) {
            if(prev == NULL) {
                list->head = node->next;
            } else {
                prev->next = node->next;
            }
            free(node);
            break;
        }
        prev = curr;
        curr = curr->next;
    }
    if(list->tail == node) {
        list->tail = prev;
    }
    list->size--;
}

void deleteListNoRegion(List *list) {

	free(list);
}
void clearListNoRegion(List *list) {
	while(list->head != NULL) listRemoveNoRegion(list, list->head);
}



