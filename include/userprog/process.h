#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "filesys/off_t.h"
#include "threads/thread.h"

tid_t process_create_initd (const char *file_name);
tid_t process_fork (const char *name, struct intr_frame *if_);
int process_exec (void *f_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (struct thread *next);
/*load_segment()에서 넘겨 줄 aux 구조체*/
struct file_page_aux {
    struct file *file; /*읽어들일 실행 파일 핸들 */
    off_t ofs; /* 현재 페이지가 파일 내에서 시작할 오프셋 */
    uint32_t read_bytes; /* 이 페이지에서 file_read_at 할 바이트 수*/
    uint32_t zero_bytes; /* 이 페이지에서 0으로 채울 바이트 수 */
    uint8_t *upage;
    bool writable;
};

#endif /* userprog/process.h */
