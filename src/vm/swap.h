#ifndef VM_SWAP_H
#define VM_SWAP_H

#define SWAP_ERROR SIZE_MAX

/* Swap initialization */
void swap_to_pageit (void);

/* Swap a frame into a swap slot */
size_t page_to_swap (const void *);

/* Swap a frame out of a swap slot to mem page */
void swap_to_page(size_t, void *);

void swap_clean_slot (size_t);
#endif /* vm/swap.h */