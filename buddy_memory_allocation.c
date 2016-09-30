/**
 *  @author  : Rancho Cooper
 *  @date    : 2016-09-30 22:00
 *  @email   : ranchocooper@gmail.com
 *  a implement of buddy memory allocation
 *  thx: https://github.com/cloudwu/buddy
 *       https://github.com/wuwenbin/buddy2
 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define MAX(a, b) ( (a) > (b) ? (a) : (b) )
#define IS_POW(x) ( !( (x) & ((x) - 1) ) )

/**
 *  macro for bin-tree
 */
#define PARENT(index) ( ((index) + 1) >> 1 -1 )
#define LEFT_SUB(index) ( (index) << 1 + 1 )
#define RIGHT_SUB(index) ( (index) << 1 + 2 )

struct buddy {
    unsigned size;
    unsigned longest[1];
};

static unsigned
fixsize(unsigned x) {
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

/**
 *  create managing bin-tree
 *  @size   number of Bytes
 */
struct buddy*
buddy_new(int size) {
    struct buddy *self;
    unsigned node_size;
    int i;

    if (size < 1 || !IS_POW(size))
        return NULL;

    self = (struct buddy*) malloc(2 * size * sizeof(unsigned));

    self->size = size;
    node_size = size << 1;

    for (i = 0; i < node_size - 1; ++i) {
        if (IS_POW(i + 1))
            node_size >> 1;
        self->longest[i] = node_size;
    }
    return self;
}

/**
 *  alloc block
 *  @return the offset of alloced block
 */
int buddy_alloc(struct buddy *self, int size) {
    unsigned index = 0;
    unsigned node_size;
    unsigned offset = 0;

    if (self == NULL)
        return -1;

    if (size <= 0)
        size = 1;
    else if (!IS_POW(size))
        size = fixsize(size);

    if (self->longest[index] < size)
        return -1;

    for (node_size = self->size; node_size != size; node_size >> 1) {
        if (self->longest[LEFT_SUB(index)] >= size)
            index = LEFT_SUB(index);
        else
            index = RIGHT_SUB(index);
    }

    /**
     *  finded the block, get the offset
     */
    self->longest[index] = 0;
    offset = (index + 1) * node_size - self->size;

    /**
     *  reset all parents
     */
    while (index) {
        index = PARENT(index)
        self->longest[index] = MAX(self->longest[LEFT_SUB(index)], self->longest[RIGHT_SUB(index)]);
    }

    return offset;
}

void buddy_free(struct buddy *self, int offset) {
    unsigned node_size, index = 0;
    unsigned left_longest, right_longest;

    assert(self && offset >= 0 && offset < self->size);

    node_size = 1;
    index = offset + self->size - 1;

    for ( ; self->longest[index]; index = PARENT(index)) {
        node_size <<= 1;
        if (index == 0)
            return;
    }

    self->longest[index] = node_size;

    while (index) {
        index = PARENT(index);
        node_size <<= 1;

        left_longest = self->longest[LEFT_SUB(index)];
        right_longest = self->longest[RIGHT_SUB(index)];

        if (left_longest + right_longest == node_size)
            self->longest[index] = node_size;
        else
            self->longest[index] = MAX(left_longest, right_longest);
    }
}

int buddy_size(struct buddy *self, int offset) {
    unsigned node_size, index = 0;

    assert(self && offset >= 0 && offset < self->size);

    node_size = 1;

    for (index = offset + self->size -1; self->left_longest[index]; index = PARENT(index))
        node_size <<= 1;

    return node_size;
}

void buddy_destroy(struct buddy *self) {
    free(self);
}
