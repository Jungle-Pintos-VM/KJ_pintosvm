/* vm.c: 가상 메모리 객체를 위한 일반 인터페이스. */
#include "threads/thread.h"
#include "threads/malloc.h"
#include "vm/vm.h"
#include "threads/mmu.h"
#include "threads/vaddr.h"
#include "vm/inspect.h"
#include "vm/uninit.h"
#include "lib/stdlib.h"
#include "threads/palloc.h"
#include "lib/kernel/hash.h"
#include "userprog/process.h"
#include <string.h>

//aaaaa
#define VM_TYPE_COUNT 3
#define STACK_HEURISTIC 32
#define MAX_STACK_SIZE (8*1024*1024)

struct list frame_table;
struct lock frame_table_lock;

/* 타입별 기본 페이지 초기화기 테이블 */
// /*static vm_initializer *page_initializer_table[VM_TYPE_COUNT] = {
// 	[VM_UNINIT] = NULL,                  /* 사용할 일이 없습니다 */
// 	[VM_ANON]   = anon_initializer,      /* 익명 페이지 초기화기 */
// 	[VM_FILE]   = file_backed_initializer/* 파일 기반 페이지 초기화기 */
// };




/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
// 각 하위 시스템의 초기화 코드를 호출하여 가상 메모리 하위 시스템을 초기화합니다.
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	// ※ 위의 라인은 수정하지마세요. ※
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
// 페이지 유형을 가져옵니다. 이 함수는 페이지가 초기화된 후 페이지의 유형을 알고 싶을 때 유용합니다. 
// 이 함수는 현재 완전히 구현되었습니다.
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
// 초기화 함수를 사용하여 보류 중인 페이지 객체를 생성합니다. 
// 페이지를 생성하려면 직접 생성하지 말고 이 함수나 `vm_alloc_page`를 통해 생성하세요.
// bool
// lazy_load_segment(struct page *page, enum vm_type type, void *aux)
// {
// 	struct file_page_aux *info = (struct file_page_aux *)aux;
// 	struct file *file = info->file;
// 	off_t ofs = info->ofs;
// 	size_t read_bytes = info->read_bytes;
// 	size_t zero_bytes = info->zero_bytes;
//
// 	if (read_bytes>0) {
// 		file_seek(file, ofs);
// 		if (file_read(file, page->frame->kva, read_bytes) != (int)read_bytes)
// 			return false;
// 	}
// 	if (zero_bytes > 0)
// 		memset(page->frame->kva + read_bytes, 0, zero_bytes);
//
// 	return true;
// 	/* TODO: Load the segment from the file */
// 	/* TODO: This called when the first page fault occurs on address VA. */
// 	/* TODO: VA is available when calling this function. */
//
// }


bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init , void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT);
	//
	// /*페이지 경계 맞추기*/
	// void *upage_ = pg_round_down(upage);

	// 현재 프로세스의 SPT 가져옴
	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	// 이미지가 이미 점유되어 있는지 확인하세요.
	bool(*page_initializer)(struct page*,enum vm_type,void *kva);
	struct page *p = malloc(sizeof(struct page));

	if (spt_find_page(spt, upage) == NULL) {

		if (p == NULL){
			return false;
		}

		if (VM_TYPE(type)) {
			if (VM_TYPE(type) == VM_ANON) {
				page_initializer = anon_initializer;
			}
			if (VM_TYPE(type) == VM_FILE) {
				page_initializer = file_backed_initializer;
			}
		}


	}
	/* TODO: Create the page, fetch the initialier according to the VM type,
	 * TODO: and then create "uninit" page struct by calling uninit_new. You
	 * TODO: should modify the field after calling the uninit_new. */
	// 해야될 것: 페이지를 생성하고, VM 유형에 따라 초기화 파일을 가져온 후, uninit_new를 호출하여 "uninit" 페이지 구조체를 생성합니다.
	// 해야될 것: uninit_new를 호출한 후 필드를 수정해야 합니다.

	//새로운 페이지 구조체 할당

	//vm_initializer * initializer = init != NULL ? init : page_initializer_table[VM_TYPE(type)];

	//uninit 페이지로 초기화
	uninit_new(p, upage, init, type, aux, page_initializer);
	/*init매개변수를 그대로 uninit_new의 마지막 인자로 넘겨서, 외부에서 지정한 초기화 함수를 등록함.*/
	p->writable = writable;
	/* TODO: Insert the page into the spt. */
	// 해당 페이지를 spt에 삽입합니다.

	if (!spt_insert_page(spt, p)) {
		return false;
	}
	return true;
}



