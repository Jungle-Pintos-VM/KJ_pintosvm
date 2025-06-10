/* vm.c: 가상 메모리 객체를 위한 일반 인터페이스. */
#include "include/threads/vaddr.h"
#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"

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
	list_init(&frame_table);      // lock 관련
	lock_init(&frame_table_lock); // lock 관련
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
uint64_t page_hash(const struct hash_elem *e, void *aux UNUSED);
bool page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
// 초기화 함수를 사용하여 보류 중인 페이지 객체를 생성합니다. 
// 페이지를 생성하려면 직접 생성하지 말고 이 함수나 `vm_alloc_page`를 통해 생성하세요.
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current() -> spt;
	upage = pg_round_down(upage);  // 정렬 보장(destroy 트러블 슈팅)

	/* Check wheter the upage is already occupied or not. */
	// 이미지가 이미 점유되어 있는지 확인하세요.
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		// 해야될 것: 페이지를 생성하고, VM 유형에 따라 초기화 파일을 가져온 후, uninit_new를 호출하여 "uninit" 페이지 구조체를 생성합니다.
		// 해야될 것: uninit_new를 호출한 후 필드를 수정해야 합니다.
		// 해당 함수는 아직 메모리에 로딩되지 않은 페이지에 대한 메타데이터를 생성해서 SPT에 등록하는게 목적입니다.

		struct page *page = malloc(sizeof(struct page));  // page 할당
		if (page == NULL)
			return false;

		enum vm_type real_type = VM_TYPE(type);
		bool (*page_initializer) (struct page *, enum vm_type, void *);
		if (real_type == VM_ANON) {
			page_initializer = anon_initializer;
		}
		else if (real_type == VM_FILE) {
			page_initializer = file_backed_initializer;
		}
		else {
			return false;   // 지원하지 않는 타입일때
		}

		page -> va = upage;  // 문제 수정중

		uninit_new(page, upage, init, type, aux, page_initializer);

		/* TODO: Insert the page into the spt. */
		// 해당 페이지를 spt에 삽입합니다.
		page -> writable = writable;    // 이 코드 없으면 pml4_set_page()는 정확한 권한 매핑불가

		if (!spt_insert_page (spt, page))
			return false;
		return true;
	}
}

/* Find VA from spt and return page. On error, return NULL. */
// spt에서 VA를 찾아 페이지를 반환합니다. 오류가 발생하면 NULL을 반환합니다.
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *page = NULL;
	struct page dummy;
	dummy.va = pg_round_down(va);  // 시작 주소를 맞춰준다. (정렬)

	/* TODO: Fill this function. */
	// 기능을 구현하세요.
	struct hash_elem *e = hash_find(&spt -> spt_hash, &dummy.hash_elem);
	if (e == NULL) return NULL;

	if(e != NULL)
		page = hash_entry(e, struct page, hash_elem);
	return page;
}

/* Insert PAGE into spt with validation. */
// 검증을 통해 spt에 PAGE를 삽입합니다.
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	// struct page *va;  // page 구조체에 있는 va를 가져온다.
	void *va = page->va;  // 쓰레드 관련 트러블 슈팅 해결중
	
	/* TODO: Fill this function. */
	//먼저, 주어진 보충 테이블에 가상주소가 존재하는지 확인해야한다.
	struct page *p = spt_find_page(spt, va);
	if (p == NULL) {
		struct hash_elem *inserted = hash_insert(&spt->spt_hash, &page->hash_elem);  // 만약, 페이지가 비었다면 spt에 페이지를 삽입합니다.
		succ = true;
	}
	else 
		return false;
	return succ;
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
	frame = calloc(1, sizeof(struct frame));
	if(frame == NULL) PANIC("todo");

	void *kva = palloc_get_page(PAL_USER);    // 커널이 접근할 수 있는 물리 페이지 주소
	if(kva == NULL) PANIC("todo");    // 사용자 풀에서 페이지를 성공적으로 가져왔는가?

	frame -> kva = kva;

	lock_acquire(&frame_table_lock);
	list_push_back(&frame_table, &frame->elem);
	lock_release(&frame_table_lock);

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL); 

	return frame;
}

