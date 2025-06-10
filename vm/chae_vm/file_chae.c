/* file.c: Implementation of memory backed file object (mmaped object). */
// file.c: 메모리 백업 파일 객체(mmaped 객체)의 구현입니다.

#include "vm/vm.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
// ※ 수정하지 마세요. ※
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
// 파일 vm의 초기화 프로그램
void
vm_file_init (void) {
}

/* Initialize the file backed page */
// 파일 백업 페이지 초기화합니다.
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;

    return true;
}

/* Swap in the page by read contents from the file. */
// 파일에서 내용을 읽어 페이지를 교체합니다.
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
// 파일에 쓰기백 내용으로 페이지를 교체합니다.
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
// 파일 백 페이지를 파괴합니다. 호출자가 PAGE를 해제합니다.
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
// mmap 관련 기능입니다.
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
}

/* Do the munmap */
// munamp 관련 기능입니다.
void
do_munmap (void *addr) {
}
