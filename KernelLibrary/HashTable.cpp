#include "pch.h"
#include "HashTable.h"

UINT64 HashBytesGeneric(PUCHAR pBytes, size_t size,
	UINT64 kMagicNum, UINT64 coefficient) {
	UINT64 hashCode = kMagicNum;
	for (int i = 0; i < size; i++) {
		hashCode = pBytes[i] + coefficient * hashCode;
	}
	return hashCode;
}

UINT64 HashBytesEx(PUCHAR pBytes, size_t size, UINT64 kMagicNum) {
	return HashBytesGeneric(pBytes, size, kMagicNum, 37);
}

UINT64 HashBytes(PUCHAR pBytes, size_t size) {
	return HashBytesEx(pBytes, size, 314159);
}

UINT64 HashUlongPtr(UINT64 value) {
	PUCHAR pBytes = (PUCHAR)&value;
	return HashBytes(pBytes, sizeof(value));
}

UINT64 HashUStringUpcase(PWCH pBuffer, int len) {
	PWCH pStart = pBuffer;
	PWCH pEnd = &pBuffer[len];
	UINT64 kMagicNumber = 314159;
	while (pStart < pEnd) {
		WCHAR upper = RtlUpcaseUnicodeChar(*pBuffer++);
		kMagicNumber = HashBytesEx((PUCHAR)&upper, sizeof(upper), kMagicNumber);
	}
	return kMagicNumber;
}

UINT32 HashTableGetBucketIndex(UINT32 bucketCount, UINT64 key) {
	return (bucketCount - 1) & HashUlongPtr(key);
}

UINT32 HashTableInsert(PHASH_TABLE Hash, PHASH_BUCKET Entry) {
	UINT32 count = (Hash->BucketCount >> 5) & 0x7FFFFFF;
	UINT64 key = (ULONGLONG(-1) << (Hash->BucketCount & 0x1F)) & Entry->HashValue;
	UINT32 idx = HashTableGetBucketIndex(count, key);

	PushEntryList(&Hash->Buckets[idx], (PSINGLE_LIST_ENTRY)Entry);
	count = Hash->ItemCount + 1;
	Hash->ItemCount = count;
	return count;
}

BOOLEAN HashBucketLastLink(PHASH_BUCKET bucket) {
	return (bucket->Hash & 1) != 0;
}

UINT32 GetHighestBitIndex(UINT32 value) {
	UINT32 index = 0;
	while (value > 0) {
		index += 1;
		value >>= 1;
	}
	return index;
}

void HashTableInitialize(PHASH_TABLE Hash, UINT32 Flags,
	UINT32 BucketCount, PSINGLE_LIST_ENTRY Buckets) {
	UINT32 count = RoundToPowerOfTwo(BucketCount, FALSE);
	if (count > 0x4000000)
		count = 0x4000000;
	Hash->ItemCount = 0;
	Hash->BucketCount = (count << 5) | Hash->BucketCount & 0x1F;
	Hash->Buckets = Buckets;
	Hash->BucketCount = Flags & 0x1F | Hash->BucketCount & 0xFFFFFFE0;
	PSINGLE_LIST_ENTRY p = Buckets;
	PHASH_BUCKET pEnd = (PHASH_BUCKET)&Buckets[count];
	if (p) {
		while ((PHASH_BUCKET)p < pEnd) {
			((PHASH_BUCKET)(p))->Hash = ((ULONG_PTR)Hash | 1);
			p++;
		}
	}
}

// Returns true if the given number is a power of 2.
BOOLEAN IsPowerOfTwo(UINT32 x) {
	return (x > 0) && ((x & (x - 1)) == 0);
}

UINT32 RoundToPowerOfTwo(UINT32 value, BOOLEAN roundUpToNext){
	if (0 == value) {
		return value;
	}
	// if value is a power of 2, return it
	if (IsPowerOfTwo(value)) {
		return value;
	}

	UINT32 idx = GetHighestBitIndex(value);
	if (roundUpToNext)
		++idx;
	return (1ul << idx);
}

PHASH_BUCKET HashTableCleanup(PHASH_TABLE Hash) {
	return (PHASH_BUCKET)Hash->Buckets;
}

PHASH_BUCKET HashTableFindNext(PHASH_TABLE Hash, UINT64 Key, PHASH_BUCKET Bucket) {
	UINT64 value = ULONGLONG(-1) << (Hash->BucketCount & 0x1F);
	UINT64 k = value & Key;
	PHASH_BUCKET pBucket = NULL;
	BOOL bLastLink = FALSE;
	if (Bucket) {
		pBucket = Bucket;
		bLastLink = HashBucketLastLink(pBucket);
	}
	else {
		UINT32 count = (Hash->BucketCount >> 5) & 0x7FFFFFF;
		if (count == 0)
			return NULL;
		UINT32 idx = HashTableGetBucketIndex(count, k);
		pBucket = (PHASH_BUCKET)&Hash->Buckets[idx];
	}

	for (bLastLink = HashBucketLastLink(pBucket); 
		!bLastLink;
		bLastLink = HashBucketLastLink(pBucket)) {
		if (k == (value & ((PHASH_BUCKET)(pBucket->Link.Next))->HashValue)) {
			return (PHASH_BUCKET)pBucket->Link.Next;
		}
		pBucket = (PHASH_BUCKET)pBucket->Link.Next;
	}
	return NULL;
}