/* Growing the stack. */
// 스택을 키웁니다.
static bool
vm_stack_growth (void *addr UNUSED) {
	struct thread *t = thread_current();
	void *va = pg_round_down(addr);  // 주소를 정렬에 맞춰준다.

	// 실제로 할당 요청
    return vm_alloc_page(VM_ANON | VM_MARKER_0, va, true);  // VM_MARKER_0: 스택에서 생성된 익명 페이지를 구별하기 위함
}
// 스택에서 생성된 익명 페이지를 구별하기 위함
/* Handle the fault on write_protected page */
// write_protected 페이지에서 오류를 처리합니다.
static bool
vm_handle_wp (struct page *page UNUSED) {
}
//#define MAX_STACK_LIMIT (1 << 20)  // 1MB
/* Return true on success */
// 성공 시 true를 반환합니다.
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = spt_find_page(spt, addr);  /* valid address인지 확인 */

	void *user_rsp = thread_current()->user_rsp;  // 시스템 콜 중이라도 유저 %rsp
	// void *user_rsp = user ? thread_current()->user_rsp : f->rsp;

	void *va = pg_round_down(addr);  // 주소를 정렬에 맞춰준다.
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	// case 1: page가 없음 → stack growth 고려
	// 스택 자동확장이 필요한 상황일 때, vm_stack_growth 함수로 확장해줍니다.
	if(page == NULL) {
		// 유저 모드에서 발생한 폴트만 처리, 유저 스택 포인터 바로 아래에 접근할 시 허용, 접근한 주소가 전체 유저 스택 범위 안에 있는지 확인. -> 모든 조건일시 스택 확장
		// 스택확장 부분이 왜 if문에 있는지 의아할 수 있는데, 이는 스택확장이 무조건 성공하지 않으므로 실패시 spt_find_page를 실행하지 않도록 방지한다.
		if (is_user_vaddr(addr) && user && addr >= f->rsp - 32 && addr < USER_STACK && vm_stack_growth(addr)) {
			page = spt_find_page(spt, addr);   // addr이 현재 스택 영역 바로 아래에 접근한 경우에만 새 페이지를 할당
		}
		else
			return false;
	}

	// case 2: 접근 권한 확인 (write 접근인데 read-only면 실패)
	if (write && !page->writable)  // 쓰기 접근인데 페이지가 읽기 전용이라면, false 반환
    	return false;
	
	// 페이지 적재 (uninit이든 아니든 모두 처리)
	return vm_do_claim_page (page);
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
	struct page *page = NULL;
	/* TODO: Fill this function */
	// 기능을 구현하세요.
	page = spt_find_page(&thread_current() -> spt, va);  // spt 페이지를 가져옵니다.

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
// PAGE를 선언하고 mmu를 설정하세요.
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();
	if(frame == NULL) {
		return false;
	}

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	// 페이지의 VA를 프레임의 PA에 매핑하기 위해 페이지 테이블 항목을 삽입합니다.
	pml4_set_page(thread_current() -> pml4,  page -> va, frame -> kva, page->writable);

	return swap_in (page, frame->kva);
}

// 페이지 va를 바탕으로 고유한 해시값 생성
// 해시값을 통해 bucket을 찾는다.
uint64_t page_hash(const struct hash_elem *e, void *aux) {
	struct page *p = hash_entry(e, struct page, hash_elem);
    return hash_bytes(&p -> va, sizeof(p -> va));  // 주소를 기준으로 비교
}

// bucket 내부 충돌 일시(같은 해시값) 정렬기준을 제공합니다.
// 내부적인 bucket에서 이진 탐색트리 또는 리스트 정렬할 때 사용된다.
bool page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux) {
    struct page *page_a = hash_entry(a, struct page, hash_elem);
    struct page *page_b = hash_entry(b, struct page, hash_elem);
    return page_a -> va < page_b -> va;  // 주소를 기준으로 비교
}

