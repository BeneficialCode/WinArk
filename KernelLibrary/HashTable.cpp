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

UINT32 HashTableInsert(PHASH_TABLE Hash, PHASH_BUCKET HashEntry) {
	UINT32 count = (Hash->BucketCount >> 5) & 0x7FFFFFF;
	UINT64 key = (-1ull << (Hash->BucketCount & 0x1F)) & HashEntry->HashValue;
	UINT32 idx = HashTableGetBucketIndex(count, key);
	PushEntryList(&Hash->Buckets[idx].Entry, &HashEntry->Entry);
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
	UINT32 BucketCount, PHASH_BUCKET Buckets) {
	UINT32 count = RoundToPowerOfTwo(BucketCount, FALSE);
	if (count > 0x4000000)
		count = 0x4000000;
	Hash->ItemCount = 0;
	Hash->BucketCount = (count << 5) | Hash->BucketCount & 0x1F;
	Hash->Buckets = Buckets;
	Hash->BucketCount = Flags & 0x1F | Hash->BucketCount & 0xFFFFFFE0;
	PHASH_BUCKET p = Buckets;
	PHASH_BUCKET pEnd = &Buckets[count];
	if (p) {
		while (p < pEnd) {
			p->Hash = ((ULONG_PTR)Hash | 1);
			p++;
		}
	}
}

// Returns true if the given number is a power of 2.
BOOLEAN IsPowerOfTwo(UINT32 x) {
	return (x > 0) && ((x & (x - 1)) == 0);
}

UINT32 RoundToPowerOfTwo(UINT32 value, BOOLEAN roundUpToNext){
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
	return Hash->Buckets;
}

PHASH_BUCKET HashTableFindNext(PHASH_TABLE Hash, UINT32 Key, PHASH_BUCKET Bucket) {
	UINT64 value = -1ull << (Hash->BucketCount & 0x1F);
	UINT32 k = value & Key;
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
		pBucket = &Hash->Buckets[idx];
	}
	for (bLastLink = HashBucketLastLink(pBucket); !bLastLink;
		bLastLink = HashBucketLastLink(pBucket)) {
		if (k == (value & pBucket->HashValue)) {
			return pBucket;
		}
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
		pEntry = pBucket;
	}

	pHash = (PHASH_TABLE)(pBucket->Hash & ~1ull);
	pEntry = NULL;
	do
	{
		pEntry = HashTableFindNext(pHash, HashEntry->HashValue, pEntry);
	} while (pEntry && pEntry != HashEntry);

	return pHash;
}

PHASH_TABLE HashTableChangeTable(PHASH_TABLE Hash, ULONG size,
	PHASH_BUCKET pBuckets) {
	UINT32 count = RoundToPowerOfTwo(size, FALSE);
	if (count > 0x4000000)
		count = 0x4000000;
	PHASH_BUCKET p = pBuckets;
	PHASH_BUCKET pEnd = &pBuckets[count];
	for (; p < pEnd; ++p) {
		p->Hash = (ULONG_PTR)Hash | 1;
	}
	
	UINT64 value = -1ull << (Hash->BucketCount & 0x1F);
	count = (Hash->BucketCount >> 5) & 0x7FFFFFF;
	for (UINT32 j = 0; j < count; ++j) {
		PHASH_BUCKET pBucket = &Hash->Buckets[j];
		while (!HashBucketLastLink(pBucket)) {
			p = pBucket;
			pBucket = CONTAINING_RECORD(pBucket->Entry.Next, HASH_BUCKET, Entry);
			UINT32 idx = HashTableGetBucketIndex(count, value & p->HashValue);
			PushEntryList(&pBuckets[idx].Entry, &p->Entry);
		}
	}
	pBuckets = Hash->Buckets;
	Hash->Buckets = pBuckets;
	Hash->BucketCount = (count >> 32) | Hash->BucketCount & 0x1F;

	return Hash;
}

PHASH_TABLE_ITERATOR HashTableIterInit(PHASH_TABLE_ITERATOR Iterator,
	PHASH_TABLE Hash) {
	RtlZeroMemory(Iterator, sizeof(HASH_TABLE_ITERATOR));
	Iterator->Hash = Hash;
	Iterator->Bucket = Hash->Buckets;
	Iterator->HashEntry = Iterator->Bucket;
	return Iterator;
}

PHASH_BUCKET HashTableIterGetNext(PHASH_TABLE_ITERATOR Iterator) {
	PHASH_BUCKET HashEntry = Iterator->HashEntry;
	UINT32 count = 0;
	if (!HashEntry || HashBucketLastLink(HashEntry)) {
		PHASH_TABLE Hash = Iterator->Hash;
		
		UINT32 count = (Hash->BucketCount >> 5) & 0x7FFFFFF;
		PHASH_BUCKET pBucket = Iterator->Bucket;
		PHASH_BUCKET pEnd = &Hash->Buckets[count];
		while (TRUE)
		{
			if (pBucket >= pEnd)
				return NULL;
			if (!HashBucketLastLink(pBucket))
				break;
			pBucket = CONTAINING_RECORD(pBucket->Entry.Next, HASH_BUCKET, Entry);
		}
		PHASH_BUCKET pHashEntry = CONTAINING_RECORD(pBucket->Entry.Next, HASH_BUCKET, Entry);
		count = (Iterator->Bucket - Iterator->Hash->Buckets) >> 3;
		Iterator->Bucket = pBucket;
		Iterator->HashEntry = pHashEntry;
		return pHashEntry;
	}
	else {
		count = (Iterator->Bucket - Iterator->Hash->Buckets) >> 3;
		return Iterator->HashEntry;
	}
}

PHASH_BUCKET MpHashTableIterRemove(PHASH_TABLE_ITERATOR Iterator) {
	PHASH_BUCKET pHashEntry;
	pHashEntry = Iterator->HashEntry;
	PHASH_BUCKET pBucket = Iterator->Bucket;
	BOOLEAN bLastLink = FALSE;

	for (bLastLink = HashBucketLastLink(pBucket); !bLastLink;
		bLastLink = HashBucketLastLink(pBucket)) {
		PHASH_BUCKET p = CONTAINING_RECORD(pBucket->Entry.Next, HASH_BUCKET, Entry);
		if (p == pHashEntry) {
			--Iterator->Hash->ItemCount;
			p->Entry.Next = pHashEntry->Entry.Next;
			pHashEntry->Entry.Next = (PSINGLE_LIST_ENTRY)((ULONG_PTR)pHashEntry->Entry.Next | 0x8000000000000002);
			Iterator->HashEntry = p;
			return pHashEntry;
		}
	}

	return NULL;
}