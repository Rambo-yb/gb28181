#ifndef __LIST_H__
#define __LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

void* ListCreate();

void ListDestory(void* list_head);

int ListPush(void* list_head, void* data, int data_size);

int ListPop(void* list_head, void* data, int data_size);

int ListSize(void* list_head);

void* ListGet(void* list_head, int index);

#ifdef __cplusplus
};
#endif

#endif