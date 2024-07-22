#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    int valid;
    int tag;
    int* blockData;
    int lruCounter; 
    int fifoCounter; 
    int dirty; 
} CacheBlock;

typedef struct {
    int sets;
    int associativity;
    CacheBlock** cache;
} SetAssociativeCache;

typedef struct {
    int size;
    int blocksize;
    int numBlocks;
    int associativity;
    char replacementPolicy[10];
    char writePolicy[10];
    SetAssociativeCache* cache;
} Cache;

void intitializeSetcache(SetAssociativeCache* cache, int numSets, int associativity, int blocksize) {
    cache->sets = numSets;
    cache->associativity = associativity;
    cache->cache = (CacheBlock**)malloc(numSets * sizeof(CacheBlock*));
    if (cache->cache == NULL) {
        printf("Memory allocation failed for cache->cache\n");
        exit(1);
    }

    for (int i = 0; i < numSets; ++i) {
        cache->cache[i] = (CacheBlock*)malloc(associativity * sizeof(CacheBlock));
        if (cache->cache[i] == NULL) {
            printf("Memory allocation failed for cache set %d\n", i);
            exit(1);
        }

        for (int j = 0; j < associativity; ++j) {
            cache->cache[i][j].valid = 0;
            cache->cache[i][j].tag = -1; 
            cache->cache[i][j].blockData = (int*)malloc(blocksize * sizeof(int));
            
        }
    }
}

int replaceindex(CacheBlock* cacheSet, int associativity, const char* replacementPolicy) {
    int replaceIndex = 0;

    if (strcmp(replacementPolicy, "LRU") == 0) {
        int leastUsedIndex = 0;
        for (int i = 1; i < associativity; ++i) {
            if (cacheSet[i].lruCounter < cacheSet[leastUsedIndex].lruCounter) {
                leastUsedIndex = i;
            }
        }
        replaceIndex = leastUsedIndex;
    } else if (strcmp(replacementPolicy, "FIFO") == 0) {
        int oldestIndex = 0;
        for (int i = 1; i < associativity; ++i) {
            if (cacheSet[i].fifoCounter < cacheSet[oldestIndex].fifoCounter) {
                oldestIndex = i;
            }
        }
        replaceIndex = oldestIndex;
    } else if (strcmp(replacementPolicy, "RANDOM") == 0) {
        replaceIndex = rand() % associativity;
    }

    return replaceIndex;
}



Cache* initCache(int size, int blocksize, int associativity, char* replacementPolicy, char* writePolicy) {
    Cache* cache = (Cache*)malloc(sizeof(Cache));


    if (cache == NULL) {
       
        return NULL;
    }
    cache->size = size;
    cache->blocksize = blocksize;
    cache->numBlocks = size / blocksize;
    cache->associativity = associativity;
    strcpy(cache->replacementPolicy, replacementPolicy);
    strcpy(cache->writePolicy, writePolicy);

    cache->cache = (SetAssociativeCache*)malloc(sizeof(SetAssociativeCache));
    if (cache->cache == NULL) {
       
        free(cache);
        return NULL;
    }

    intitializeSetcache(cache->cache, cache->numBlocks / associativity, associativity, blocksize);

    return cache;
}



void getAddressInfo(int address, int blocksize, int cacheSize, int associativity, int *tag, int *index) {
    int numSets;
    if (associativity == 0) {
        numSets = 1;
    } else {
        numSets = cacheSize / (blocksize * associativity);
    }

    int blockOffsetBits = log2(blocksize);
    int indexBits = log2(numSets);
    int tagBits = 32 - blockOffsetBits - indexBits;

    *index = (address >> blockOffsetBits) & ((1 << indexBits) - 1);
    *tag = (address >> (blockOffsetBits + indexBits)) & ((1 << tagBits) - 1);
}

