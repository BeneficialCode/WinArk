#include "pch.h"
#include "HashTable.h"

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
	PSINGLE_LIST_ENTRY pEnd = &Buckets[count];
	if (p) {
		while (p < pEnd) {
			p->Next = (PSINGLE_LIST_ENTRY)((ULONG_PTR)Hash | 1);
			p++;
		}
	}
}

// Returns true if the given number is a power of 2.
BOOLEAN IsPowerOfTwo(UINT32 x) {
	return (x > 0) && ((x & (x - 1)) == 0);
}

UINT32 RoundToPowerOfTwo(UINT32 value, BOOLEAN roundUpToNext) {
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

UINT32 HashTableGetBucketIndex(UINT32 bucketCount, UINT64 key) {
	return (bucketCount - 1) & HashUlongPtr(key);
}

UINT32 HashTableInsert(PHASH_TABLE Hash, PHASH_BUCKET pBucket) {
	UINT32 count = (Hash->BucketCount >> 5) & 0x7FFFFFF;
	UINT32 flags = Hash->BucketCount & 0x1F;
	UINT64 key = (MAXULONG_PTR << flags) & pBucket->HashValue;
	UINT32 idx = HashTableGetBucketIndex(count, key);

	PushEntryList(&Hash->Buckets[idx], (PSINGLE_LIST_ENTRY)pBucket);
	count = Hash->ItemCount + 1;
	Hash->ItemCount = count;
	return count;
}

BOOLEAN HashBucketLastLink(PSINGLE_LIST_ENTRY bucket) {
	return ((ULONG_PTR)bucket->Next & 1) != 0;
}

PSINGLE_LIST_ENTRY HashTableChangeTable(PHASH_TABLE Hash, ULONG allocCount, PSINGLE_LIST_ENTRY pBuckets) {
	PSINGLE_LIST_ENTRY pOldBuckets = NULL;
	UINT32 count = RoundToPowerOfTwo(allocCount, FALSE);
	if (count > 0x4000000)
		count = 0x4000000;
	PSINGLE_LIST_ENTRY p = pBuckets;
	PSINGLE_LIST_ENTRY pEnd = &pBuckets[count];
	for (; p < pEnd; ++p) {
		p->Next = (PSINGLE_LIST_ENTRY)((ULONG_PTR)Hash | 1);
	}

	UINT64 value = MAXULONG_PTR << (Hash->BucketCount & 0x1F);
	UINT32 bucketCount = (Hash->BucketCount >> 5) & 0x7FFFFFF;
	for (UINT32 j = 0; j < bucketCount; ++j) {
		PSINGLE_LIST_ENTRY pBucket = &Hash->Buckets[j];
		while (!HashBucketLastLink(pBucket)) {
			PSINGLE_LIST_ENTRY pEntry = pBucket->Next;
			pBucket->Next = pBucket->Next->Next;
			UINT64 hashValue = ((PHASH_BUCKET)pEntry)->HashValue;
			UINT32 idx = HashTableGetBucketIndex(bucketCount, value & hashValue);
			PushEntryList(&pBuckets[idx], pEntry);
		}
	}

	pOldBuckets = Hash->Buckets;
	Hash->Buckets = pBuckets;
	Hash->BucketCount = (count << 5) | Hash->BucketCount & 0x1F;

	return pOldBuckets;
}

PSINGLE_LIST_ENTRY HashTableFindNext(PHASH_TABLE Hash, UINT64 HashValue, PSINGLE_LIST_ENTRY pLink) {
	UINT64 value = MAXULONG_PTR << (Hash->BucketCount & 0x1F);
	UINT64 k1 = value & HashValue;
	BOOL bLastLink = FALSE;
	PSINGLE_LIST_ENTRY p = pLink;
	if (pLink) {
		pLink = pLink;
		bLastLink = HashBucketLastLink(pLink);
	}
	else {
		UINT32 count = (Hash->BucketCount >> 5) & 0x7FFFFFF;
		if (count == 0)
			return NULL;
		UINT32 idx = HashTableGetBucketIndex(count, k1);
		p = &Hash->Buckets[idx];
	}

	for (bLastLink = HashBucketLastLink(p);
		!bLastLink;
		bLastLink = HashBucketLastLink(p)) {
		UINT64 k2 = value & ((PHASH_BUCKET)p->Next)->HashValue;
		if (k1 == k2) {
			return p->Next;
		}
		p = p->Next;
	}
	return NULL;
}

PHASH_TABLE HashTableGetTable(PSINGLE_LIST_ENTRY HashEntry) {
	BOOL bLastLink = FALSE;
	PHASH_TABLE pHash = NULL;
	PSINGLE_LIST_ENTRY p = HashEntry;

	for (bLastLink = HashBucketLastLink(HashEntry);
		!bLastLink;
		bLastLink = HashBucketLastLink(p)) {
		p = p->Next;
	}

	pHash = (PHASH_TABLE)((ULONG_PTR)p->Next & ~1ull);
	PSINGLE_LIST_ENTRY pEntry = NULL;
	do
	{
		UINT64 hashValue = ((PHASH_BUCKET)HashEntry)->HashValue;
		pEntry = HashTableFindNext(pHash, hashValue, pEntry);
	} while (pEntry && pEntry != HashEntry);

	return pHash;
}

PSINGLE_LIST_ENTRY HashTableCleanup(PHASH_TABLE Hash) {
	return Hash->Buckets;
}

PSINGLE_LIST_ENTRY HashTableRemoveKey(PHASH_TABLE Hash, UINT64 HashValue) {
	if (!Hash->ItemCount)
		return NULL;

	UINT64 value = MAXULONG_PTR << (Hash->BucketCount & 0x1F);
	UINT64 k1 = value & HashValue;
	UINT32 count = (Hash->BucketCount >> 5) & 0x7FFFFFF;
	UINT32 idx = HashTableGetBucketIndex(count, k1);
	PSINGLE_LIST_ENTRY p = &Hash->Buckets[idx];

	BOOL bLastLink;
	for (bLastLink = HashBucketLastLink(p); !bLastLink; bLastLink = HashBucketLastLink(p)) {
		PSINGLE_LIST_ENTRY pNext = p->Next;
		UINT64 k2 = value & ((PHASH_BUCKET)p)->HashValue;
		if (k1 == k2) {
			p->Next = pNext->Next;
			--Hash->ItemCount;
			pNext->Next = (PSINGLE_LIST_ENTRY)((ULONG_PTR)pNext->Next | 0x8000000000000002);
			return p->Next;
		}
		p = p->Next;
	}

	return NULL;
}

void HashTableIterInit(PHASH_TABLE_ITERATOR Iterator, PHASH_TABLE Hash) {
	RtlZeroMemory(Iterator, sizeof(HASH_TABLE_ITERATOR));
	Iterator->Hash = Hash;
	Iterator->Bucket = Hash->Buckets;
	Iterator->HashEntry = Iterator->Bucket;
}

PSINGLE_LIST_ENTRY HashTableIterGetNext(PHASH_TABLE_ITERATOR Iterator) {
	PSINGLE_LIST_ENTRY pLink = Iterator->HashEntry;
	UINT32 count = 0;

	if (!pLink || HashBucketLastLink(pLink)) {
		PHASH_TABLE Hash = Iterator->Hash;
		PSINGLE_LIST_ENTRY pBucket = Iterator->Bucket + 1;

		count = (Hash->BucketCount >> 5) & 0x7FFFFFF;
		PSINGLE_LIST_ENTRY pEnd = &Hash->Buckets[count];
		while (TRUE)
		{
			if (pBucket >= pEnd)
				return NULL;
			if (!HashBucketLastLink(pBucket))
				break;
			pBucket++;
		}
		PSINGLE_LIST_ENTRY pHashEntry = pBucket->Next;
		Iterator->Bucket = pBucket;
		Iterator->HashEntry = pHashEntry;
		return pHashEntry;
	}
	else {
		Iterator->HashEntry = pLink->Next;
		return Iterator->HashEntry;
	}
}

PSINGLE_LIST_ENTRY HashTableIterRemove(PHASH_TABLE_ITERATOR Iterator) {
	PSINGLE_LIST_ENTRY pHashEntry = Iterator->HashEntry;
	PSINGLE_LIST_ENTRY pBucket = Iterator->Bucket;
	BOOLEAN bLastLink = FALSE;

	for (bLastLink = HashBucketLastLink(pBucket); !bLastLink; bLastLink = HashBucketLastLink(pBucket)) {
		if (pBucket->Next == pHashEntry) {
			pBucket->Next = pHashEntry->Next;
			--Iterator->Hash->ItemCount;
			pHashEntry->Next = (PSINGLE_LIST_ENTRY)((ULONG_PTR)pHashEntry->Next | 0x8000000000000002);
			Iterator->HashEntry = pBucket;
			return pHashEntry;
		}
		pBucket = pBucket->Next;
	}

	return NULL;
}