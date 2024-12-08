#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

//FUNCTION DECLARATIONS

static void syscall_handler(struct intr_frame *);
static void sys_halt(void);
static void sys_exit(int);
static int sys_open(const char *);
static int sys_write(int, const void *, unsigned);
static void sys_close(int);

static struct file_descriptor *get_open_file(int);
static void close_open_file(int);
static bool is_valid_uvaddr(const void *);
static int allocate_fd(void);

//

void syscall_init(void)
{
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
  list_init(&open_files);
  lock_init(&fs_lock);
}

static void
syscall_handler(struct intr_frame *f)
{
  static uint32_t *esp;
  esp = f->esp;

  if (!is_valid_ptr(esp) || !is_valid_ptr(esp + 1) ||
      !is_valid_ptr(esp + 2) || !is_valid_ptr(esp + 3))
  {
    exit(-1);
  }
  else
  {
    int syscall_number = *esp;
    switch (syscall_number)
    {
    case SYS_HALT:
      sys_halt();
      break;
    case SYS_EXIT:
      sys_exit(*(esp + 1));
      break;
    case SYS_OPEN:
      f->eax = sys_open((char *)*(esp + 1));
      break;
    case SYS_CLOSE:
      sys_close(*(esp + 1));
      break;
    case SYS_WRITE:
      f->eax = sys_write(*(esp + 1), (void *)*(esp + 2), *(esp + 3));
      break;
    default:
      break;
    }
  }
}

void sys_halt(void)
{
  shutdown_power_off();
}

void sys_exit(int status)
{
  struct child_status *child;
  struct thread *cur = thread_current();
  printf("%s: exit(%d)\n", cur->name, status);
  thread_exit();
}

int sys_open(const char *file_name)
{
  struct file *f;
  struct file_descriptor *fd;
  int status = -1;

  if (!is_valid_ptr(file_name))
    exit(-1);

  lock_acquire(&fs_lock);

  f = filesys_open(file_name);
  if (f != NULL)
  {
    fd = calloc(1, sizeof *fd);
    fd->fd_num = allocate_fd();
    fd->owner = thread_current()->tid;
    fd->file_struct = f;
    list_push_back(&open_files, &fd->elem);
    status = fd->fd_num;
  }
  lock_release(&fs_lock);
  return status;
}

void sys_close(int fd)
{
  struct file_descriptor *fd_struct;
  lock_acquire(&fs_lock);
  fd_struct = get_open_file(fd);
  if (fd_struct != NULL && fd_struct->owner == thread_current()->tid)
    close_open_file(fd);
  lock_release(&fs_lock);
  return;
}

int sys_write(int fd, const void *buffer, unsigned size)
{
  struct file_descriptor *fd_struct;
  int status = 0;

  unsigned buffer_size = size;
  void *buffer_tmp = buffer;

  /* check the user memory pointing by buffer are valid */
  while (buffer_tmp != NULL)
  {
    if (!is_valid_ptr(buffer_tmp))
      exit(-1);

    /* Advance */
    if (buffer_size > PGSIZE)
    {
      buffer_tmp += PGSIZE;
      buffer_size -= PGSIZE;
    }
    else if (buffer_size == 0)
    {
      /* terminate the checking loop */
      buffer_tmp = NULL;
    }
    else
    {
      /* last loop */
      buffer_tmp = buffer + size - 1;
      buffer_size = 0;
    }
  }
  lock_acquire(&fs_lock);
  if (fd == STDIN_FILENO)
  {
    status = -1;
  }
  else if (fd == STDOUT_FILENO)
  {
    putbuf(buffer, size);
    status = size;
  }
  else
  {
    fd_struct = get_open_file(fd);
    if (fd_struct != NULL)
      status = file_write(fd_struct->file_struct, buffer, size);
  }
  lock_release(&fs_lock);

  return status;
}

bool
create (const char *file_name, unsigned size)
{
  bool status;

  if (!is_valid_ptr (file_name))
    exit (-1);

  lock_acquire (&fs_lock);
  status = filesys_create(file_name, size);  
  lock_release (&fs_lock);
  return status;
}

//HELPERS

bool is_valid_ptr(const void *usr_ptr)
{
  struct thread *cur = thread_current();
  if (is_valid_uvaddr(usr_ptr))
  {
    return (pagedir_get_page(cur->pagedir, usr_ptr)) != NULL;
  }
  return false;
}

static bool
is_valid_uvaddr(const void *uvaddr)
{
  return (uvaddr != NULL && is_user_vaddr(uvaddr));
}

int allocate_fd()
{
  static int fd_current = 1;
  return ++fd_current;
}

void
close_open_file (int fd)
{
  struct list_elem *e;
  struct list_elem *prev;
  struct file_descriptor *fd_struct; 
  e = list_end (&open_files);
  while (e != list_head (&open_files)) 
    {
      prev = list_prev (e);
      fd_struct = list_entry (e, struct file_descriptor, elem);
      if (fd_struct->fd_num == fd)
	{
	  list_remove (e);
          file_close (fd_struct->file_struct);
	  free (fd_struct);
	  return ;
	}
      e = prev;
    }
  return ;
}