PHASH_TABLE HashTableGetTable(PHASH_BUCKET HashEntry) {
	BOOL bLastLink = FALSE;
	PHASH_TABLE pHash = NULL;
	PHASH_BUCKET pBucket = (PHASH_BUCKET)HashEntry;
	PHASH_BUCKET pEntry = NULL;

	for (bLastLink = HashBucketLastLink(pBucket); !bLastLink;
		bLastLink = HashBucketLastLink(pBucket)) {
		pEntry = (PHASH_BUCKET)(pBucket->Link.Next);
	}

	pHash = (PHASH_TABLE)(pBucket->Hash & ~1ull);
	pEntry = NULL;
	do
	{
		pEntry = HashTableFindNext(pHash, HashEntry->HashValue, pEntry);
	} while (pEntry && pEntry != HashEntry);

	return pHash;
}

PHASH_BUCKET HashTableChangeTable(PHASH_TABLE Hash, ULONG size,
	PSINGLE_LIST_ENTRY pBuckets) {
	PHASH_BUCKET pOldBuckets = NULL;
	UINT32 count = RoundToPowerOfTwo(size, FALSE);
	if (count > 0x4000000)
		count = 0x4000000;
	PSINGLE_LIST_ENTRY p = pBuckets;
	PHASH_BUCKET pEnd = (PHASH_BUCKET)&pBuckets[count];
	for (; p < (PSINGLE_LIST_ENTRY)pEnd; ++p) {
		((PHASH_BUCKET)(p))->Hash = (ULONG_PTR)Hash | 1;
	}
	
	UINT64 value = ULONGLONG(-1) << (Hash->BucketCount & 0x1F);
	UINT32 bucketCount = (Hash->BucketCount >> 5) & 0x7FFFFFF;
	for (UINT32 j = 0; j < bucketCount; ++j) {
		PHASH_BUCKET pBucket = (PHASH_BUCKET)&Hash->Buckets[j];
		while (!HashBucketLastLink(pBucket)) {
			PHASH_ENTRY pEntry = (PHASH_ENTRY)pBucket->Link.Next;
			pBucket->Link.Next = pBucket->Link.Next->Next;
			UINT32 idx = HashTableGetBucketIndex(bucketCount, value & pEntry->HashValue);
			PushEntryList((PSINGLE_LIST_ENTRY)&pBuckets[idx], (PSINGLE_LIST_ENTRY)&p->Next);
		}
	}
	pOldBuckets = (PHASH_BUCKET)Hash->Buckets;
	Hash->Buckets = (PSINGLE_LIST_ENTRY)pBuckets;
	Hash->BucketCount = (count << 5) | Hash->BucketCount & 0x1F;

	return pOldBuckets;
}

PHASH_TABLE_ITERATOR HashTableIterInit(PHASH_TABLE_ITERATOR Iterator,
	PHASH_TABLE Hash) {
	RtlZeroMemory(Iterator, sizeof(HASH_TABLE_ITERATOR));
	Iterator->Hash = Hash;
	Iterator->Bucket = (PHASH_BUCKET)Hash->Buckets;
	Iterator->HashEntry = Iterator->Bucket;
	return Iterator;
}

PHASH_BUCKET HashTableIterGetNext(PHASH_TABLE_ITERATOR Iterator) {
	PHASH_BUCKET HashEntry = Iterator->HashEntry;
	UINT32 count = 0;

	if (!HashEntry || HashBucketLastLink(HashEntry)) {
		// 这个分支完全没有走过 直接就g了 相当于连链表的大小都没判断过
		PHASH_TABLE Hash = Iterator->Hash;
		PSINGLE_LIST_ENTRY pBucket = (PSINGLE_LIST_ENTRY)&Iterator->Bucket->HashValue;

		count = (Hash->BucketCount >> 5) & 0x7FFFFFF;
		PHASH_BUCKET pEnd = (PHASH_BUCKET)&Hash->Buckets[count];
		while (TRUE)
		{
			if (pBucket >= (PSINGLE_LIST_ENTRY)pEnd)
				return NULL;
			if (!HashBucketLastLink((PHASH_BUCKET)pBucket))
				break;
			pBucket++;
		}
		PHASH_BUCKET pHashEntry = (PHASH_BUCKET)pBucket->Next;
		Iterator->Bucket = (PHASH_BUCKET)pBucket;
		Iterator->HashEntry = pHashEntry;
		return pHashEntry;
	}
	else {
		// 一直走的这 一直顺着链表往下遍历 很有可能是过程里面 有成员被移除了。数据不一致，我和他不同步，嗯 你们不同步啊
		// 这方法都没法让别人遍历了，真好。有因为可以try catch估计问题不大 多写几个判断就行了。啥判断啊，你又没法影响wdfilter MmCopyVirtualMemory 然后 MmIsValid把 估计不会蓝
		// 不好控制，今天就这样吧？ok
		Iterator->HashEntry = (PHASH_BUCKET)HashEntry->Link.Next;
		return Iterator->HashEntry;
	}
}

PHASH_BUCKET HashTableIterRemove(PHASH_TABLE_ITERATOR Iterator) {
	PHASH_BUCKET pHashEntry;
	pHashEntry = Iterator->HashEntry;
	PHASH_BUCKET pBucket = Iterator->Bucket;
	BOOLEAN bLastLink = FALSE;

	for (bLastLink = HashBucketLastLink(pBucket); !bLastLink; bLastLink = HashBucketLastLink(pBucket)) {
		if (pBucket == pHashEntry) {
			pBucket->Hash = pHashEntry->Hash;
			--Iterator->Hash->ItemCount;
			pHashEntry->Hash |= 0x8000000000000002ui64;
			Iterator->HashEntry = pBucket;
			return pHashEntry;
		}
		pBucket = (PHASH_BUCKET)pBucket->Link.Next;
	}

	return NULL;
}