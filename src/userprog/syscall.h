#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

struct file_descriptor
{
  int fd_num;
  tid_t owner;
  struct file *file_struct;
  struct list_elem elem;
};

struct lock fs_lock;
struct list open_files;

void syscall_init (void);

#endif /**< userprog/syscall.h */
