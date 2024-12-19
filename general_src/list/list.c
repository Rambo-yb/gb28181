#include <string.h>
#include <stdlib.h>
#include "list.h"

typedef struct {
    void* data;
    int data_size;
    void* next_node;
} ListNode;

void* ListCreate() {
    ListNode* new_node = (ListNode*)malloc(sizeof(ListNode));
    if (new_node == NULL) {
        return NULL;
    }

    memset(new_node, 0, sizeof(ListNode));
    return new_node;
}

void ListDestory(void* list_head){
    if (list_head == NULL) {
        return ;
    }

    ListNode* cur_node = (ListNode*) list_head;
    while (cur_node->next_node != NULL)
    {
        ListNode* node = cur_node->next_node;
        cur_node->next_node = node->next_node;

        free(node->data);
        free(node);
    }

    free(cur_node->data);
    free(cur_node);
}

int ListPush(void* list_head, void* data, int data_size) {
    if (list_head == NULL || data == NULL) {
        return -1;
    }

    ListNode* new_node = (ListNode*)malloc(sizeof(ListNode));
    if (new_node == NULL) {
        return -1;
    }

    memset(new_node, 0, sizeof(ListNode));
    new_node->data = malloc(data_size);
    if (new_node->data == NULL) {
        free(new_node);
        return -1;
    }

    memcpy(new_node->data, data, data_size);
    new_node->data_size = data_size;

    ListNode* cur_node = (ListNode*) list_head;
    while (cur_node->next_node != NULL)
    {
        cur_node = cur_node->next_node;
    }
    
    cur_node->next_node = new_node;
    return 0;
}

int ListPop(void* list_head, void* data, int data_size) {
    if (list_head == NULL || ((ListNode*)list_head)->next_node == NULL || data == NULL) {
        return -1;
    }

    ListNode* pre_node = NULL;
    ListNode* cur_node = (ListNode*) list_head;
    while (cur_node->next_node != NULL)
    {
        pre_node = cur_node;
        cur_node = cur_node->next_node;
    }

    if (cur_node->data_size > data_size) {
        return -1;
    }
    
    memcpy(data, cur_node->data, cur_node->data_size);

    free(cur_node->data);
    free(cur_node);
    pre_node->next_node = NULL;

    return 0;
}

int ListSize(void* list_head) {
    if (list_head == NULL) {
        return -1;
    }

    int cnt = 0;
    ListNode* cur_node = (ListNode*) list_head;
    while (cur_node->next_node != NULL)
    {
        cnt++;
        cur_node = cur_node->next_node;
    }

    return cnt;
}

void* ListGet(void* list_head, int index) {
    if (list_head == NULL || index > ListSize(list_head)) {
        return NULL;
    }

    ListNode* cur_node = ((ListNode*)list_head)->next_node;
    while (index--)
    {
        cur_node = cur_node->next_node;
    }
    
    return cur_node->data;
}