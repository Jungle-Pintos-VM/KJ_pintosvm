/* file.c: Implementation of memory backed file object (mmaped object). */
// file.c: 메모리 백업 파일 객체(mmaped 객체)의 구현입니다.

#include "vm/vm.h"
#include "include/threads/vaddr.h"
#include "include/threads/mmu.h"

// 추가된 정의부
#define PGBITS  12                         /* Number of offset bits. */
#define PGSIZE  (1 << PGBITS)              /* Bytes in a page. */
#define ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP) * (STEP)) // mmap에서 사용

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

struct mmap_file {
	int mapid;
	struct file *file;
	void *addr;
	size_t length;
	int writable;
	struct list_elem elem;
	off_t ofs;
	uint32_t read_bytes;
	uint32_t zero_bytes;
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
	// struct file_page *file_page UNUSED = &page->file;
	// if ((page->mmaped_file != NULL) && pml4_is_dirty(thread_current()->pml4, page->va)) {
	// 	file_write_at(page->mmaped_file, page->frame->kva, page->file.read_bytes, page->file.ofs);
	// }

	// free(page->frame);	
	// if (page->mmaped_file != NULL) {
	// 	file_close(page->mmaped_file);
	// }

	// pml4_clear_page(page->owner->pml4, page->va);
}

// mmap에서 lazt페이지를 로드한다.
static bool
lazy_load_mmap (struct page *page, void *aux) {
	// printf("mmap lazy load\n");

	uint8_t* upage = page->va;
	uint8_t* kpage = page->frame->kva;
	
	struct mmap_file* mmap = (struct mmap_file*)aux;
	struct file* file = mmap -> file;
	off_t ofs = mmap -> ofs;
	size_t page_read_bytes = mmap -> read_bytes;
	size_t page_zero_bytes = mmap -> zero_bytes;

	ASSERT ((page_read_bytes + page_zero_bytes) % PGSIZE == 0);
	ASSERT (pg_ofs (upage) == 0);
	ASSERT (ofs % PGSIZE == 0);

	/* Do calculate how to fill this page.
		* We will read PAGE_READ_BYTES bytes from FILE
		* and zero the final PAGE_ZERO_BYTES bytes. */

	/* Load this page. */
	if (file_read_at (file, kpage, page_read_bytes, ofs) != (int) page_read_bytes) {
		return false;
	}
	memset (kpage + page_read_bytes, 0, page_zero_bytes);

	free(aux);
	return true;
}

/* Do the mmap */
// mmap 관련 기능입니다.
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
			off_t file_ofs;
			
	//printf("addr: %d, length: %d, writable:%d, file: %d, file_ofs: %d\n", addr, length, writable, file, file_ofs);
	void *backup_addr = addr;
	addr = pg_round_down(addr);
	off_t total_len = file_length(file);
	//printf("total_len: %d\n",total_len);
	uint32_t read_bytes = total_len - offset;
	uint32_t zero_bytes = ROUND_UP(read_bytes, PGSIZE) - read_bytes;
	off_t ofs = offset;
	uint8_t * upage = addr;

	while (read_bytes > 0 || zero_bytes > 0){
		
	size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
	size_t page_zero_bytes = PGSIZE - page_read_bytes;
	
	struct mmap_file *mmap = calloc(1, sizeof(struct mmap_file));
	mmap -> addr = addr;
	// mmap -> length = length;
	mmap -> writable = writable;
	mmap -> file = file;
	mmap -> ofs = ofs;
	mmap -> read_bytes = page_read_bytes;
	mmap -> zero_bytes = page_zero_bytes;

	if(!vm_alloc_page_with_initializer(VM_FILE, addr, writable, lazy_load_mmap, mmap)) {
		return false;
	}
	// vm_alloc_page()

	read_bytes -= page_read_bytes;
	zero_bytes -= page_zero_bytes;
	upage += PGSIZE;
	/* file_read를 하지 않기 때문에 수동으로 ofs을 이동시켜줘야 한다. */
	ofs += page_read_bytes;
	}

	return backup_addr;
}

/* Do the munmap */
// munamp 관련 기능입니다. map을 끝낸다. 수정된 내용이 있으면 파일에 반영
void
do_munmap (void *addr) {
	addr = pg_round_down(addr);
	struct page* munmap_page = spt_find_page(&thread_current()->spt, addr);
	if (munmap_page == NULL) {
		return;
	}

	struct list* mapped_pages = munmap_page->mmaped_list;
	if(mapped_pages == NULL) {
		return;
	}

	struct list_elem* mapped_page_elem;
	struct page* tmp_page;
	while (!list_empty(mapped_pages)) {
		mapped_page_elem = list_pop_front(mapped_pages);
		tmp_page = list_entry(mapped_page_elem, struct page, mmaped_elem);
		spt_remove_page(&thread_current()->spt, tmp_page);
	}
	free(mapped_pages);
}

// void
// do_munmap (void *addr) {
// 	struct page *page = NULL;

// 	int count = 0;
// 	while((page = spt_find_page(&thread_current()->spt, addr)) != NULL){
// 		struct container *aux = (struct container *)page->uninit.aux;
// 		if(pml4_is_dirty(thread_current()->pml4, page->va)){
// 				file_write_at(aux->file, page->va, aux->page_read_bytes, aux->ofs);
// 				pml4_set_dirty(thread_current()->pml4, page->va, 0);
// 		}
// 		count++;
// 		//spt_remove_page(thread_current()->spt, page);
// 		pml4_clear_page(thread_current()->pml4, addr); 
// 		addr += PGSIZE;
// 	}
// }

	// struct list_elem* mapped_page_elem;
	// struct page* tmp_page;
// 수정사항을 파일에 적고 종료를 해야한다. 수정 여부는 pml4_isdirty함수 사용.
	//pml4_is_dirty(mapped_pages, munmap);

	// while (!list_empty(mapped_pages)) {
	// 	mapped_page_elem = list_pop_front(mapped_pages);
	// 	tmp_page = list_entry(mapped_page_elem, struct page, mmaped_elem);
	// 	vm_dealloc_page(tmp_page);
	// }
	// free(mapped_pages);
