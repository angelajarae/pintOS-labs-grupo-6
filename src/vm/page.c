#include "vm/page.h"
#include "threads/pte.h"
#include "vm/frame.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"
#include "filesys/file.h"
#include "string.h"
#include "userprog/syscall.h"
#include "vm/swap.h"

/* Helper functions for loading page types and cleaning up */
static bool load_page_file(struct suppl_pte *);
static bool load_page_swap(struct suppl_pte *);
static void free_suppl_pte(struct hash_elem *, void * UNUSED);

/* Initialize supplemental page table management (currently no-op). */
void vm_page_init(void) {
  return;
}

/* Hash function for supplemental page table. */
unsigned suppl_pt_hash(const struct hash_elem *he, void *aux UNUSED) {
  const struct suppl_pte *vspte = hash_entry(he, struct suppl_pte, elem);
  return hash_bytes(&vspte->uvaddr, sizeof vspte->uvaddr);
}

/* Comparison function for supplemental page table entries. */
bool suppl_pt_less(const struct hash_elem *hea, const struct hash_elem *heb, void *aux UNUSED) {
  const struct suppl_pte *vsptea = hash_entry(hea, struct suppl_pte, elem);
  const struct suppl_pte *vspteb = hash_entry(heb, struct suppl_pte, elem);
  return vsptea->uvaddr < vspteb->uvaddr;
}

/* Find a supplemental page table entry by virtual address. */
struct suppl_pte *get_suppl_pte(struct hash *ht, void *uvaddr) {
  struct suppl_pte spte = {.uvaddr = uvaddr};
  struct hash_elem *e = hash_find(ht, &spte.elem);
  return e != NULL ? hash_entry(e, struct suppl_pte, elem) : NULL;
}

/* Load the page described by the supplemental page table entry. */
bool load_page(struct suppl_pte *spte) {
  switch (spte->type) {
    case FILE:
      return load_page_file(spte);
    case SWAP:
      return load_page_swap(spte);
    default:
      return false;
  }
}

/* Load a file-backed page into memory. */
static bool load_page_file(struct suppl_pte *spte) {
  struct thread *cur = thread_current();

  file_seek(spte->data.file_page.file, spte->data.file_page.ofs);

  uint8_t *kpage = vm_allocate_frame(PAL_USER); // Allocate a frame.
  if (kpage == NULL) return false;

  if (file_read(spte->data.file_page.file, kpage, spte->data.file_page.read_bytes)
      != (int)spte->data.file_page.read_bytes) {
    vm_free_frame(kpage);
    return false;
  }

  memset(kpage + spte->data.file_page.read_bytes, 0, spte->data.file_page.zero_bytes);

  if (!pagedir_set_page(cur->pagedir, spte->uvaddr, kpage, spte->data.file_page.writable)) {
    vm_free_frame(kpage);
    return false;
  }

  spte->is_loaded = true;
  return true;
}

/* Load a swapped page into memory. */
static bool load_page_swap(struct suppl_pte *spte) {
  uint8_t *kpage = vm_allocate_frame(PAL_USER); // Allocate a frame.
  if (kpage == NULL) return false;

  if (!pagedir_set_page(thread_current()->pagedir, spte->uvaddr, kpage, spte->swap_writable)) {
    vm_free_frame(kpage);
    return false;
  }

  swap_to_page(spte->swap_slot_idx, spte->uvaddr); // Swap data into memory.

  if (spte->type == SWAP) {
    hash_delete(&thread_current()->suppl_page_table, &spte->elem); // Remove swap entry.
  } else if (spte->type == (FILE | SWAP)) {
    spte->type = FILE;
    spte->is_loaded = true;
  }

  return true;
}

/* Free all entries in a supplemental page table. */
static void free_suppl_pt(struct hash *suppl_pt) {
  hash_destroy(suppl_pt, free_suppl_pte);
}

/* Free a supplemental page table entry. */
void free_suppl_pte(struct hash_elem *e, void *aux UNUSED) {
  struct suppl_pte *spte = hash_entry(e, struct suppl_pte, elem);

  if (spte->type & SWAP) swap_clean_slot(spte->swap_slot_idx);

  free(spte);
}

/* Insert a new supplemental page table entry. */
bool insert_suppl_pte(struct hash *spt, struct suppl_pte *spte) {
  if (spte == NULL) return false;

  struct hash_elem *result = hash_insert(spt, &spte->elem);
  return result == NULL;
}

/* Add a file-backed entry to the supplemental page table. */
bool suppl_pt_insert_file(struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes, bool writable) {
  struct suppl_pte *spte = calloc(1, sizeof *spte);
  if (spte == NULL) return false;

  spte->uvaddr = upage;
  spte->type = FILE;
  spte->data.file_page = (struct {
    struct file *file;
    off_t ofs;
    uint32_t read_bytes;
    uint32_t zero_bytes;
    bool writable;
  }){file, ofs, read_bytes, zero_bytes, writable};
  spte->is_loaded = false;

  struct hash_elem *result = hash_insert(&thread_current()->suppl_page_table, &spte->elem);
  return result == NULL;
}

/* Grow the stack by adding a page. */
void grow_stack(void *uvaddr) {
  struct thread *t = thread_current();
  void *spage = vm_allocate_frame(PAL_USER | PAL_ZERO); // Allocate zeroed frame.

  if (spage == NULL) return;

  if (!pagedir_set_page(t->pagedir, pg_round_down(uvaddr), spage, true)) {
    vm_free_frame(spage);
  }
}








