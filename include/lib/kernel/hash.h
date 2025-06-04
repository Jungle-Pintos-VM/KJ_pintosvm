#ifndef __LIB_KERNEL_HASH_H
#define __LIB_KERNEL_HASH_H

/* Hash table.
 *
 * This data structure is thoroughly documented in the Tour of
 * Pintos for Project 3.
 *
 * This is a standard hash table with chaining.  To locate an
 * element in the table, we compute a hash function over the
 * element's data and use that as an index into an array of
 * doubly linked lists, then linearly search the list.
 *
 * The chain lists do not use dynamic allocation.  Instead, each
 * structure that can potentially be in a hash must embed a
 * struct hash_elem member.  All of the hash functions operate on
 * these `struct hash_elem's.  The hash_entry macro allows
 * conversion from a struct hash_elem back to a structure object
 * that contains it.  This is the same technique used in the
 * linked list implementation.  Refer to lib/kernel/list.h for a
 * detailed explanation.
 * 해시 테이블.
 *
 * 이 자료구조는 프로젝트 3을 위한 Pintos 둘러보기(Tour of Pintos)에
 * 상세히 문서화되어 있습니다.
 *
 * 이것은 체이닝(chaining)을 사용하는 표준적인 해시 테이블입니다. 테이블에서
 * 요소를 찾기 위해, 요소의 데이터에 대해 해시 함수를 계산하고
 * 그 결과를 이중 연결 리스트 배열의 인덱스로 사용한 다음,
 * 해당 리스트를 선형적으로 검색합니다.
 *
 * 체인 리스트는 동적 할당을 사용하지 않습니다. 대신, 해시에
 * 잠재적으로 포함될 수 있는 각 구조체는 struct hash_elem 멤버를
 * 포함해야 합니다. 모든 해시 함수는 이러한 `struct hash_elem`들에
 * 대해 작동합니다. hash_entry 매크로는 struct hash_elem에서
 * 그것을 포함하는 구조체 객체로 다시 변환하는 것을 허용합니다.
 * 이것은 연결 리스트 구현에서 사용된 것과 동일한 기법입니다.
 * 자세한 설명은 lib/kernel/list.h를 참조하십시오.*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "list.h" //해시 버킷이 리스트로 구현되므로 list.h 포함

/* Hash element. 해시 요소*/
struct hash_elem {
	struct list_elem list_elem; //해시 체이닝을 위한 리스트 요소
};

/* Converts pointer to hash element HASH_ELEM into a pointer to
 * the structure that HASH_ELEM is embedded inside.  Supply the
 * name of the outer structure STRUCT and the member name MEMBER
 * of the hash element.  See the big comment at the top of the
 * file for an example.
 * 해시 요소 HASH_ELEM에 대한 포인터를 HASH_ELEM이 내장된
 * 구조체에 대한 포인터로 변환합니다. 외부 구조체의 이름 STRUCT와
 * 해시 요소의 멤버 이름 MEMBER를 제공하십시오.
 * 예시는 이 파일 상단의 큰 주석을 참조하십시오.*/
#define hash_entry(HASH_ELEM, STRUCT, MEMBER)                   \
	((STRUCT *) ((uint8_t *) &(HASH_ELEM)->list_elem        \
		- offsetof (STRUCT, MEMBER.list_elem)))

/* Computes and returns the hash value for hash element E, given
 * auxiliary data AUX.
 * 보조 데이터 AUX가 주어졌을 때, 해시 요소 E에 대한 해시 값을
 * 계산하고 반환합니다.*/
typedef uint64_t hash_hash_func (const struct hash_elem *e, void *aux);

/* Compares the value of two hash elements A and B, given
 * auxiliary data AUX.  Returns true if A is less than B, or
 * false if A is greater than or equal to B.
 * 보조 데이터 AUX가 주어졌을 때, 두 해시 요소 A와 B의 값을
 * 비교합니다. A가 B보다 작으면 true를 반환하고,
 * A가 B보다 크거나 같으면 false를 반환합니다.*/
typedef bool hash_less_func (const struct hash_elem *a,
		const struct hash_elem *b,
		void *aux);

/* Performs some operation on hash element E, given auxiliary
 * data AUX.
 * 보조 데이터 AUX가 주어졌을 때, 해시 요소 E에 대해
 * 어떤 작업을 수행합니다.*/
typedef void hash_action_func (struct hash_elem *e, void *aux);

/* Hash table. 해시 테이블*/
struct hash {
	size_t elem_cnt;            /* Number of elements in table.  테이블 내 요소의 수*/
	size_t bucket_cnt;          /* Number of buckets, a power of 2. 버킷의 수, 2의 거듭제곱이어야 함.*/
	struct list *buckets;       /* Array of `bucket_cnt' lists.  bucket_cnt 개수만큼의 리스트 배열.*/
	hash_hash_func *hash;       /* Hash function. 해시값 계산하는 함수*/
	hash_less_func *less;       /* Comparison function. 비교 함수 (정렬 또는 특정 요소 찾기에 사용될 수 있음)*/
	void *aux;                  /* Auxiliary data for `hash' and `less'. hash 및 lsee함수를 위한 보조 데이터*/
};

/* A hash table iterator. 해시 테이블 반복자(iterator)*/
struct hash_iterator {
	struct hash *hash;          /* The hash table. 해시 테이블*/
	struct list *bucket;        /* Current bucket. 현재 버킷(리스트)*/
	struct hash_elem *elem;     /* Current hash element in current bucket. 현재 버킷 내의 현재 해시 요소.*/
};

/* Basic life cycle. 기본적인 생명 주기 함수들*/
bool hash_init (struct hash *, hash_hash_func *, hash_less_func *, void *aux);//해시 테이블 초기화
void hash_clear (struct hash *, hash_action_func *); // 해시 테이블의 모든 요소 제거(액션 함수 적용 가능)
void hash_destroy (struct hash *, hash_action_func *);// 해시 테이블 파괴 (액션 함수 적용 가능)

/* Search, insertion, deletion. 검색, 삽입, 삭제 함수들*/
struct hash_elem *hash_insert (struct hash *, struct hash_elem *); //요소 삽입
struct hash_elem *hash_replace (struct hash *, struct hash_elem *); //요소 교체(기존 요소가 있다면 제거 후 새 요소 삽입)
struct hash_elem *hash_find (struct hash *, struct hash_elem *); //요소 검색
struct hash_elem *hash_delete (struct hash *, struct hash_elem *); //요소 삭제

/* Iteration. 반복 함수들*/
void hash_apply (struct hash *, hash_action_func *); //모든 요소에 액션 함수 적용
void hash_first (struct hash_iterator *, struct hash *); //반복자 초기화(첫 번째 요소)
struct hash_elem *hash_next (struct hash_iterator *); //다음 요소로 이동
struct hash_elem *hash_cur (struct hash_iterator *); //현재 요소 반환

/* Information. 정보 함수들*/
size_t hash_size (struct hash *); //해시 테이블의 요소 개수 반환
bool hash_empty (struct hash *); //해시 테이블이 비었는지 확인

/* Sample hash functions. 샘플 해시 함수들*/
uint64_t hash_bytes (const void *, size_t); // 바이트 배열에 대한 해시 함수
uint64_t hash_string (const char *); //문자열에 대한 해시 함수
uint64_t hash_int (int); //정수에 대한 해시 함수

#endif /* lib/kernel/hash.h */
