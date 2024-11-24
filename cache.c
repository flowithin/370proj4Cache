/*
 * EECS 370, University of Michigan
 * Project 4: LC-2K Cache Simulator
 * Instructions are found in the project spec.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define DEBUG
#define MAX_CACHE_SIZE 256
#define MAX_BLOCK_SIZE 256

// **Note** this is a preprocessor macro. This is not the same as a function.
// Powers of 2 have exactly one 1 and the rest 0's, and 0 isn't a power of 2.
#define is_power_of_2(val) (val && !(val & (val - 1)))


/*
 * Accesses 1 word of memory.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads and 1 for writes.
 * write_data is a word, and is only valid if write_flag is 1.
 * If write flag is 1, mem_access does: state.mem[addr] = write_data.
 * The return of mem_access is state.mem[addr].
 */
extern int mem_access(int addr, int write_flag, int write_data);

/*
 * Returns the number of times mem_access has been called.
 */
extern int get_num_mem_accesses(void);

//Use this when calling printAction. Do not modify the enumerated type below.
enum actionType
{
    cacheToProcessor,
    processorToCache,
    memoryToCache,
    cacheToMemory,
    cacheToNowhere
};

/* You may add or remove variables from these structs */
typedef struct blockStruct
{
    int data[MAX_BLOCK_SIZE];
    int dirty;
    int lruLabel;
    int tag;
    int valid;
} blockStruct;

typedef struct cacheStruct
{
    blockStruct blocks[MAX_CACHE_SIZE];
    int blockSize;
    int numSets;
    int blocksPerSet;
    // add any variables for end-of-run stats
} cacheStruct;

/* Global Cache variable */
cacheStruct cache;
int miss;
int hits;
void printAction(int, int, enum actionType);
void printCache(void);

/*
 * Set up the cache with given command line parameters. This is
 * called once in main(). You must implement this function.
 */
void cache_init(int blockSize, int numSets, int blocksPerSet)
{
    if (blockSize <= 0 || numSets <= 0 || blocksPerSet <= 0) {
        printf("error: input parameters must be positive numbers\n");
        exit(1);
    }
    if (blocksPerSet * numSets > MAX_CACHE_SIZE) {
        printf("error: cache must be no larger than %d blocks\n", MAX_CACHE_SIZE);
        exit(1);
    }
    if (blockSize > MAX_BLOCK_SIZE) {
        printf("error: blocks must be no larger than %d words\n", MAX_BLOCK_SIZE);
        exit(1);
    }
    if (!is_power_of_2(blockSize)) {
        printf("warning: blockSize %d is not a power of 2\n", blockSize);
    }
    if (!is_power_of_2(numSets)) {
        printf("warning: numSets %d is not a power of 2\n", numSets);
    }
    printf("Simulating a cache with %d total lines; each line has %d words\n",
        numSets * blocksPerSet, blockSize);
    printf("Each set in the cache contains %d lines; there are %d sets\n",
        blocksPerSet, numSets);

    /********************* Initialize Cache *********************/
  cache.numSets = numSets;
  cache.blockSize = blockSize;
  cache.blocksPerSet = blocksPerSet;
  /*int cacheTotSize = numSets * blocksPerSet * blockSize;*/
  for(int j = 0; j < numSets; j++)
  {
    for (int i = 0; i < blocksPerSet; i++)
    {
      cache.blocks[j*blocksPerSet + i].valid = 0;
      cache.blocks[j*blocksPerSet + i].tag = 0;
      cache.blocks[j*blocksPerSet + i].dirty = 0;
      cache.blocks[j*blocksPerSet + i].lruLabel = i;//follwing the lru scheme in slides
    }

  
  }

    return;
}

/*
 * Access the cache. This is the main part of the project,
 * and should call printAction as is appropriate.
 * It should only call mem_access when absolutely necessary.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads (fetch/lw) and 1 for writes (sw).
 * write_data is a word, and is only valid if write_flag is 1.
 * The return of mem_access is undefined if write_flag is 1.
 * Thus the return of cache_access is undefined if write_flag is 1.
 */