/* Find VA from spt and return page. On error, return NULL. */
// spt에서 VA를 찾아 페이지를 반환합니다. 오류가 발생하면 NULL을 반환합니다.


/* Insert PAGE into spt with validation. */
// 검증을 통해 spt에 PAGE를 삽입합니다.
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	if (hash_find(&spt->spt_hash, &page->hash_elem)!=NULL)
		return false;

	hash_insert(&spt->spt_hash, &page->hash_elem);
	return true;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return;
}

/* Get the struct frame, that will be evicted. */
// 내보낼 구조체 프레임을 가져옵니다.
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */
	 // 내보내는 규칙은 본인이 정하세요.

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
// 한 페이지를 제거하고 해당 프레임을 반환합니다.
// 오류 발생 시 NULL을 반환합니다.
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
// palloc() 함수는 프레임을 가져옵니다. 사용 가능한 페이지가 없으면 해당 페이지를 제거하고 반환합니다.
// 이 함수는 항상 유효한 주소를 반환합니다. 
// 즉, 사용자 풀 메모리가 가득 차면 이 함수는 프레임을 제거하여 사용 가능한 메모리 공간을 가져옵니다.
static struct frame *
vm_get_frame (void) {
	/*커널 페이지 할당*/
	void *kva = palloc_get_page(PAL_USER);
	if (kva == NULL)
		PANIC("Out of memory");

	/*frame 구조체 생성 및 초기화*/
	struct frame *frame = calloc(1,sizeof(struct frame));

	frame->kva = kva;

	/*frame table에 삽입(락 필요)*/
	// lock_acquire(&frame_table_lock);
	// list_push_back(&frame_table, &frame->frame_elem);
	// lock_release(&frame_table_lock);

	return frame;
}

/* Growing the stack. */
// 스택을 키웁니다.
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
// write_protected 페이지에서 오류를 처리합니다.
static bool
vm_handle_wp (struct page *page UNUSED) {
	return false;
}

