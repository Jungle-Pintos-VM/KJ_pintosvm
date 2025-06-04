#ifndef VM_VM_H
#define VM_VM_H
#include <stdbool.h>
#include "threads/palloc.h"

enum vm_type {
	/* page not initialized 아직 초기화되지 않은 페이지*/
	VM_UNINIT = 0,
	/* page not related to the file, aka anonymous page 파일과 관련 없는 페이지, 일명 익명 페이지 */
	VM_ANON = 1,
	/* page that realated to the file 파일과 관련된 페이지*/
	VM_FILE = 2,
	/* page that hold the page cache, for project 4 페이지 캐시를 담고 있는 페이지*/
	VM_PAGE_CACHE = 3,

	/* Bit flags to store state 상태를 저장하기 위한 비트 플래그들*/

	/* Auxillary bit flag marker for store information. You can add more
	 * markers, until the value is fit in the int.
	 * 정보를 저장하기 위한 보조 비트 플래그 마커입니다. int 값에 맞을 때까지
	 * 더 많은 마커를 추가할 수 있습니다.*/
	VM_MARKER_0 = (1 << 3), // 이진수 ...00001000(값 8)
	VM_MARKER_1 = (1 << 4), // 이진수 ...00010000(값 16)

	/* DO NOT EXCEED THIS VALUE. 이 값을 초과하지 마십시오*/
	VM_MARKER_END = (1 << 31),
};

#include "vm/uninit.h"
#include "vm/anon.h"
#include "vm/file.h"
#ifdef EFILESYS
#include "filesys/page_cache.h"
#endif

struct page_operations;
struct thread;

#define VM_TYPE(type) ((type) & 7)

/* The representation of "page".
 * This is kind of "parent class", which has four "child class"es, which are
 * uninit_page, file_page, anon_page, and page cache (project4).
 * DO NOT REMOVE/MODIFY PREDEFINED MEMBER OF THIS STRUCTURE.
 * page의 표현입니다.
 * 이것은 일종의 "부모 클래스"이며, 네 개의 "자식 클래스"를 가집니다
 * uninit_page, file_page, anon_page, 그리고 페이지 캐시(프로젝트4)
 * 이 구조체의 미리 정의된 멤버를 제거하거나 수정하지 마십시오.*/
struct page {
	const struct page_operations *operations; //페이지 타입별 연산을 정의하는 함수 포인터 테이블
	void *va;              /* Address in terms of user space 사용자 공간 기준의 주소(가상 주소) */
	struct frame *frame;   /* Back reference for frame 프레임에 대한 역참조(이 페이지가 현재 어떤 물리 프레임에 있는지)*/

	/* Your implementation 여러분의 구현 부분(여기에 필요한 추가 멤버들을 정의합니다)*/
    //이것도 구현되어 있음 hash.h의 46번줄, hash_elem 내부에 struct list_elem이 정의되어있고 링크드 리스트 형태로 prev, next가 있음.
    struct hash_elem hash_elem;
    //이걸 통해서 pintos 해시 테이블 라이브러리가 제공하는 함수들을 통해 해시 테이블에 추가되거나 검색될 수 있는 해시 가능한 항목이 되는 것.
    //struct page 객체들을 해시 테이블에서 관리할 수 있도록 하기 위함.

	/* Per-type data are binded into the union.
	 * Each function automatically detects the current union
	 * 타입별 데이터는 공용체(union)에 바인딩됩니다.
	 * 각 함수는 현재 공용체를 자동으로 감지합니다*/
	union {
		struct uninit_page uninit; // 초기화되지 않은 페이지 정보
		struct anon_page anon; // 익명 페이지 정보(스왑 관련)
		struct file_page file; // 파일 기반 페이지 정보 (실행 파일, 메모리 매핑 파일)
#ifdef EFILESYS
		struct page_cache page_cache;
#endif
	};
};

/* The representation of "frame" */
struct frame {
	void *kva;
	struct page *page;
};

/* The function table for page operations.
 * This is one way of implementing "interface" in C.
 * Put the table of "method" into the struct's member, and
 * call it whenever you needed.
 * 페이지 연산을 위한 함수 테이블입니다
 * 이것은 C언어에서 "인터페이스"를 구현하는 한 가지 방법입니다.
 * "메소드" 테이블을 구조체의 멤버에 넣고,
 * 필요할 때마다 그것을 호출합니다.*/
struct page_operations {
	bool (*swap_in) (struct page *, void *);//페이지 내용을 물리 메모리 프레임으로 가져오는 함수를 가리킴
	bool (*swap_out) (struct page *); // 물리 메모리 프레임에 있는 페이지 내용을 디스크로 내보내는 함수를 가리킴
	void (*destroy) (struct page *); // 페이지와 관련된 모든 자원을 정리하고 페이지 구조체 자체를 파괴(해체 준비)하는 함수를 가리킴
	enum vm_type type; //이 page_operations 테이블이 어떤 종류의 페이지를 위한 것인지 나타내는 타입 정보입니다. struct page의 union멤버와 연결되어, 해당 페이지 타입에 맞는 초기화 및 정리 작업을 수행하는 데 사용됨.
};

#define swap_in(page, v) (page)->operations->swap_in ((page), v)
#define swap_out(page) (page)->operations->swap_out (page)
#define destroy(page) \
	if ((page)->operations->destroy) (page)->operations->destroy (page)

/* Representation of current process's memory space.
 * We don't want to force you to obey any specific design for this struct.
 * All designs up to you for this.
 * 현재 프로세스 메모리 공간의 표현입니다.
 * 이 구조체에 대해 특정 디자인을 강요하고 싶지 않습니다.
 * 모든 디자인은 여러분에게 달려 있습니다.*/

//여기에서 보조 페이지 테이블을 구현하기 위한 멤버들을 정의해야함
//hash.h 86번줄에 hash 구조체가 만들어져 있음.
struct supplemental_page_table {
    struct hash spt_hash;
};


#include "threads/thread.h"
void supplemental_page_table_init (struct supplemental_page_table *spt);
bool supplemental_page_table_copy (struct supplemental_page_table *dst,
		struct supplemental_page_table *src);
void supplemental_page_table_kill (struct supplemental_page_table *spt);
struct page *spt_find_page (struct supplemental_page_table *spt,
		void *va);
bool spt_insert_page (struct supplemental_page_table *spt, struct page *page);
void spt_remove_page (struct supplemental_page_table *spt, struct page *page);

void vm_init (void);
bool vm_try_handle_fault (struct intr_frame *f, void *addr, bool user,
		bool write, bool not_present);

#define vm_alloc_page(type, upage, writable) \
	vm_alloc_page_with_initializer ((type), (upage), (writable), NULL, NULL)
bool vm_alloc_page_with_initializer (enum vm_type type, void *upage,
		bool writable, vm_initializer *init, void *aux);
void vm_dealloc_page (struct page *page);
bool vm_claim_page (void *va);
enum vm_type page_get_type (struct page *page);

#endif  /* VM_VM_H */
