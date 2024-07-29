/**
 * Created by Lucas (Deuce) Palmer
*/
#include "my402list.h"
#include <stdlib.h>

int  My402ListLength(My402List* list){
    return list->num_members;
}
int  My402ListEmpty(My402List* list){
    return list->num_members <= 0;
}

int  My402ListAppend(My402List* list, void* obj){
    My402ListElem *elem = (My402ListElem *)malloc(sizeof(My402ListElem));
    if (elem == NULL){//malloc memory allocation failed
        return 0;
    }
    elem->obj = obj;
    elem->next = &list->anchor;
    if (list->num_members == 0){//if the list is empty, connect elem to anchor
        elem->prev = &list->anchor;
        list->anchor.next = elem;
        list->anchor.prev = elem;
    } else {//if the list is not empty, add elem behind the last element
        elem->prev = My402ListLast(list);
        My402ListLast(list)->next = elem;
        list->anchor.prev = elem;
    }
    list->num_members++;
    return 1;
}
int  My402ListPrepend(My402List* list, void* obj){
    My402ListElem *elem = (My402ListElem *)malloc(sizeof(My402ListElem));
    if (elem == NULL){//malloc memory allocation failed
        return 0;
    }
    elem->obj = obj;
    elem->prev = &list->anchor;
    if (list->num_members == 0){//if the list is empty, connect elem to anchor
        elem->next = &list->anchor;
        list->anchor.prev = elem;
        list->anchor.next = elem;
    } else {//if the list is not empty, add elem in front of the first element
        elem->next = My402ListFirst(list);
        My402ListFirst(list)->prev = elem;
        list->anchor.next = elem;
    }
    list->num_members++;
    return 1;
}
void My402ListUnlink(My402List* list, My402ListElem* elem){
    //connect the previous and next pointers of elem to each other, remvoing elem as an intermediary
    My402ListElem *before = elem->prev;
    before->next = elem->next;
    elem->next->prev = before;
    free(elem);//free the My402ListElem from memory
    list->num_members--;
}
void My402ListUnlinkAll(My402List* list){
    //iterate through the list and remove each element one-by-one
    My402ListElem *iter = list->anchor.next;
    while (iter != &list->anchor){
        My402ListUnlink(list, iter);
        iter = iter->next;
    }
}
int  My402ListInsertAfter(My402List* list, void* obj, My402ListElem* node){
    if (node == NULL){//append obj if node does not exist
        My402ListAppend(list, obj);
    } else {
        My402ListElem *elem = (My402ListElem *)malloc(sizeof(My402ListElem));
        if (elem == NULL){//malloc memory allocation failed
            return 0;
        }
        elem->obj = obj;
        elem->prev = node;
        elem->next = node->next;
        node->next->prev = elem;
        node->next = elem;
        list->num_members++;
    }
    return 1;
}
int  My402ListInsertBefore(My402List* list, void* obj, My402ListElem* node){
    if (node == NULL){//prpend obj if node does not exist
        My402ListPrepend(list, obj);
    } else {
        My402ListElem *elem = (My402ListElem *)malloc(sizeof(My402ListElem));
        if (elem == NULL){//malloc memory allocation failed
            return 0;
        }
        elem->obj = obj;
        elem->next = node;
        elem->prev = node->prev;
        node->prev->next = elem;
        node->prev = elem;
        list->num_members++;
    }
    return 1;
}

My402ListElem *My402ListFirst(My402List* list){
    if (list->num_members == 0){//return NULL if list is empty
        return NULL;
    } else {
        return list->anchor.next;
    }
}
My402ListElem *My402ListLast(My402List* list){
    if (list->num_members == 0){//return NULL if list is empty
        return NULL;
    } else {
        return list->anchor.prev;
    }
}
My402ListElem *My402ListNext(My402List* list, My402ListElem* node){
    if (node->next == &list->anchor){//return NULL if node is last
        return NULL;
    } else {
        return node->next;
    }
}
My402ListElem *My402ListPrev(My402List* list, My402ListElem* node){
    if (node->prev == &list->anchor){//return NULL if node is first
        return NULL;
    } else {
        return node->prev;
    }
}

My402ListElem *My402ListFind(My402List* list, void* obj){
    //iterate through the list and check for obj match
    My402ListElem *iter = list->anchor.next;
    while (iter != &list->anchor){
        if (iter->obj == obj){//return current My402ListElem if its obj is equal
            return iter;
        }
        iter = iter->next;
    }
    return NULL;
}

int My402ListInit(My402List* list){
    list->num_members = 0;//initiate empty list with 0 members
    list = (My402List *)malloc(sizeof(My402List));
    if (list == NULL){//malloc memory allocation failed
        return 0;
    }
    return 1;
}