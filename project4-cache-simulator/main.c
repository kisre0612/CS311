#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>

#define BYTES_PER_WORD 4

struct cache {
    uint32_t valid;
    uint32_t dirty;
    uint32_t info;
};

struct cache_list {
	struct cache_list *pnode;
	struct cache_list *fnode;
	uint32_t way;
};

// cache stat variables
uint32_t total_reads;
uint32_t total_writes;
uint32_t write_backs;
uint32_t reads_hits;
uint32_t write_hits;
uint32_t reads_misses;
uint32_t write_misses;

// global variables
uint32_t capacity;
uint32_t way;
uint32_t blocksize;
uint32_t indexsize;
uint32_t set;
uint32_t words;
uint32_t addr;
int opt_c;		// command option: -c => Cache configuration specification
int opt_x;		// command option: -x => Dump the cache content at the end of simulation

/***************************************************************/
/*                                                             */
/* Procedure : cdump                                           */
/*                                                             */
/* Purpose   : Dump cache configuration                        */   
/*                                                             */
/***************************************************************/
void cdump(int capacity, int assoc, int blocksize){

	printf("Cache Configuration:\n");
    	printf("-------------------------------------\n");
	printf("Capacity: %dB\n", capacity);
	printf("Associativity: %dway\n", assoc);
	printf("Block Size: %dB\n", blocksize);
	printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : sdump                                           */
/*                                                             */
/* Purpose   : Dump cache stat		                       */   
/*                                                             */
/***************************************************************/
void sdump(int total_reads, int total_writes, int write_backs,
	int reads_hits, int write_hits, int reads_misses, int write_misses) {
	printf("Cache Stat:\n");
    	printf("-------------------------------------\n");
	printf("Total reads: %d\n", total_reads);
	printf("Total writes: %d\n", total_writes);
	printf("Write-backs: %d\n", write_backs);
	printf("Read hits: %d\n", reads_hits);
	printf("Write hits: %d\n", write_hits);
	printf("Read misses: %d\n", reads_misses);
	printf("Write misses: %d\n", write_misses);
	printf("\n");
}


/***************************************************************/
/*                                                             */
/* Procedure : xdump                                           */
/*                                                             */
/* Purpose   : Dump current cache state                        */ 
/* 							       */
/* Cache Design						       */
/*  							       */
/* 	    cache[set][assoc][word per block]		       */
/*      						       */
/*      						       */
/*       ----------------------------------------	       */
/*       I        I  way0  I  way1  I  way2  I                 */
/*       ----------------------------------------              */
/*       I        I  word0 I  word0 I  word0 I                 */
/*       I  set0  I  word1 I  word1 I  work1 I                 */
/*       I        I  word2 I  word2 I  word2 I                 */
/*       I        I  word3 I  word3 I  word3 I                 */
/*       ----------------------------------------              */
/*       I        I  word0 I  word0 I  word0 I                 */
/*       I  set1  I  word1 I  word1 I  work1 I                 */
/*       I        I  word2 I  word2 I  word2 I                 */
/*       I        I  word3 I  word3 I  word3 I                 */
/*       ----------------------------------------              */
/*      						       */
/*                                                             */
/***************************************************************/
void xdump(int set, int way, struct cache **cache)
{
	int i,j,k = 0;

	printf("Cache Content:\n");
    	printf("-------------------------------------\n");
	for(i = 0; i < way;i++)
	{
		if(i == 0)
		{
			printf("    ");
		}
		printf("      WAY[%d]",i);
	}
	printf("\n");

	for(i = 0 ; i < set;i++)
	{
		printf("SET[%d]:   ",i);
		for(j = 0; j < way;j++)
		{
			if(k != 0 && j == 0)
			{
				printf("          ");
			}
			printf("0x%08x  ", cache[i][j].info);
		}
		printf("\n");
	}
	printf("\n");
}

// Basic Command : ./cs311cache -c capacity:associativity:block_size [-x] <input trace file>
// Capacity: 4B (one word) ~ 8KB, Associativity: 1 way ~ 16 ways, Block size: 4B ~ 32B
// [Tag Bit | Index Bit | Block Offset(0)]

void read_command(int argc, char *argv[]) {
	int opt;
	while((opt = getopt(argc, argv, "c:x")) != -1) {
		switch(opt) {
			case 'c':
				opt_c = 1;
				sscanf(optarg, "%d:%d:%d", &capacity, &way, &blocksize);
				set = capacity/way/blocksize;
				words = blocksize/BYTES_PER_WORD;
				indexsize = 0;
				for(int x = set >> 1; x > 0; x = x >> 1) indexsize++;
				break;
			case 'x':
				opt_x = 1;
				break;
			case '?':
				exit(0);
		}
	}
} 

void allocate_cache(struct cache **cache, struct cache_list **c_list) {
	for(int i = 0; i < set; i++) {
		struct cache_list *c_entry, *p_entry = NULL;
		c_entry = (struct cache_list*) malloc(sizeof(struct cache_list));
		c_entry->pnode = p_entry;
		c_entry->way = 0;
		c_list[i] = c_entry;
		for (int j = 0; j < way; j++) {
            c_entry->fnode = (struct cache_list*) malloc(sizeof(struct cache_list));
            p_entry = c_entry;
            c_entry = c_entry->fnode;
            c_entry->way = j;
            c_entry->pnode = p_entry;
        }
        c_entry->fnode = NULL;
	}
}

void readfile(struct cache **cache, struct cache_list **list, FILE *f) {
	char fbuffer[20], op_type;
	while(fgets(fbuffer, 20, f)) {
		struct cache *result;
		struct cache_list *item;
		uint32_t cache_info; 
		int index = 0, hit = 0;
		int op_R = 0, op_W = 0;		// op_type type: R(read), W(write)

		sscanf(fbuffer, "%c %x", &op_type, &addr);
		if(op_type == 'R') op_R = 1;
		if(op_type == 'W') op_W = 1;
		index = (addr/blocksize) & ((1 << indexsize) - 1);
		cache_info = addr & ~(blocksize - 1);

		for(int i = 0; i < way; i++) {
			// if cache hit happens, move it to cache head (using double linked list)
			if(cache[index][i].valid) {
				if(cache[index][i].info == cache_info) {
					hit = 1;
					result = &cache[index][i];
					for(item = list[index]; ; item = item->fnode) if(item->way == i) break;
					if(item->pnode) {
						if(item->fnode) item->fnode->pnode = item->pnode;
						item->pnode->fnode = item->fnode;
						item->pnode = NULL;
                    	item->fnode = list[index];
						list[index]->pnode = item;
                    	list[index] = item;
					}
					if(op_R) {
						total_reads++;
						reads_hits++;
					} else if(op_W) {
						result->dirty = 1;
						total_writes++;
						write_hits++;
					} else {
						// can't be here
						exit(0);
					}
					break;
				}
			}
		}
		if(hit) continue;	// go to next line of the input file

		// if no hit occurred => miss! => find LRU index
		struct cache *LRUcache;
		struct cache_list *LRUitem = list[index];
		int LRUindex = -1;
		for(int i = 0; i < way; i++) {
			if(!cache[index][i].valid) {
				LRUcache = &cache[index][i];
				LRUindex = i;
				for(LRUitem = list[index]; ; LRUitem = LRUitem->fnode) if(LRUitem->way == i) break;
				break;
			}
		}
		if(LRUindex < 0) {	// LRUindex == -1 => no invalid node at its index
			for(int i = 0; i < way - 1; i++) LRUitem = LRUitem->fnode;
			LRUcache = &cache[index][LRUitem->way];
			LRUindex = LRUitem->way;
		}
		if(LRUcache->dirty) write_backs++;
		LRUcache->info = cache_info;
		LRUcache->valid = 1;
		LRUcache->dirty = 0;
		result = LRUcache;
		if(LRUitem->pnode) {	// using double linked list for LRUitem
			if(LRUitem->fnode) LRUitem->fnode->pnode = LRUitem->pnode;
			LRUitem->pnode->fnode = LRUitem->fnode;
			LRUitem->pnode = NULL;
            LRUitem->fnode = list[index];
			list[index]->pnode = LRUitem;
            list[index] = LRUitem;
		}
		if(op_R) {
			total_reads++;
			reads_misses++;
		} else if(op_W) {
			result->dirty = 1;
			total_writes++;
			write_misses++;
		} else {
			// can't be here
			exit(0);
		}
    }
}

/*
	The write policy of the cache must be write-allocate and write-back. The replacement policy must be the perfect LRU.

	Cache content is not a data content, but the address which is aligned with block size.
	Ex) Assume block size is 16B. When the address 0x10001234 is stored in cache,
	the content of the cache entry is 0x10001230 since block size bits (4 bits) are masked by 0.
	[Tag Bit | Index Bit | Block Offset(0)]
*/

int main(int argc, char *argv[]) {                              
	struct cache **cache;
	struct cache_list **c_list;
	int i, j, k;

	read_command(argc, argv);
	
	// cache allocate & initialize
	cache = (struct cache**) malloc(sizeof(struct cache*) * set);
	for(int i = 0; i < set; i++) {
		cache[i] = (struct cache*) malloc(sizeof(struct cache) * way);
		for(int j = 0; j < way; j++) {
			cache[i][j].info = 0;
			cache[i][j].valid = 0;
			cache[i][j].dirty = 0;
		}
	}
	c_list = (struct cache_list**) malloc(sizeof(struct cache_list*) * set);
	allocate_cache(cache, c_list);
	
	// readfile
	FILE *f = fopen(argv[optind], "r");
	readfile(cache, c_list, f);

    // test
    cdump(capacity, way, blocksize);
    sdump(total_reads, total_writes, write_backs, reads_hits, write_hits, reads_misses, write_misses); 
    if(opt_x) xdump(set, way, cache);

    return 0;
}
