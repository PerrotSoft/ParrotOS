#ifndef PARROT_VECTOR_H
#define PARROT_VECTOR_H

#include <stdint.h>
#include <stddef.h>

#ifdef __UEFI__
    #include <Library/UefiBootServicesTableLib.h>
    #define MEM_ALLOC(size) ({ void* p; gBS->AllocatePool(EfiLoaderData, size, (void**)&p) == 0 ? p : NULL; })
    #define MEM_FREE(ptr)   gBS->FreePool(ptr)
#else
    #include <stdlib.h>
    #define MEM_ALLOC(size) malloc(size)
    #define MEM_FREE(ptr)   free(ptr)
#endif

typedef struct {
    int32_t id;
    void* data;
} VectorItem;

struct Vector;

typedef void* (*VectorGet_t)(struct Vector* self, int32_t id);
typedef void* (*VectorIdx_t)(struct Vector* self, uint64_t index);
typedef void  (*VectorPush_t)(struct Vector* self, int32_t id, void* data);
typedef void  (*VectorRem_t) (struct Vector* self, int32_t id);
typedef void  (*VectorFree_t)(struct Vector* self);
typedef uint8_t (*VectorSetId_t)(struct Vector* self, int32_t id, void* new_data);

typedef struct Vector {
    VectorItem* items;
    uint64_t    size;
    uint64_t    capacity;

    VectorPush_t Push;
    VectorGet_t  GetById;
    VectorSetId_t  SetById;
    VectorIdx_t  GetAt;
    VectorRem_t  Remove;
    VectorFree_t Clear;
} Vector;

static uint8_t _vSetId(Vector* self, int32_t id, void* new_data) {
    for (uint64_t i = 0; i < self->size; i++) {
        if (self->items[i].id == id) {
            self->items[i].data = new_data;
            return 1;
        }
    }
    return 0; 
}
static void _vPush(Vector* self, int32_t id, void* data) {
    if (self->size >= self->capacity) {
        uint64_t new_cap = (self->capacity == 0) ? 4 : self->capacity * 2;
        VectorItem* new_items = (VectorItem*)MEM_ALLOC(new_cap * sizeof(VectorItem));
        if (!new_items) return;

        for (uint64_t i = 0; i < self->size; i++) {
            new_items[i] = self->items[i];
        }

        if (self->items) MEM_FREE(self->items);
        self->items = new_items;
        self->capacity = new_cap;
    }
    self->items[self->size].id = id;
    self->items[self->size].data = data;
    self->size++;
}

static void* _vGetId(Vector* self, int32_t id) {
    for (uint64_t i = 0; i < self->size; i++) {
        if (self->items[i].id == id) return self->items[i].data;
    }
    return NULL;
}

static void* _vGetIdx(Vector* self, uint64_t index) {
    return (index < self->size) ? self->items[index].data : NULL;
}

static void _vRem(Vector* self, int32_t id) {
    for (uint64_t i = 0; i < self->size; i++) {
        if (self->items[i].id == id) {
            for (uint64_t j = i; j < self->size - 1; j++) self->items[j] = self->items[j+1];
            self->size--;
            return;
        }
    }
}

static void _vFree(Vector* self) {
    if (self->items) MEM_FREE(self->items);
    self->items = NULL;
    self->size = 0;
    self->capacity = 0;
}
static inline void VectorInit(Vector* v, uint64_t initial_capacity) {
    v->size = 0;
    v->capacity = initial_capacity;
    v->items = (initial_capacity > 0) ? (VectorItem*)MEM_ALLOC(initial_capacity * sizeof(VectorItem)) : NULL;

    v->Push    = _vPush;
    v->GetById = _vGetId;
    v->GetAt   = _vGetIdx;
    v->Remove  = _vRem;
    v->Clear   = _vFree;
    v->SetById = _vSetId;
}

#endif