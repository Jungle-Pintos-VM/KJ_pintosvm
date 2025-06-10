/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */
// anon.c: 디스크가 아닌 이미지(익명 페이지라고도 함)에 대한 페이지 구현을 담당합니다.

#include "vm/vm.h" /*VM_TYPE, page_operations 등*/
#include "devices/disk.h" /*disk_get(), disk_size()*/
#include "threads/vaddr.h" /*PGSIZE, DISK_SECTOR_SIZE,pg_round_down 등 */
#include "threads/malloc.h" /*malloc/free*/
#include "threads/synch.h" /*lock_init*/
#include "threads/thread.h" /* thread_current */
#include "bitmap.h" /*bitmap_create*/
#include "string.h"


/* DO NOT MODIFY BELOW LINE */
// ※ 해당 라인은 수정하지마세요. ※
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/*스왑 슬롯 관리용 비트맵과 동기화 락*/
static struct bitmap *swap_table;
static struct lock swap_lock;

/* DO NOT MODIFY this struct */
// ※ 해당 구조체는 수정하지 마세요. ※
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/*페이지 하나당 디스크 섹터 수*/
static const size_t SECTORS_PER_PAGE = PGSIZE/ DISK_SECTOR_SIZE;

/* Initialize the data for anonymous pages */
// 익명 페이지에 대한 데이터 초기화를 수행합니다.
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	// swao_disk를 설정하세요.
	/*1.스왑 디스크 가져오기 (IDE 채널 1, 장치 1)*/
	swap_disk = disk_get(1,1);

	/*디스크 크기를 페이지 단위로 계산*/
	size_t swap_page_cnt = disk_size(swap_disk)/SECTORS_PER_PAGE;

	/*비트맵 생성(false = 빈 슬롯)*/
	swap_table = bitmap_create(swap_page_cnt);
	if (swap_table == NULL)
		PANIC("vm_anon_init : bitmap_create failed");

	lock_init(&swap_lock);
}

/* Initialize the file mapping */
// 파일 매핑을 초기화합니다.
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	/*페이지 연산자 구조체를 익명OPS로 설정*/
	page->operations = &anon_ops;
	struct anon_page *anon_page = &page->anon;

	// /*물리 메모리를 0으로 초기화*/
	// memset(kva, 0 ,PGSIZE);

	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
}

/* Swap out the page by writing contents to the swap disk. */
// 스왑 디스크에서 내용을 읽어 페이지를 스왑합니다.
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
// 익명 페이지를 삭제합니다. 호출자가 PAGE를 해제합니다.
static void
anon_destroy (struct page *page) {
//	struct anon_page *anon_page = &page->anon;
//    if(page->frame != NULL){
//        // 1. vm_get_frame에서 사용한 palloc_get_page로 할당한 물리 페이지를 해제함
//        palloc_free_page(page->frame->kva);
//        // 2. vm_get_frame에서 calloc으로 동적 메모리 할당한 struct frame 관리 구조체 자체를 해제
//        free(page->frame);
//    }
// 이 함수를 활성화 시키면 기존에 통과되던 테스트가 다 실패하게 됨.
}
