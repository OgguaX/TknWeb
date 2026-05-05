#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

#include "tkn.h"

extern void tknLog(const char* message);
extern void *tknMalloc(size_t size);
extern void tknFree(void *ptr);
extern void tknAbort(void);

extern void *tknMemcpy(void *dest, const void *src, size_t n);
extern void *tknMemmove(void *dest, const void *src, size_t n);
extern void *tknMemset(void *ptr, int value, size_t n);
extern int tknMemcmp(const void *s1, const void *s2, size_t n);

#define TKN_CLAMP(x, min, max) \
    ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#define TKN_DEFAULT_COLLECTION_SIZE 8
#define TKN_DEFAULT_COLLECTION_POWER_OF_TWO 7
#define TKN_MIN_COLLECTION_SIZE 1

typedef struct
{
    uint32_t maxCount;
    uint32_t count;
    size_t dataSize;
    void *array;
} TknDynamicArray;

typedef struct TknListNode
{
    void *data;
    struct TknListNode *pNextNode;
} TknListNode;

typedef struct
{
    uint32_t powerOfTwo;
    uint32_t capacity;
    uint32_t count;
    size_t dataSize;
    TknListNode **nodePtrs;
} TknHashSet;

TknHashSet tknCreateHashSet(size_t dataSize);
void tknDestroyHashSet(TknHashSet tknHashSet);
bool tknAddToHashSet(TknHashSet *pTknHashSet, const void *pData);
bool tknContainsInHashSet(TknHashSet *pTknHashSet, const void *pData);
void tknRemoveFromHashSet(TknHashSet *pTknHashSet, const void *pData);
void tknClearHashSet(TknHashSet *pTknHashSet);

TknDynamicArray tknCreateDynamicArray(size_t dataSize, uint32_t maxCount);
void tknDestroyDynamicArray(TknDynamicArray tknDynamicArray);
void tknInsertIntoDynamicArray(TknDynamicArray *pTknDynamicArray, void *pData, uint32_t index);
void tknAddToDynamicArray(TknDynamicArray *pTknDynamicArray, void *pData);
void tknRemoveFromDynamicArray(TknDynamicArray *pTknDynamicArray, void *pData);
void tknRemoveAtIndexFromDynamicArray(TknDynamicArray *pTknDynamicArray, uint32_t index);
void tknClearDynamicArray(TknDynamicArray *pTknDynamicArray);
void *tknGetFromDynamicArray(TknDynamicArray *pTknDynamicArray, uint32_t index);
bool tknContainsInDynamicArray(TknDynamicArray *pTknDynamicArray, void *pData);
