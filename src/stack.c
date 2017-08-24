#include <string.h>
#include <stdlib.h>

#include "stack.h"
#include "thread.h"

ZzStack *ZzGetCurrentThreadStack(zpointer key_ptr) {
    ZzStack *stack  = (ZzStack *)ZzGetCurrentThreadStack(key_ptr);
    if(!stack)
		return NULL;
    return stack;
}

ZzStack *ZzNewStack(zpointer key_ptr) {
    ZzStack *stack;
    stack = (ZzStack *)malloc(sizeof(ZzStack));
    stack->capacity = 4;
    ZzCallStack **callstacks = (ZzCallStack **)malloc(sizeof(ZzCallStack *) * (stack->capacity));
    if(!callstacks)
        return NULL;
    stack->callstacks = callstacks;
	stack->size = 0;
	stack->key_ptr = key_ptr;
	ZzThreadSetCurrentThreadData(key_ptr, (zpointer)stack);
    return stack;
}

ZzCallStack *ZzNewCallStack(ZzStack *stack) {
    ZzCallStack *callstack;
    callstack = (ZzCallStack *)malloc(sizeof(ZzCallStack));
	callstack->capacity = 4;
	
	callstack->items = (ZzCallStackItem *)malloc(sizeof(ZzCallStackItem) * callstack->capacity);
	if(!callstack->items)
		return NULL;

    return callstack;
}

ZzCallStack *ZzPopCallStack(ZzStack *stack) {
	if(stack->size > 0)
		stack->size--;
	else
		return NULL;
	ZzCallStack *callstack = stack->callstacks[stack->size];
	return callstack;
}

bool ZzPushCallStack(ZzStack *stack, ZzCallStack *callstack) {
	if(!stack)
		return false;

	if (stack->size >= stack->capacity)
	{
		ZzCallStack **callstacks = (ZzCallStack **)realloc(stack->callstacks, sizeof(ZzCallStack *) * (stack->capacity) * 2);
		if(!callstacks)
			return false;
		stack->callstacks = callstacks;
		stack->capacity = stack->capacity * 2;
	}

	stack->callstacks[stack->size++] = callstack;
	return true;
}

zpointer ZzGetCallStackData(zpointer callstack_ptr, char *key) {
	ZzCallStack *callstack = (ZzCallStack *)callstack_ptr;
	if(!callstack)
		return NULL;
	for (int i = 0; i < callstack->size; ++i)
	{
		if (!strcmp(callstack->items[i].key, key))
		{
			return callstack->items[i].value;
		}
	}
	return NULL;
}
ZzCallStackItem *ZzNewCallStackData(ZzCallStack *callstack) {
	if(!callstack)
		return NULL;
	if(callstack->size >= callstack->capacity) {
		ZzCallStackItem *callstackitems = (ZzCallStackItem *)realloc(callstack->items, sizeof(ZzCallStackItem) * callstack->capacity * 2);
		if(!callstackitems)
			return NULL;
		callstack->items = callstackitems;
		callstack->capacity = callstack->capacity * 2;
	}
	return &(callstack->items[callstack->size++]);
}

bool ZzSetCallStackData(zpointer callstack_ptr, char *key, zpointer value_ptr, zsize value_size) {
	ZzCallStack *callstack = (ZzCallStack *)callstack_ptr;
	if(!callstack)
		return false;

	ZzCallStackItem *item = ZzNewCallStackData(callstack);

	char *key_tmp = (char *)malloc(strlen(key) + 1);
    strncpy(key_tmp, key, strlen(key) + 1);
    
	zpointer value_tmp = (zpointer)malloc(value_size);
	memcpy(value_tmp, value_ptr, value_size);
	item->key = key_tmp;
	item->value = value_tmp;
	return true;
}

