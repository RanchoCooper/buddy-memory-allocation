/**
 *  @author  : Rancho Cooper
 *  @date    : 2016-09-30 21:56
 *  @email   : ranchocooper@gmail.com
 */

#ifndef BUDDY_MEMORY_ALLOCATTION_H
#define BUDDY_MEMORY_ALLOCATTION_H

struct buddy;

struct buddy *buddy_new(int size);
void buddy_destroy(struct buddy *);
int buddy_alloc(struct buddy *, int size);
void buddy_free(struct buddy *, int offset);
int buddy_size(struct buddy *, int offset);
void buddy_dump(struct buddy *);

#endif
