#ifndef VM_PAGE_H
#define VM_PAGE_H

#define STACK_SIZE (8 * (1 << 20)) // Maximum stack size: 8 MB.

#include <stdio.h>
#include "threads/thread.h"
#include "threads/palloc.h"
#include "lib/kernel/hash.h"
#include "filesys/file.h"

/* Supplemental Page Table Management */

/* Page types:
   - SWAP: Stored in swap space.
   - FILE: File-backed.
   - MMF: Memory-mapped file. */
enum suppl_pte_type
{
  SWAP = 001,
  FILE = 002,
  MMF  = 004
};

/* An entry in the supplemental page table. */
struct suppl_pte
{
  void *uvaddr;             // Virtual address (unique key).
  enum suppl_pte_type type; // Page type.
  union suppl_pte_data data;// Type-specific data.
  bool is_loaded;           // Whether the page is loaded.

  /* Swap information */
  size_t swap_slot_idx;     // Swap slot index.
  bool swap_writable;       // Writable flag for swapped pages.

  struct hash_elem elem;    // Hash table element.
};

/* Stores page-specific data:
   - For file-backed pages, includes file, offsets, and size. */
union suppl_pte_data
{
  struct
  {
    struct file *file;       // File associated with the page.
    off_t ofs;               // File offset.
    uint32_t read_bytes;     // Bytes to read.
    uint32_t zero_bytes;     // Bytes to zero-fill.
    bool writable;           // Writable flag.
  } file_page;
};

/* Initialize supplemental page table management. */
void vm_page_init(void);

/* Hash table utilities for supplemental page table. */
unsigned suppl_pt_hash(const struct hash_elem *, void *);
bool suppl_pt_less(const struct hash_elem *, const struct hash_elem *, void *);

/* Insert a supplemental page table entry. */
bool insert_suppl_pte(struct hash *, struct suppl_pte *);

/* Add a file-backed page to the table. */
bool suppl_pt_insert_file(struct file *, off_t, uint8_t *, uint32_t, uint32_t, bool);

/* Retrieve a supplemental page table entry by address. */
struct suppl_pte *get_suppl_pte(struct hash *, void *);

/* Free all entries in a supplemental page table. */
void free_suppl_pt(struct hash *);

/* Load a page into memory. */
bool load_page(struct suppl_pte *);

/* Expand the stack by adding a page. */
void grow_stack(void *);

#endif /* vm/page.h */