int cache_access(int addr, int write_flag, int write_data)
{
  printf("cache_access called\n");
    /* The next line is a placeholder to connect the simulator to
    memory with no cache. You will remove this line and implement
    a cache which interfaces between the simulator and memory. */
  /*  if write  */
  /*enum actionType ac = */
  int blockaSetBitsize = ceil(log2(cache.blockSize * cache.numSets));
  int blockOffSetBitsize = log2(cache.blockSize); 
  int setAddr = (addr >> (blockOffSetBitsize)) & (cache.numSets-1);//which set it is in
  int blockAddr = setAddr * cache.blocksPerSet;//the starting address of the set(in block)
  int blockOffset = addr & (cache.blockSize-1);
  int blockAddrMem = addr - blockOffset;//the starting address of the memory block(chunck)
  int Tag = addr >> blockaSetBitsize;
  int minLRU=cache.blocksPerSet;
  int Posi;
  #ifndef DEBUG
  printf("%d %d %d %d %d %d %d %d \n",
    addr,
    blockaSetBitsize, 
  /*int blockOffSetBitsize ze); */
   setAddr, 
   blockAddr, 
   blockOffset ,
   blockAddrMem ,
   Tag ,
   minLRU);
  #endif

  if(write_flag){
    //check lru
    for(int i =blockAddr; i < blockAddr + cache.blocksPerSet; i++)
    {
      if(cache.blocks[i].lruLabel < minLRU)
      {
        minLRU = cache.blocks[i].lruLabel;
        Posi = i;
      }
    }
    /*printf("posi = %d", Posi);*/
    //check dirty bit, write to mem
    if(cache.blocks[Posi].valid && cache.blocks[Posi].dirty){
      for(int i=0; i < cache.blockSize; i++)
        mem_access(addr, 1, cache.blocks[Posi].data[i]);
      printAction(Posi, cache.blockSize, cacheToMemory);
    }
    //update cache, set dirty to 1
    cache.blocks[Posi].tag = Tag;
    cache.blocks[Posi].dirty = 1;
    cache.blocks[Posi].valid = 1;
    //bring the chunk of data writing to from mem
    /*printf("memtocache\n");*/
    printAction(blockAddrMem, cache.blockSize, memoryToCache);
    for(int i=0; i < cache.blockSize; i++){
      cache.blocks[Posi].data[i] = mem_access(blockAddrMem + i, 0, write_data);
    }
    cache.blocks[Posi].data[blockOffset] = write_data;
    printAction(addr, 1, processorToCache);
    //update all the lru in the blocks
    for(int i = 0; i < cache.blocksPerSet; i ++)
    {
      cache.blocks[blockAddr + i].lruLabel--;
    }
    cache.blocks[Posi].lruLabel = cache.blocksPerSet-1;
    printCache();
  } else {
    printf("read\n");
    //read process
    //check if cache has it. check tag in set
    for(int i=0; i < cache.blocksPerSet; i++){
      if(cache.blocks[blockAddr + i].tag == Tag && cache.blocks[blockAddr + i].valid){
        printAction(addr, 1, cacheToProcessor);
        //lru change
        for(int j=0; j < cache.blocksPerSet; j++)
        {
          cache.blocks[blockAddr+j].lruLabel--;
        }
        cache.blocks[blockAddr+i].lruLabel = cache.blocksPerSet-1;
        hits ++ ;
        return cache.blocks[blockAddr + i].data[blockOffset];
      }
    }
    //didn't find it
    //get the lru one
    #ifndef DEBUG
    printf("didn't found it\n");
    #endif
    miss++;
    minLRU = cache.blocksPerSet;
    for(int i =blockAddr; i < blockAddr + cache.blocksPerSet; i++)
      {
      /*printf("lru = %d", cache.blocks[i].lruLabel);*/
      if(cache.blocks[i].lruLabel < minLRU)
      {
        minLRU = cache.blocks[i].lruLabel;
        Posi = i;
      }
    }
    int PosiMem = (cache.blocks[Posi].tag << blockaSetBitsize) + (setAddr << blockOffSetBitsize); //position of the write back data in memory
    //before writing, write back
    if(cache.blocks[Posi].dirty == 1)
    {
      printAction(PosiMem, cache.blockSize, cacheToMemory);
      for(int i =0 ;i < cache.blockSize; i++)
      {
         mem_access(cache.blocks[Posi].data[i], 1, 0);
      }  
    } else if(cache.blocks[Posi].valid) 
        printAction(PosiMem, cache.blockSize, cacheToNowhere);

    //write from mem to cache
    printAction(blockAddrMem, cache.blockSize,memoryToCache);
    for(int i =0 ;i < cache.blockSize; i++)
    {
      cache.blocks[Posi].data[i] = mem_access(blockAddrMem+i, 0, 0);
    }
    for(int j=0; j < cache.blocksPerSet; j++)
    {
      cache.blocks[blockAddr+j].lruLabel--;
    }
    cache.blocks[Posi].valid = 1;
    cache.blocks[Posi].tag = Tag;
    cache.blocks[Posi].lruLabel = cache.blocksPerSet - 1;
    cache.blocks[Posi].dirty = 0;
    //now from caceh to processor
    printAction(addr, 1, cacheToProcessor);
    return cache.blocks[Posi].data[blockOffset];
  }
  return 0;
}


