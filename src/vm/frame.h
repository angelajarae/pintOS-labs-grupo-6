#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/thread.h"

/* Struct representing a frame in memory, associated with a thread, 
   a page table entry (PTE), and a user virtual address (UVA). */
struct vm_frame {
  void *frame;           /* Pointer to the physical frame. */
  tid_t tid;             /* Thread ID owning the frame. */
  uint32_t *pte;         /* Page table entry linked to the frame. */
  void *uva;             /* User virtual address associated with the frame. */
  struct list_elem elem; /* List element for the frame table. */
};

/* Global list to manage frames in the system. */
struct list vm_frames;

/* Initializes the frame table. */
void vm_frame_init (void);

/* Allocates a new frame with the specified flags. */
void *vm_allocate_frame (enum palloc_flags flags);

/* Frees the given frame and removes it from the frame table. */
void vm_free_frame (void *frame);

/* Links a frame to a user process's page table and virtual address. */
void vm_frame_set_usr (void *frame, uint32_t *pte, void *uva);

/* Selects a frame to evict, writes its content to swap or file if necessary, 
   and makes the frame available for reuse. */
void *evict_frame (void);

#endif /* vm/frame.h */
