/* vm.c: 가상 메모리 객체를 위한 일반 인터페이스. */

#include <stdio.h>
#include <string.h>
#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "hash.h"
#include "threads/mmu.h"
#include "user/syscall.h"
#include "threads/thread.h"
#include "threads/synch.h" // lock_init등을 위해 추가



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
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
// 초기화 함수를 사용하여 보류 중인 페이지 객체를 생성합니다. 
// 페이지를 생성하려면 직접 생성하지 말고 이 함수나 `vm_alloc_page`를 통해 생성하세요.
// 페이지를 예약하는 함수, 실제 메모리는 할당하지 않고, 나중에 필요할 때 어떻게 만들지 계획만 세워둠.
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
                                vm_initializer *init, void *aux) {

    ASSERT (VM_TYPE(type) != VM_UNINIT);

    struct supplemental_page_table *spt = &thread_current()->spt;

    if (spt_find_page (spt, upage) == NULL) {
        struct page *p = malloc(sizeof(struct page));
        if (p == NULL) {
            return false;
        }
//        printf("얼록 페이지\n");

        bool (*page_initializer)(struct page *, enum vm_type, void *kva);

        /* VM 타입에 따라 올바른 page_initializer를 직접 전달합니다. */
        switch (VM_TYPE(type)) {
            case VM_ANON:
                page_initializer = anon_initializer;
                break;
            case VM_FILE:
                page_initializer = file_backed_initializer;
                break;
            default:
                /* 다른 타입이 있다면 여기에 추가, 없다면 실패 처리 */
                goto err; // 이 switch에 해당하지 않는 타입은 오류
        }

        uninit_new(p, upage, init, type, aux, page_initializer);
        p->writable = writable;

        if (spt_insert_page(spt, p)) {
            return true;
        }
        /* spt_insert_page 실패 시 */
        err:
        free(p); // 페이지 생성 실패 시 할당한 메모리 해제
        return false;
    }
    return false; // 이미 페이지가 존재하는 경우
}

/* Find VA from spt and return page. On error, return NULL. */
// spt에서 VA를 찾아 페이지를 반환합니다. 오류가 발생하면 NULL을 반환합니다.
struct page *
spt_find_page (struct supplemental_page_table *spt, void *va) {
	/* TODO: Fill this function. */
    struct page *page_find;
    struct page dummy_page;
    struct hash_elem *elem_find;

    if (spt == NULL || va == NULL){
        return NULL;
    }

    dummy_page.va = pg_round_down(va);

    elem_find = hash_find(&spt->spt_hash, &dummy_page.hash_elem);

    if(elem_find != NULL){
        return hash_entry(elem_find, struct page, hash_elem);
    }else{
        return NULL;
    }
}

/* Insert PAGE into spt with validation. */
// 검증을 통해 spt에 PAGE를 삽입합니다.
bool
spt_insert_page (struct supplemental_page_table *spt,
		struct page *page) {
	/* TODO: Fill this function. */
    struct hash_elem *insert_result;

    if (spt_find_page(spt, page->va) != NULL){
        return false;
    }
    insert_result = hash_insert(&spt->spt_hash, &page->hash_elem);

    if (insert_result == NULL){
        return true;
    }else{
        return false;
    }
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
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
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
    void *kva = palloc_get_page(PAL_USER);

    if (kva == NULL){
        PANIC("todo");
    }

    frame = calloc(1, sizeof(struct frame));
    frame -> kva = kva;

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
// 스택을 키웁니다.
static void
vm_stack_growth (void *addr UNUSED) {
    if(vm_alloc_page(VM_ANON | VM_MARKER_0, addr, true)){
        if (vm_claim_page(addr)){
            return;
        }
    }
    PANIC("나 진짜 너무 많은 일이 잇엇어 힘들다진짜");
}

/* Handle the fault on write_protected page */
// write_protected 페이지에서 오류를 처리합니다.
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
// 성공 시 true를 반환합니다.
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
    struct supplemental_page_table *spt = &thread_current()->spt;
    struct page *page = NULL;

    // 주소 유효성 검사
    if (is_kernel_vaddr(addr) || addr == NULL) {
        thread_exit();
    }

    page = spt_find_page(spt, pg_round_down(addr));

    if (page == NULL) {
        // user 플래그를 사용한 더 안전하고 간단한 로직
        if (user) { // 사용자 모드에서 발생한 폴트일 때만 스택 성장 검사
            void *rsp = f->rsp;
            bool is_stack_growth = (addr >= rsp - 8) &&
                                   (addr < (void *)USER_STACK) &&
                                   (addr >= (void *)USER_STACK - (1 << 20));

            if (is_stack_growth) {
                // 스택 성장 처리
                vm_stack_growth(pg_round_down(addr));
                // 성공적으로 처리했으니, 다시 페이지를 찾아 claim 시도
                return vm_do_claim_page(spt_find_page(spt, pg_round_down(addr)));
            }
        }
        // 사용자 모드 폴트가 아니거나, 스택 성장 조건이 아니면 진짜 오류
        thread_exit();
    }

    // 페이지를 찾았을 때의 처리 로직
    if (write && !page->writable) {
        thread_exit();
    }

    return vm_do_claim_page(page);
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

/* Claim the PAGE and set up the mmu. */
// PAGE를 선언하고 mmu를 설정하세요.
static bool
vm_do_claim_page (struct page *page) {
    struct frame *frame = vm_get_frame ();

    /* Set links */
    frame->page = page;
    page->frame = frame;

    /* TODO: Insert page table entry to map page's VA to frame's PA. */
    // 페이지의 VA를 프레임의 PA에 매핑하기 위해 페이지 테이블 항목을 삽입합니다.
    pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable);
    //페이지를 프레임과 연결 시키고, 읽기/쓰기 규칙 설정
    return swap_in (page, frame->kva);
}


