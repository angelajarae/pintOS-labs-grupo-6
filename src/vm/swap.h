#ifndef VM_SWAP_H
#define VM_SWAP_H

#define SWAP_ERROR SIZE_MAX

/* Block device that contains the swap */
struct block *swap_device;

/* Bitmap of swap slot availablities and corresponding lock */
static struct bitmap *swap_map;

/* Represents how many sectors are needed to store a page */
static size_t SECTORS_PER_PAGE = PGSIZE / BLOCK_SECTOR_SIZE;
static size_t swap_size_in_page (void);

/* Swap initialization */
void swap_to_pageit (void);

/* Swap a frame into a swap slot */
size_t page_to_swap (const void *);

/* Swap a frame out of a swap slot to mem page */
void swap_to_page(size_t, void *);

void swap_clean_slot (size_t);
#endif /* vm/swap.h */