void simulate(Cache* cache, char mode, char* addressStr) {
    int address;
    sscanf(addressStr, "%x", &address);

    int tag, index;
    getAddressInfo(address, cache->blocksize, cache->size, cache->associativity, &tag, &index);

    SetAssociativeCache* cachePtr = cache->cache;

    if (index < 0 || index >= cachePtr->sets) {
        printf("Invalid set index: %d\n", index);
        return;
    }

    int setIndex = index;
    CacheBlock* cacheSet = cachePtr->cache[setIndex];

    if (mode == 'R') {
        if (cache->associativity == 1) {
            if (cacheSet->valid && cacheSet->tag == tag) {
                printf("Address: 0x%08x, Set: 0x%x, Hit, Tag: 0x%x\n", address, index, tag);
            } else {
                printf("Address: 0x%08x, Set: 0x%x, Miss, Tag: 0x%x\n", address, index, tag);

                cacheSet->valid = 1;
                cacheSet->tag = tag;
            }
        } else {
            // Read operation
            int blockFound = 0;
            for (int i = 0; i < cache->associativity; ++i) {
                if (cacheSet[i].valid && cacheSet[i].tag == tag) {
                    printf("Address: 0x%08x, Set: 0x%x, Hit, Tag: 0x%x\n", address, index, tag);
                    blockFound = 1;
                    break;
                }
            }
           if (!blockFound) {
                printf("Address: 0x%08x, Set: 0x%x, Miss, Tag: 0x%x\n", address, index, tag);

                int replaceIndex = replaceindex(cacheSet, cache->associativity, cache->replacementPolicy);

                cacheSet[replaceIndex].valid = 1;
                cacheSet[replaceIndex].tag = tag;

               
                for (int i = 0; i < cache->blocksize; ++i) {
                    cacheSet[replaceIndex].blockData[i] = address + i;
                }
            }
        }
    }  else if(mode == 'W') {
       
        int blockFound = 0;
        for (int i = 0; i < cache->associativity; ++i) {
            if (cacheSet[i].valid && cacheSet[i].tag == tag) {
              
                blockFound = 1;

              
                for (int j = 0; j < cache->blocksize; ++j) {
                    cacheSet[i].blockData[j] = address + j; 
                }

              
                if (strcmp(cache->writePolicy, "WT") == 0 || strcmp(cache->writePolicy, "WB") == 0) {
                    cacheSet[i].dirty = (strcmp(cache->writePolicy, "WB") == 0) ? 1 : 0;
                }

                printf("Address: 0x%08x, Set: 0x%x, Hit, Tag: 0x%x\n", address, setIndex, tag);
                break;
            }
        }

        if (!blockFound) {
           
            int replaceIndex = replaceindex(cacheSet, cache->associativity, cache->replacementPolicy);
            cacheSet[replaceIndex].valid = 1;
            cacheSet[replaceIndex].tag = tag;

           
            for (int i = 0; i < cache->blocksize; ++i) {
                cacheSet[replaceIndex].blockData[i] = address + i;
            }

           
            if (strcmp(cache->writePolicy, "WT") == 0 || strcmp(cache->writePolicy, "WB") == 0) {
                cacheSet[replaceIndex].dirty = (strcmp(cache->writePolicy, "WB") == 0) ? 1 : 0;
            }

            printf("Address: 0x%08x, Set: 0x%x, Miss, Tag: 0x%x\n", address, setIndex, tag);
        }
        }
}

int main() {
    FILE *configFile = fopen("cache.config", "r");
    FILE *accessFile = fopen("cache.access", "r");

   
    int cacheSize, blockSize, associativity;
    char replacementPolicy[10], writePolicy[10];

    fscanf(configFile, "%d", &cacheSize);
    fscanf(configFile, "%d", &blockSize);
    fscanf(configFile, "%d", &associativity);
    fscanf(configFile, "%s", replacementPolicy);
    fscanf(configFile, "%s", writePolicy);

     if(associativity==0) {associativity=cacheSize/blockSize;
    }

    
    Cache* cache = initCache(cacheSize, blockSize, associativity, replacementPolicy, writePolicy);

    
       char mode;
    char addressStr[10];
    while (fscanf(accessFile, " %c: %s", &mode, addressStr) != EOF) {
        simulate(cache, mode, addressStr);
    }

   
    fclose(configFile);
    fclose(accessFile);
   
    for (int i = 0; i < cache->cache->sets; i++) {
        for (int j = 0; j < cache->associativity; j++) {
            free(cache->cache->cache[i][j].blockData);
        }
        free(cache->cache->cache[i]);
    }
    free(cache->cache->cache);
    free(cache->cache);
    free(cache);

    return 0;
}