#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include "lib/kernel/list.h"
#include "userprog/pagedir.h"
#include "vm/page.h"
#include "threads/pte.h"
#include "vm/swap.h"

#include "vm/frame.h"

/* Lock to synchronize access to the frame table. */
static struct lock vm_lock;

/* Lock to ensure eviction operations are atomic. */
static struct lock eviction_lock;

/* Frame table operations. */
static bool add_vm_frame(void *);
static void remove_vm_frame(void *);
/* Retrieve the vm_frame struct corresponding to the given frame address. */
static struct vm_frame *get_vm_frame(void *);

/* Functions needed for eviction. */
static struct vm_frame *frame_to_evict(void); // Select a frame for eviction.
/* Save the evicted frame's content to swap space. */
static bool save_evicted_frame(struct vm_frame *);

/* Initialize the frame table and related data structures. */
void
vm_frame_init()
{
  list_init(&vm_frames);
  lock_init(&vm_lock);
  lock_init(&eviction_lock);
}

/* Allocate a frame from the user pool and add it to the frame table. */
void *
vm_allocate_frame(enum palloc_flags flags)
{
  void *frame = NULL;

  /* Try to allocate a page from the user pool. */
  if (flags & PAL_USER)
    {
      if (flags & PAL_ZERO)
        frame = palloc_get_page(PAL_USER | PAL_ZERO);
      else
        frame = palloc_get_page(PAL_USER);
    }

  /* If successful, add to the frame table. Otherwise, evict a frame to free space. */
  if (frame != NULL)
    add_vm_frame(frame);
  else
    if ((frame = evict_frame()) == NULL)
      PANIC("Eviction failed while trying to allocate a frame.");

  return frame;
}

/* Free a frame and remove its entry from the frame table. */
void
vm_free_frame(void *frame)
{
  /* Remove the frame table entry. */
  remove_vm_frame(frame);
  /* Free the frame. */
  palloc_free_page(frame);
}

/* Set the user page attributes (PTE and virtual address) for a given frame. */
void
vm_frame_set_usr(void *frame, uint32_t *pte, void *upage)
{
  struct vm_frame *vf;
  vf = get_vm_frame(frame);
  if (vf != NULL)
    {
      vf->pte = pte;
      vf->uva = upage;
    }
}

/* Evict a frame and prepare its content for swapping. */
void *
evict_frame()
{
  bool result;
  struct vm_frame *vf;
  struct thread *t = thread_current();

  lock_acquire(&eviction_lock);

  vf = frame_to_evict();
  if (vf == NULL)
    PANIC("No frame available for eviction.");

  result = save_evicted_frame(vf);
  if (!result)
    PANIC("Failed to save evicted frame to swap space.");
  
  /* Reset frame metadata. */
  vf->tid = t->tid;
  vf->pte = NULL;
  vf->uva = NULL;

  lock_release(&eviction_lock);

  return vf->frame;
}

/* Select a frame to evict using the clock algorithm. */
static struct vm_frame *
frame_to_evict()
{
  struct vm_frame *vf;
  struct thread *t;
  struct list_elem *e;

  struct vm_frame *vf_class0 = NULL;

  int round_count = 1;
  bool found = false;

  /* Iterate through the frame table to find a candidate for eviction. */
  while (!found)
    {
      e = list_head(&vm_frames);
      while ((e = list_next(e)) != list_tail(&vm_frames))
        {
          vf = list_entry(e, struct vm_frame, elem);
          t = thread_get_by_id(vf->tid);
          bool accessed = pagedir_is_accessed(t->pagedir, vf->uva);
          if (!accessed)
            {
              vf_class0 = vf;
              list_remove(e);
              list_push_back(&vm_frames, e);
              break;
            }
          else
            {
              pagedir_set_accessed(t->pagedir, vf->uva, false);
            }
        }

      if (vf_class0 != NULL || round_count++ == 2)
        found = true;
    }

  return vf_class0;
}

/* Save the content of the evicted frame to swap space. */
static bool
save_evicted_frame(struct vm_frame *vf)
{
  struct thread *t;
  struct suppl_pte *spte;

  /* Get the thread and its supplemental page table. */
  t = thread_get_by_id(vf->tid);

  /* Retrieve or create a supplemental page table entry for the frame's virtual address. */
  spte = get_suppl_pte(&t->suppl_page_table, vf->uva);
  if (spte == NULL)
    {
      spte = calloc(1, sizeof *spte);
      spte->uvaddr = vf->uva;
      spte->type = SWAP;
      if (!insert_suppl_pte(&t->suppl_page_table, spte))
        return false;
    }

  size_t swap_slot_idx;

  /* If the page is dirty or not a file, save it to swap space. */
  if (pagedir_is_dirty(t->pagedir, spte->uvaddr) || (spte->type != FILE))
    {
      swap_slot_idx = page_to_swap(spte->uvaddr);
      if (swap_slot_idx == SWAP_ERROR)
        return false;

      spte->type |= SWAP;
    }

  memset(vf->frame, 0, PGSIZE);

  spte->swap_slot_idx = swap_slot_idx;
  spte->swap_writable = *(vf->pte) & PTE_W;
  spte->is_loaded = false;

  /* Clear the page mapping from the page directory. */
  pagedir_clear_page(t->pagedir, spte->uvaddr);

  return true;
}

/* Add a frame to the frame table. */
static bool
add_vm_frame(void *frame)
{
  struct vm_frame *vf;
  vf = calloc(1, sizeof *vf);

  if (vf == NULL)
    return false;

  vf->tid = thread_current()->tid;
  vf->frame = frame;
  
  lock_acquire(&vm_lock);
  list_push_back(&vm_frames, &vf->elem);
  lock_release(&vm_lock);

  return true;
}

/* Remove a frame from the frame table. */
static void
remove_vm_frame(void *frame)
{
  struct vm_frame *vf;
  struct list_elem *e;
  
  lock_acquire(&vm_lock);
  e = list_head(&vm_frames);
  while ((e = list_next(e)) != list_tail(&vm_frames))
    {
      vf = list_entry(e, struct vm_frame, elem);
      if (vf->frame == frame)
        {
          list_remove(e);
          free(vf);
          break;
        }
    }
  lock_release(&vm_lock);
}

/* Retrieve the vm_frame corresponding to a given frame address. */
static struct vm_frame *
get_vm_frame(void *frame)
{
  struct vm_frame *vf;
  struct list_elem *e;
  
  lock_acquire(&vm_lock);
  e = list_head(&vm_frames);
  while ((e = list_next(e)) != list_tail(&vm_frames))
    {
      vf = list_entry(e, struct vm_frame, elem);
      if (vf->frame == frame)
        break;
      vf = NULL;
    }
  lock_release(&vm_lock);

  return vf;
}
