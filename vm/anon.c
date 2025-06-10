/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */
// anon.c: 디스크가 아닌 이미지(익명 페이지라고도 함)에 대한 페이지 구현을 담당합니다.

#include "vm/vm.h"
#include "devices/disk.h"

/* DO NOT MODIFY BELOW LINE */
// ※ 해당 라인은 수정하지마세요. ※
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
// ※ 해당 구조체는 수정하지 마세요. ※
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
// 익명 페이지에 대한 데이터 초기화를 수행합니다.
void
vm_anon_init (void) {
	// /* TODO: Set up the swap_disk. */
	// swap_disk를 설정하세요.
	swap_disk = NULL;
}
// bool vm_anon_init(struct page *page, enum vm_type type, void *kva) {
//     // 1. page_operations 설정
//     page->operations = &anon_ops;

//     // 2. anon_page 필드 초기화
//     struct anon_page *anon_page = &page->anon;
//     anon_page->swap_slot = -1;  // 아직 스왑되지 않았음을 나타냄

//     return true;  // 성공적으로 초기화되었음을 알림
// }

/* Initialize the file mapping */
// 파일 매핑을 초기화합니다.
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page -> operations = &anon_ops;  // 페이지가 anon으로 동작
	// page -> anon.aux = aux;

	struct anon_page *anon_page = &page->anon;

	// anon_page->swap_slot = -1;  // 아직 스왑되지 않았음을 나타냄
	// return true;
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
	struct anon_page *anon_page = &page->anon;
}
