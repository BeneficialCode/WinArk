#pragma once

typedef struct _HASH_ENTRY {
	SINGLE_LIST_ENTRY Link;
	ULONG_PTR HashValue;
}HASH_ENTRY, * PHASH_ENTRY;

typedef struct _HASH_BUCKET {
	union {
		ULONG_PTR Hash;
		SINGLE_LIST_ENTRY Link;
	};
	UINT64 HashValue;
}HASH_BUCKET,*PHASH_BUCKET;

typedef struct _HASH_TABLE {
	UINT32 ItemCount;
	UINT32 BucketCount;
	PSINGLE_LIST_ENTRY Buckets;
}HASH_TABLE,*PHASH_TABLE;

typedef struct _HASH_TABLE_ITERATOR {
	PHASH_TABLE Hash;
	PHASH_BUCKET HashEntry;
	PHASH_BUCKET Bucket;
}HASH_TABLE_ITERATOR, * PHASH_TABLE_ITERATOR;

UINT64 HashBytesGeneric(PUCHAR pBytes, size_t size,
	UINT64 kMagicNum, UINT64 coefficient);

UINT64 HashBytesEx(PUCHAR pBytes, size_t size, UINT64 kMagicNum);

UINT64 HashBytes(PUCHAR pBytes, size_t size);

UINT64 HashUlongPtr(UINT64 value);

UINT64 HashUStringUpcase(PWCH pBuffer, int len);

UINT32 HashTableGetBucketIndex(UINT32 bucketCount, UINT64 key);

UINT32 HashTableInsert(PHASH_TABLE Hash, PHASH_BUCKET pBucket);

BOOLEAN HashBucketLastLink(PHASH_BUCKET bucket);

void HashTableInitialize(PHASH_TABLE Hash, UINT32 Flags,
	UINT32 BucketCount, PSINGLE_LIST_ENTRY Buckets);

UINT32 GetHighestBitIndex(UINT32 value);

UINT32 RoundToPowerOfTwo(UINT32 value, BOOLEAN roundUpToNext);

BOOLEAN IsPowerOfTwo(UINT32 x);

PHASH_BUCKET HashTableCleanup(PHASH_TABLE Hash);
PHASH_BUCKET HashTableFindNext(PHASH_TABLE Hash, UINT64 Key, PHASH_BUCKET Bucket);
PHASH_TABLE HashTableGetTable(PHASH_BUCKET HashEntry);
PHASH_BUCKET HashTableChangeTable(PHASH_TABLE Hash, ULONG size, PSINGLE_LIST_ENTRY pBucket);
PHASH_TABLE_ITERATOR HashTableIterInit(PHASH_TABLE_ITERATOR Iterator, PHASH_TABLE Hash);
PHASH_BUCKET HashTableIterGetNext(PHASH_TABLE_ITERATOR Iterator);
PHASH_BUCKET HashTableIterRemove(PHASH_TABLE_ITERATOR Iterator);