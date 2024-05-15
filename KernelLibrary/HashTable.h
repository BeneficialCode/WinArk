#pragma once

typedef struct _HASH_BUCKET {
	SINGLE_LIST_ENTRY BucketLink;
	UINT64 Key;
}HASH_BUCKET, * PHASH_BUCKET;

typedef struct _HASH_TABLE {
	UINT32 EntryCount;
	UINT32 MaskBitCount : 5;
	UINT32 BucketCount : 27;
	PSINGLE_LIST_ENTRY Buckets;
}HASH_TABLE, * PHASH_TABLE;

typedef struct _HASH_TABLE_ITERATOR {
	PHASH_TABLE Hash;
	PSINGLE_LIST_ENTRY HashEntry;
	PSINGLE_LIST_ENTRY Bucket;
}HASH_TABLE_ITERATOR, * PHASH_TABLE_ITERATOR;

UINT64 HashBytesGeneric(PUCHAR pBytes, size_t size,
	UINT64 kMagicNum, UINT64 coefficient);
UINT64 HashBytesEx(PUCHAR pBytes, size_t size, UINT64 kMagicNum);
UINT64 HashBytes(PUCHAR pBytes, size_t size);
UINT64 HashUlongPtr(UINT64 value);
UINT64 HashUStringUpcase(PWCH pBuffer, int len);

UINT32 GetHighestBitIndex(UINT32 value);
UINT32 RoundToPowerOfTwo(UINT32 value, BOOLEAN roundUpToNext);
BOOLEAN IsPowerOfTwo(UINT32 x);


void HashTableInitialize(PHASH_TABLE Hash, UINT32 MaskBitCount,
	UINT32 BucketCount, PSINGLE_LIST_ENTRY Buckets);
UINT32 HashTableGetBucketIndex(UINT32 BucketCount, UINT64 Key);
UINT32 HashTableInsert(PHASH_TABLE Hash, PHASH_BUCKET pBucket);
BOOLEAN HashBucketLastLink(PSINGLE_LIST_ENTRY bucket);
PSINGLE_LIST_ENTRY HashTableChangeTable(PHASH_TABLE Hash, ULONG allocCount, PSINGLE_LIST_ENTRY pBuckets);
PSINGLE_LIST_ENTRY HashTableFindNext(PHASH_TABLE Hash, UINT64 HashValue, PSINGLE_LIST_ENTRY pLink);
PHASH_TABLE HashTableGetTable(PSINGLE_LIST_ENTRY HashEntry);
PSINGLE_LIST_ENTRY HashTableCleanup(PHASH_TABLE Hash);
PSINGLE_LIST_ENTRY HashTableRemoveKey(PHASH_TABLE Hash, UINT64 HashValue);

void HashTableIterInit(PHASH_TABLE_ITERATOR Iterator, PHASH_TABLE Hash);
PSINGLE_LIST_ENTRY HashTableIterGetNext(PHASH_TABLE_ITERATOR Iterator);
PSINGLE_LIST_ENTRY HashTableIterRemove(PHASH_TABLE_ITERATOR Iterator);