/* Claim the page that allocate on VA. */
// VA로 할당된 페이지를 선언합니다.
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function */
	// 기능을 구현하세요.
//    printf("함수 호출 확인EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE: %p\n", va);

    page = spt_find_page(&thread_current()->spt, va);

    if (page == NULL){

//        printf("SPT에서 페이지 찾았나 확인EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE\n");
        // 트러블 슈팅: 여기서 page == NULL이 참이 되었음, spt_find_page에서 페이지를 못찾았다는 뜻, 그럼 setup_stack에서 vm_alloc_page가 문제일수 있음.
        return false;
    }

    if(page->frame != NULL){
//        printf("이미 프레임이 있나 확인EEEEEEEEEEEEEEEEEEEEEEEEEEEEEE\n");
        return true;
    }

//    printf("성공했나?");

    return vm_do_claim_page (page);
}


uint64_t haha_func(const struct hash_elem *ha, void *aux UNUSED){
    const struct page *p = hash_entry(ha, struct page, hash_elem);
    return hash_bytes(&p->va, sizeof p->va);
}

bool hale_func(const struct hash_elem *ha_1,const struct hash_elem *ha_2, void *aux UNUSED){
    const struct page *page_1 = hash_entry(ha_1, struct page, hash_elem);
    const struct page *page_2 = hash_entry(ha_2, struct page, hash_elem);

    return page_1->va < page_2->va;
}

/* Initialize new supplemental page table */
// 새로운 보충 페이지 테이블을 초기화합니다.
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
    hash_init (&spt->spt_hash, haha_func, hale_func, NULL);
}

/* Copy supplemental page table from src to dst */
// src에서 dst로 보충 페이지 테이블 복사합니다.
// 자식 프로세스를 만들 때 부모의 spt도 자식에게 복사해줘야 함.
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED, struct supplemental_page_table *src UNUSED){

    struct hash_iterator i;
    hash_first(&i, &src->spt_hash);
    while(hash_next(&i)){
        struct page *parent_page = hash_entry(hash_cur(&i), struct page, hash_elem);

        //1. 부모 페이지 타입에 따라 자식 페이지를 할당함.
        if (parent_page->operations->type == VM_UNINIT){
            vm_initializer *init = parent_page->uninit.init;
            void *aux = parent_page->uninit.aux;
            if(!vm_alloc_page_with_initializer(page_get_type(parent_page), parent_page->va, parent_page->writable,init, aux))
            {
                return false;
            }
        }else{
            if(!vm_alloc_page(page_get_type(parent_page), parent_page->va,parent_page->writable)){
                return false;
            }

            if (!vm_claim_page(parent_page->va)){
                return false;
            }

            //2. 방금 생성된 자식 페이지를 찾아서, 부모의 프레임 내용을 복사합니다.
            struct page *child_page = spt_find_page(dst, parent_page->va);
            if(child_page == NULL){
                return false;
            }
            memcpy(child_page->frame->kva, parent_page->frame->kva, PGSIZE);
        }
    }
    return true;
}

static void
kill_page(struct hash_elem *e, void *aux UNUSED) {
    struct page *page = hash_entry(e, struct page, hash_elem);
    destroy(page); // 페이지 내용물(프레임 등) 자원 해제
}


/* Free the resource hold by the supplemental page table */
// 보충 페이지 테이블에서 리소스 보류를 해제합니다.
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	// 스레드가 보유한 supplemental_page_table을 모두 파괴하고 수정된 내용을 모두 저장소에 다시 쓰게 구현하세요.
    hash_clear(&spt->spt_hash, kill_page);

}