/* Initialize new supplemental page table */
// 새로운 보충 페이지 테이블을 초기화합니다.
// spt는 페이지 폴트 발생 시 해당 vm에서 어떤 데이터가 있어야 하는지 파악합니다.
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt -> spt_hash, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
// src에서 dst로 보충 페이지 테이블 복사합니다.
// 부모 프로세스가 자식에 상속할 떄 사용. 이 함수의 결과로 자식은 부모의 주소를 완벽히 복사한다.
// src는 부모의 주소 공간, dst는 자식의 주소 공간이다.
// src와 dst는 지칭하는 대명사와 같다. 결론적으로 src에는 현재 스레드의 spt가 들어갈것이다.
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
		
		// src의 해시 테이블 순회 준비 
		struct hash_iterator i;
		hash_first(&i, &src -> spt_hash);  // spt가 해쉬에 있음. first로 첫번째 버킷 지정

		// 모든 page를 하나씩 순회(모든 버킷)
		while (hash_next(&i)) {
			struct page *src_page = hash_entry(hash_cur(&i), struct page, hash_elem);

			// 공통 메타데이터 추출
			void *va = src_page -> va;
			enum vm_type type = page_get_type(src_page);  // 부모 실제 타입 추출
			bool writable = src_page -> writable;
			
			// unint 타입인지 확인
			if (src_page->operations->type == VM_UNINIT) {     // fork-read를 위한 수정
				// init 함수와 aux 복사 (지연로딩)
				struct uninit_page *uninit = &src_page -> uninit;
				vm_initializer *init = uninit -> init;
				void *aux = uninit -> aux;

				// dst에 등록
				vm_alloc_page_with_initializer(type, va, writable, init, aux);
			}

			// 초기화된 페이지라면
			else {
				// dst에도 동일한 페이지를 생성한다.
				vm_alloc_page_with_initializer(type, va, writable, NULL, NULL);

				// dst 페이지 가져와서 claim 한다.
				struct page *dst_page = spt_find_page(dst, va);
				vm_claim_page(va);
				// printf("dstpage: %p\n", dst_page -> frame -> kva);
				// printf("srcpage: %p\n", src_page -> frame -> kva);
				// 실제 프레임 데이터를 복사
				memcpy(dst_page -> frame -> kva, src_page -> frame -> kva, PGSIZE);  // 목적지, 출발지, 사이즈 순이다.
			}

			// 기타 속성을 복사
			if (src_page -> is_stack) 
				spt_find_page(dst, va) -> is_stack = true;
		}
		return true;
}

// 타입에 맞게 삭제해주는 함수.
// void page_destroy (struct hash_elem *e, void *aux) {
// 	struct page *src_page = hash_entry(e, struct page, hash_elem);
	
// 	vm_dealloc_page(src_page);    // page 구조체 해제(free, destroy 함수 포함)
// 	printf("here\n");

// 	if (src_page->frame != NULL) {
// 		printf("here\n");
// 		void *va = pg_round_down(src_page->va);   // 정렬 보장(destroy 트러블 슈팅)
//         pml4_clear_page(thread_current()->pml4, va);
		
//         // frame_free(src_page->frame);
// 		printf("here\n");
// 		src_page->frame->page = NULL;
//         src_page->frame = NULL;
//     }
//     if (src_page->operations && src_page->operations->destroy)
//         destroy(src_page);
		
// 	free (src_page);
// }

void page_destroy (struct hash_elem *e, void *aux) {
	struct page *src_page = hash_entry(e, struct page, hash_elem);
	
 	// 페이지 매핑 해제 & 프레임이 있다면 해제
    vm_dealloc_page(src_page);
}

/* Free the resource hold by the supplemental page table */
// 보충 페이지 테이블에서 리소스 보류를 해제합니다.
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	// 스레드가 보유한 supplemental_page_table을 모두 파괴하고 수정된 내용을 모두 저장소에 다시 쓰게 구현하세요.
	hash_clear(&spt -> spt_hash, page_destroy);  // 그 안에 있는 페이지들 해제(page_destroy), 해시들도 해제, 
	
	// hash_destroy(&spt -> spt_hash, page_destroy);  // hash_destory로 하면 문제가 생긴다. 아마 process_cleanup 두번하는 문제도 있고, 다양하다.
}