/*
 * print end of run statistics like in the spec. **This is not required**,
 * but is very helpful in debugging.
 * This should be called once a halt is reached.
 * DO NOT delete this function, or else it won't compile.
 * DO NOT print $$$ in this function
 */
void printStats(void)
{
    printCache();
    printf("End of run statistics:\n");
    printf("num_mem_accees : %d",get_num_mem_accesses());
    printf("miss : %d, hits: %d",miss,hits);
    
    return;
}

/*
 * Log the specifics of each cache action.
 *
 *DO NOT modify the content below.
 * address is the starting word address of the range of data being transferred.
 * size is the size of the range of data being transferred.
 * type specifies the source and destination of the data being transferred.
 *  -    cacheToProcessor: reading data from the cache to the processor
 *  -    processorToCache: writing data from the processor to the cache
 *  -    memoryToCache: reading data from the memory to the cache
 *  -    cacheToMemory: evicting cache data and writing it to the memory
 *  -    cacheToNowhere: evicting cache data and throwing it away
 */
void printAction(int address, int size, enum actionType type)
{
    printf("$$$ transferring word [%d-%d] ", address, address + size - 1);

    if (type == cacheToProcessor) {
        printf("from the cache to the processor\n");
    }
    else if (type == processorToCache) {
        printf("from the processor to the cache\n");
    }
    else if (type == memoryToCache) {
        printf("from the memory to the cache\n");
    }
    else if (type == cacheToMemory) {
        printf("from the cache to the memory\n");
    }
    else if (type == cacheToNowhere) {
        printf("from the cache to nowhere\n");
    }
    else {
        printf("Error: unrecognized action\n");
        exit(1);
    }

}

/*
 * Prints the cache based on the configurations of the struct
 * This is for debugging only and is not graded, so you may
 * modify it, but that is not recommended.
 */
void printCache(void)
{
    int blockIdx;
    int decimalDigitsForWaysInSet = (cache.blocksPerSet == 1) ? 1 : (int)ceil(log10((double)cache.blocksPerSet));
    printf("\ncache:\n");
    for (int set = 0; set < cache.numSets; ++set) {
        printf("\tset %i:\n", set);
        for (int block = 0; block < cache.blocksPerSet; ++block) {
            blockIdx = set * cache.blocksPerSet + block;
            if(cache.blocks[set * cache.blocksPerSet + block].valid) {
                printf("\t\t[ %0*i ] : ( V:T | D:%c | LRU:%-*i | T:%i )\n\t\t%*s{",
                    decimalDigitsForWaysInSet, block,
                    (cache.blocks[blockIdx].dirty) ? 'T' : 'F',
                    decimalDigitsForWaysInSet, cache.blocks[blockIdx].lruLabel,
                    cache.blocks[blockIdx].tag,
                    7+decimalDigitsForWaysInSet, "");
                for (int index = 0; index < cache.blockSize; ++index) {
                    printf(" 0x%08X", cache.blocks[blockIdx].data[index]);
                }
                printf(" }\n");
            }
            else {
                printf("\t\t[ %0*i ] : (V:F)\n\t\t%*s{  }\n", decimalDigitsForWaysInSet, block, 7+decimalDigitsForWaysInSet, "");
            }
        }
    }
    printf("end cache\n");
}