/* Return true on success */
// 성공 시 true를 반환합니다.
bool
vm_try_handle_fault (struct intr_frame *f UNUSED,
	void *addr UNUSED,
		bool user UNUSED,
		bool write UNUSED,
		bool not_present UNUSED) {

	void *fault_addr = addr;

	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	//1. 유효한 페이지 폴트 검증
	if (!not_present || fault_addr == NULL || !is_user_vaddr(fault_addr))
		return false;
	//2. 페이지 경계로 정렬
	void *page_addr = pg_round_down(fault_addr);

	//3. SPT에서 페이지 조회
	page = spt_find_page(spt, page_addr);
	if (page) {
		return vm_do_claim_page(page);
	}
	//4.사용자 스택 확장 여부 검사
	void *esp = (void*)f->rsp;
	if (((uintptr_t)USER_STACK - (uintptr_t)page_addr)<=MAX_STACK_SIZE
		&& (uintptr_t)fault_addr >= (uintptr_t)esp - STACK_HEURISTIC) {
		if (!vm_alloc_page (VM_ANON, page_addr, true))
			return false;
		page = spt_find_page(spt, page_addr);
		return page && vm_do_claim_page(page);
	}
	//5. 처리 불가
	return false;
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
// 페이지를 비웁니다. 
// ※ 해당 함수는 수정하지 마세요! ※
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
// VA로 할당된 페이지를 선언합니다.
bool
vm_claim_page (void *va UNUSED) {


	/*va로부터 supplemental page table 가져오기*/
	struct supplemental_page_table *spt = &thread_current()->spt;

	/*va가 속한 페이지를 lookup*/
	struct page *page = spt_find_page(spt,va);
	if (page == NULL)
		return false;

	/*이미 프레임이 할당된 페이지라면, 성공으로 간주*/
	if (page->frame != NULL)
		return true;

	/*실제 클레임 수행(프레임 할당 + 매핑 + 초기화)*/
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
// PAGE를 선언하고 mmu를 설정하세요.
bool
vm_do_claim_page (struct page *page) {
	ASSERT(page != NULL);

	/*프레임 할당*/
	struct frame *frame = vm_get_frame ();
	if (frame == NULL)
		return false;

	/*페이지 <-> 프레임 연결*/
	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	// 페이지의 VA를 프레임의 PA에 매핑하기 위해 페이지 테이블 항목을 삽입합니다.

	/*MMU 설정: VA -> PA 매핑*/
	// bool writable = page->writable;
	// if (!pml4_set_page(thread_current()->pml4,
	// 	page->va, frame->kva, writable)) {
	// 	/*매핑 실패 시 cleanup: frame_table에서 제거 + 메모리 해제*/
	// 	// lock_acquire(&frame_table_lock);
	// 	// list_remove(&frame->frame_elem);
	// 	// lock_release(&frame_table_lock);
	//
	// 	palloc_free_page(frame->kva); // 물리 메모리 반환
	// 	free(frame); // frame 구조체 반환
	// 	return false;
	// }

	/*lazy load*/
	// if (page->operations->type == VM_UNINIT) {
	// 	//return (page->operations->swap_in)(page, frame->kva);
	// 	vm_initializer *swapin_func = page->operations->swap_in;
	// 	return swapin_func(page, frame->kva);
	// }

	pml4_set_page(thread_current()->pml4,page->va,frame->kva,page->writable);

	return swap_in(page,frame->kva);
}

uint64_t
page_hash(const struct hash_elem *e, void *aux UNUSED) {
	struct page *p = hash_entry(e,struct page, hash_elem);
	return hash_bytes(&p->va, sizeof p->va);
}

bool
page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED) {
	const struct page *p1 = hash_entry(a,struct page, hash_elem);
	const struct page *p2 = hash_entry(b, struct page, hash_elem);
	return p1->va < p2->va;
}

/* Initialize new supplemental page table */
// 새로운 보충 페이지 테이블을 초기화합니다.
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt->spt_hash, page_hash, page_less,NULL);
}

/* Copy supplemental page table from src to dst */
// src에서 dst로 보충 페이지 테이블 복사합니다.
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* Free the resource hold by the supplemental page table */
// 보충 페이지 테이블에서 리소스 보류를 해제합니다.
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	// 스레드가 보유한 supplemental_page_table을 모두 파괴하고 수정된 내용을 모두 저장소에 다시 쓰게 구현하세요.
}




/*해시 테이블 초기화*/
/*void
spt_init(struct supplemental_page_table *spt) {
	hash_init(&spt->spt_hash, page_hash, page_less,NULL);
}
*/
/*SPT에 페이지 삽입*/
/*bool
spt_insert_page(struct supplemental_page_table *spt, struct page *p) {
	//hash_insert()자체ㅏ 같은 키(같은 p->va)가 있으면 삽입 실패로 처리해주기 때문에 별도의 덮어쓰기 방지 코드 필요 없음
	return hash_insert(&spt->spt_hash, &p->hash_elem) == NULL;
}*/

/*SPT에서 페이지 탐색*/
struct page
*spt_find_page(struct supplemental_page_table *spt, void *va) {
	void *rounded_va = pg_round_down(va);
	struct page temp;
	temp.va = rounded_va;
	struct hash_elem *e = hash_find(&spt->spt_hash, &temp.hash_elem);
	if (e == NULL) {
		return NULL;
	}
	else {
		struct page *p = hash_entry(e, struct page, hash_elem);
		return p;
	}
}






void
page_destroy(struct hash_elem *e, void *aux UNUSED) {
	struct page *p = hash_entry(e, struct page, hash_elem);

	if (p == NULL)
		return;

	if (p->frame != NULL) {
		palloc_free_page(p->frame->kva);
		free(p->frame);
	}

	if (p->operations && p->operations->destroy) {
		(p->operations->destroy)(p);

	}

	free(p);

}


/*SPT 최종 정리 함수*/
void
spt_kill(struct supplemental_page_table *spt) {
	hash_destroy(&spt->spt_hash, page_destroy);
}