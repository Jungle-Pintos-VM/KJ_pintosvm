/* uninit.c: Implementation of uninitialized page.
 *
 * All of the pages are born as uninit page. When the first page fault occurs,
 * the handler chain calls uninit_initialize (page->operations.swap_in).
 * The uninit_initialize function transmutes the page into the specific page
 * object (anon, file, page_cache), by initializing the page object,and calls
 * initialization callback that passed from vm_alloc_page_with_initializer
 * function.
 * */

/* uninit.c: 초기화되지 않은 페이지 구현합니다.
 *
 * 모든 페이지는 uninit 페이지로 생성됩니다.
 * 첫 번째 페이지 폴트가 발생하면 핸들러 체인은 uninit_initialize(page->operations.swap_in)를 호출합니다.
 * uninit_initialize 함수는 페이지 객체를 초기화하여 페이지를 특정 페이지 객체(anon, file, page_cache)로 변환하고,
 * vm_alloc_page_with_initializer 함수에서 전달된 초기화 콜백을 호출합니다. */

#include "vm/vm.h"
#include "vm/uninit.h"

static bool uninit_initialize (struct page *page, void *kva);
static void uninit_destroy (struct page *page);

/* DO NOT MODIFY this struct */
// ※ 이 구조를 수정하지 마십시오. ※
// 초기화 안 된 페이지의 행동 매뉴얼
static const struct page_operations uninit_ops = {
	.swap_in = uninit_initialize, // 첫 사용 시 초기화 (전원 켜기)
	.swap_out = NULL, // 없음(전원 끄기)(아직 안 켜졌으니까 NULL)
	.destroy = uninit_destroy, // 사용안하고 버릴 때
	.type = VM_UNINIT, // 미개봉 제품(제품 종류)
};

/* DO NOT MODIFY this function */
// ※ 이 구조를 수정하지 마십시오. ※
// 빈 페이지를 설정하는 함수, 아직 초기화되지 않은 페이지의 정보를 세팅함.
// 나중에 페이지 폴트가 발생하면 이 정보들을 사용해서 실제 페이지를 만듬.
void
uninit_new (struct page *page, void *va, vm_initializer *init,
		enum vm_type type, void *aux,
		bool (*initializer)(struct page *, enum vm_type, void *)) {
	ASSERT (page != NULL);

	*page = (struct page) {
		.operations = &uninit_ops,
		.va = va,
		.frame = NULL, /* no frame for now */
		.uninit = (struct uninit_page) {
			.init = init,
			.type = type,
			.aux = aux,
			.page_initializer = initializer,
		}
	};
}

/* Initalize the page on first fault */
// 첫 번째 오류 발생 시 페이지 초기화합니다.
static bool
uninit_initialize (struct page *page, void *kva) {
	struct uninit_page *uninit = &page->uninit;

	/* Fetch first, page_initialize may overwrite the values */
	// Fetch를 먼저하세요. page_initialize가 값을 덮어쓸 수 있습니다.
	vm_initializer *init = uninit->init;
	void *aux = uninit->aux;

	/* TODO: You may need to fix this function. */
	// 이 기능을 수정해야 할 수도 있습니다.
	return uninit->page_initializer (page, uninit->type, kva) &&
		(init ? init (page, aux) : true);
}

/* Free the resources hold by uninit_page. Although most of pages are transmuted
 * to other page objects, it is possible to have uninit pages when the process
 * exit, which are never referenced during the execution.
 * PAGE will be freed by the caller. */

/* uninit_page가 보유한 리소스를 해제합니다.
 * 대부분의 페이지는 다른 페이지 객체로 변환되지만,
 * 프로세스 종료 시 실행 중에 참조되지 않는 uninit 페이지가 생성될 수 있습니다.
 * PAGE는 호출자에 의해 해제됩니다. */
static void
uninit_destroy (struct page *page) {
	struct uninit_page *uninit UNUSED = &page->uninit;
	/* TODO: Fill this function.
	 * TODO: If you don't have anything to do, just return. */
	// 할 것이 없으면 그냥 작성 안해도됩니다.
}
