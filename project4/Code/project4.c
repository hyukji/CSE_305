#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


unsigned long long strTollong(char* str) {
	char* hex = "Xx";
	return (strchr(hex, str[1])) ? (unsigned long long)strtol(str, NULL, 16) : atoll(str);
}

int getMissRate(int miss, int access) {
	if(!access) { return 0; }
	return miss * 100 / access;
}

int getPower(long value){
	int power = 0;
	while (value % 2 == 0 && value > 1) {
		value /= 2;
		power++;
	}
	
	return power;
}


struct CacheData {
	int capacity;
	int way;
	int block_size;
	
	long index;
	int index_bit;
	
	int byte_offset_bit;
	
	int blank;
};


struct CacheBlock {
	int valid;
	int isDirty;
	int access;
	unsigned long long tag;
	unsigned long long addr;
	
};

int main(int argc, char* argv[]) {
	struct CacheData L1_data, L2_data;
	
	int _lru;
	
	if(!strcmp(argv[7], "-lru")) { 
		_lru = 1;
	} else if(!strcmp(argv[7], "-random")) { 
		_lru = 0; 
	} else {
		printf("check lru option\n");
		exit(1);
	}
	
	if(strcmp(argv[1], "-c") | strcmp(argv[3], "-a") | strcmp(argv[5], "-b")) {
		printf("check option\n");
		exit(1);
	} 
	
	L2_data.capacity = atoi(argv[2]);
	L2_data.way = atoi(argv[4]);
	L2_data.block_size = atoi(argv[6]);
	
	
	L2_data.index = L2_data.capacity * 1024 / (L2_data.way * L2_data.block_size);
	L2_data.index_bit = getPower(L2_data.index);
	L2_data.byte_offset_bit = getPower(L2_data.block_size);
	
	L1_data.capacity = L2_data.capacity / 4;
	L1_data.way = (L2_data.way <= 2) ? L2_data.way : (L2_data.way / 4);
	L1_data.block_size = L2_data.block_size;
	L1_data.index = L1_data.capacity * 1024 / (L1_data.way * L1_data.block_size);
	L1_data.index_bit = getPower(L1_data.index);
	L1_data.byte_offset_bit = getPower(L1_data.block_size);

	struct CacheBlock** L1 = (struct CacheBlock**)malloc(L1_data.index * sizeof(struct CacheBlock*));
	struct CacheBlock** L2 = (struct CacheBlock**)malloc(L2_data.index * sizeof(struct CacheBlock*));
	for (long i = 0; i < L1_data.index; i++) {
		L1[i] = (struct CacheBlock*)malloc(L1_data.way * sizeof(struct CacheBlock));
	}
	for (long i = 0; i < L2_data.index; i++) {
		L2[i] = (struct CacheBlock*)malloc(L2_data.way * sizeof(struct CacheBlock));
	}
	
	
	int total_access = 0;
	int read_access = 0;
	int write_access = 0;
	
	int L1_read_miss = 0;
	int L2_read_miss = 0;
	int L1_write_miss = 0;
	int L2_write_miss = 0;
	
	int L1_clean_evict = 0;
	int L1_dirty_evict = 0;
	int L2_clean_evict = 0;
	int L2_dirty_evict = 0;
	
	
	// load file
	FILE* fp;
	fp = fopen(argv[8], "r");

	if(fp == NULL) {
		printf("file %s is not exist!\n", argv[8]);
		exit(1);
	}
	
	char type, buffer[64];
	char* str_addr;
	unsigned long long addr, L1_tag, L2_tag;
	int L1_idx, L2_idx, hit;
	struct CacheBlock L1_target, L2_target;
	
	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		
		strtok(buffer, " \n");
		str_addr = strtok(NULL, " \n");
		addr = strTollong(str_addr);
		
		hit = 0;
		total_access++;
		if(buffer[0] == 'W') { write_access++; }
		else { read_access++; }
		

		L1_idx = (addr >> L1_data.byte_offset_bit) & (int)(pow(2, L1_data.index_bit) - 1);
		L1_tag = (addr >> (L1_data.byte_offset_bit + L1_data.index_bit));
		
		L1_data.blank = -1;
		for(int w =0; w < L1_data.way; w++) {
			L1_target = L1[L1_idx][w];
			if(!L1_target.valid) { 
				L1_data.blank = w; 
				continue; 
			}
			if(L1_target.tag == L1_tag) { 
				// L1 hit
				hit = 1;
				if(buffer[0] == 'W') { 
					L1_target.isDirty = 1;
				}
				L1_target.access = total_access;
				L1[L1_idx][w] = L1_target;
				break;
			}
		}
		if(hit) { continue; }
		

		if(buffer[0] == 'W') { L1_write_miss++; }
		else { L1_read_miss++; }
		
		
		// L1 miss => L1 pop
		struct CacheBlock L1_newBlock = {1, 0, total_access, L1_tag, addr};
		struct CacheBlock L1_eviction = {0, 0, 0, 0, 0};
		if(L1_data.blank >= 0) {
			// add target to blank of L1
			L1[L1_idx][L1_data.blank] = L1_newBlock;
		}
		else if(_lru) {
			// find and delete lru before add target to L1
			int m = total_access;
			int m_w;
			for(int w = 0; w < L1_data.way; w++) {
				L1_target = L1[L1_idx][w];
				if(L1_target.valid && L1_target.access < m) { 
					m = L1_target.access;
					m_w = w; 
				}
			}
			L1_eviction = L1[L1_idx][m_w];
			if(L1_eviction.isDirty) { L1_dirty_evict++; } else { L1_clean_evict++; }
			L1[L1_idx][m_w] = L1_newBlock;
		}
		else {
			// delete random before add target to L1
			int rnd = rand() % L1_data.way;
			L1_eviction = L1[L1_idx][rnd];
			if(L1_eviction.isDirty) { L1_dirty_evict++; } else { L1_clean_evict++; }
			L1[L1_idx][rnd] = L1_newBlock;
		}
		
		// find L1_evict in L1 and update L1
		if(L1_eviction.valid & L1_eviction.isDirty) {
			int L2_evict_idx = (L1_eviction.addr >> L2_data.byte_offset_bit) & (int)(pow(2, L2_data.index_bit) - 1);
			int L2_evict_tag = (L1_eviction.addr >> (L2_data.byte_offset_bit + L2_data.index_bit));
			
			for(int w =0; w < L2_data.way; w++) {
				L2_target = L2[L2_evict_idx][w];
				if(L2_target.valid & L2_target.tag == L2_evict_tag) { 
					// find in L2
					L2[L2_evict_idx][w].isDirty = 1;
					break;
				}
			}
		}
		
		
		// L2 check
		L2_idx = (addr >> L2_data.byte_offset_bit) & (int)(pow(2, L2_data.index_bit) - 1);
		L2_tag = (addr >> (L2_data.byte_offset_bit + L2_data.index_bit));
		
		hit = 0;
		L2_data.blank = -1;
		for(int w =0; w < L2_data.way; w++) {
			L2_target = L2[L2_idx][w];
			if(!L2_target.valid) { 
				L2_data.blank = w; 
				continue; 
			}
			if(L2_target.tag == L2_tag) { 
				// hit
				hit = 1;
				if(buffer[0] == 'W') { 
					L2_target.isDirty = 1;
				}
				L2_target.access = total_access;
				L2[L2_idx][w] = L2_target;
				break;
			}
		}
		
		
		struct CacheBlock L2_newBlock = {1, 0, total_access, L2_tag, addr};
		struct CacheBlock L2_eviction = {0, 0, 0, 0, 0};
		
		// L1 miss, L2 miss 
		if(!hit) {
			if(buffer[0] == 'W') { L2_write_miss++; }
			else { L2_read_miss++; }
			
			// L2 update
			if(L2_data.blank >= 0) {
				// add target to blank of L2
				L2[L2_idx][L2_data.blank] = L2_newBlock;
			} 
			else if(_lru){
				// find and delete lru before add target to L2
				int m = total_access;
				int m_w;
				for(int w = 0; w < L2_data.way; w++) {
					L2_target = L2[L2_idx][w];
					if(L2_target.valid && (L2_target.access < m)) { 
						m = L2_target.access;
						m_w = w; 
					}
				}
				
				L2_eviction = L2[L2_idx][m_w];
				if(L2_eviction.isDirty) { L2_dirty_evict++; } else { L2_clean_evict++; }
				
				L2[L2_idx][m_w] = L2_newBlock;
			}
			else {
				// delete random before add target to L1
				int rnd = rand() % L2_data.way;
				
				L2_eviction = L2[L2_idx][rnd];
				if(L2_eviction.isDirty) { L2_dirty_evict++; } else { L2_clean_evict++; }
				
				L2[L2_idx][rnd] = L2_newBlock;
			}
				
			// find L2_evict in L1 and update L1
			if(L2_eviction.valid) {
				int L1_evict_idx = (L2_eviction.addr >> L1_data.byte_offset_bit) & (int)(pow(2, L1_data.index_bit) - 1);
				int L1_evict_tag = (L2_eviction.addr >> (L1_data.byte_offset_bit + L1_data.index_bit));
				
				for(int w =0; w < L1_data.way; w++) {
					L1_target = L1[L1_evict_idx][w];
					if(L1_target.valid & L1_target.tag == L1_evict_tag) { 
						// find L1_eviction
						if(L1[L1_evict_idx][w].isDirty) { L1_dirty_evict++; } else { L1_clean_evict++; }
						L1[L1_evict_idx][w].valid = 0;
						break;
					}
				}
			}
			
		}
		
		
	}
	
	fclose(fp);
	
	
	char out_file_name[100];
	sprintf(out_file_name, "%s_%s_%s_%s.out", strtok(argv[8], ". \n"), argv[2], argv[4], argv[6]);
	
	FILE *out_file = fopen(out_file_name, "w+");
	if (out_file == NULL) {
		printf("cant open output file\n");
		return 1;
	}

    fprintf(out_file, "-- General Stats --\n");
    fprintf(out_file, "L1 Capacity: %d\n", L1_data.capacity);
	fprintf(out_file, "L1 Way: %d\n", L1_data.way);
	
	fprintf(out_file, "L2 Capacity: %d\n", L2_data.capacity);
	fprintf(out_file, "L2 Way: %d\n", L2_data.way);
	
	fprintf(out_file, "Block Size: %d\n", L1_data.block_size);
	
	fprintf(out_file, "Total accesses: %d\n", total_access);
	fprintf(out_file, "Read accesses: %d\n", read_access);
	fprintf(out_file, "Write accesses: %d\n", write_access);
	
	fprintf(out_file, "L1 Read misses: %d\n", L1_read_miss);
	fprintf(out_file, "L2 Read misses: %d\n", L2_read_miss);
	fprintf(out_file, "L1 Write misses: %d\n", L1_write_miss);
	fprintf(out_file, "L2 Write misses: %d\n", L2_write_miss);
	
	fprintf(out_file, "L1 Read miss rate: %d%%\n", getMissRate(L1_read_miss, read_access));
	fprintf(out_file, "L2 Read miss rate: %d%%\n", getMissRate(L2_read_miss ,L1_read_miss));
	fprintf(out_file, "L1 Write miss rate: %d%%\n", getMissRate(L1_write_miss, write_access));
	fprintf(out_file, "L2 Write miss rate: %d%%\n", getMissRate(L2_write_miss, L1_write_miss));
	
	fprintf(out_file, "L1 clean eviction: %d\n", L1_clean_evict);
	fprintf(out_file, "L2 clean eviction: %d\n", L2_clean_evict);
	fprintf(out_file, "L1 dirty eviction: %d\n", L1_dirty_evict);
	fprintf(out_file, "L2 dirty eviction: %d\n", L2_dirty_evict);
	

	fclose(out_file); 
	
	return 0;
}