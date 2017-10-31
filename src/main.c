#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define CACHE_CAPACITY 4		//in KB
#define VIAS 2					//2WSA cache
#define BLOCK_SIZE 32			//in bytes
#define NUMBER_OF_SETS 64
#define INSTRUCTION_SIZE 16		//in bits
#define MAIN_MEMORY_SIZE 65536	//in bytes; 64 KB

/* define needed bits for each part of address */
#define OFFSET 5
#define INDEX 6
#define TAG 5

#define CACHE_SIZE 64			//2^INDEX size of each way of the cache

struct cache_block {
	unsigned int tag:TAG;
	unsigned int validity_bit:1;
	unsigned int dirty_bit:1;
	char data;
	char last_accessed;
};

/* main and cache memory variables */
struct cache_block cache[VIAS][CACHE_SIZE][BLOCK_SIZE];
char main_memory[MAIN_MEMORY_SIZE];

/* statistics variables */
float total_accesses = 0;
float missed_accesses = 0;
double miss_rate;

void init(){
	/* set all main memory to 0 */
	memset(&main_memory, 0, MAIN_MEMORY_SIZE);
	/* set all cache blocks as invalid */
	for (int i = 0; i < VIAS; ++i){
		for (int j = 0; j < CACHE_SIZE; ++j){
			for (int k = 0; k < BLOCK_SIZE; ++k){
				cache[i][j][k].validity_bit = 0;
				cache[i][j][k].last_accessed = 'n';
			}
		}
	}
	/* set miss rate to 0 */
	miss_rate = 0;
}

void calculate_offset_index_tag(int address, int *offset, int *index, int *tag){
	/* offset */
	*offset = address << (sizeof(address) - OFFSET);
	*offset = *offset >> (sizeof(address) - OFFSET);

	/* index */
	*index = address << (sizeof(address) - INSTRUCTION_SIZE);
	*index = *index >> (sizeof(address) - INSTRUCTION_SIZE + OFFSET + TAG);

	/* tag */
	*tag = address >> (INDEX + OFFSET);
}

void update_main_memory(int cache_way, int offset, int index){
	int address = 0;
	address += cache[cache_way][index][offset].tag;
	address = address << (TAG);

	address += index;
	address = address << (INDEX);

	address += offset;
	
	main_memory[address] = cache[cache_way][index][offset].data;
}

void write_in_cache(char data_to_write, int way, int index, int offset, int tag){
	cache[way][index][offset].tag = tag;
	cache[way][index][offset].validity_bit = 1;
	/* dirty bit is 0 because data has just been read from memory, 
	 * so data in cache and in memory is the same */
	cache[way][index][offset].dirty_bit = 0;
	cache[way][index][offset].data = data_to_write;
	cache[way][index][offset].last_accessed = 'y';
}

void load_byte(int address, int offset, int index, int tag){
	//TODO refactor. Theres duplicated code in here. 
	char written = 'n';
	for (int i = 0; i < VIAS; ++i){
		if (cache[i][index][offset].validity_bit == 0){
			/* if there is no valid data here... */
			write_in_cache(main_memory[address], i, index, offset, tag);
			written = 'y';
			break;
		}
	}
	if (written == 'n'){
		for (int i = 0; i < VIAS; ++i){
			if (cache[i][index][offset].last_accessed == 'n'){
				/* if this data hasnt been last accessed... */
				/* check for previous data and save to memory */
				if (cache[i][index][offset].dirty_bit == 1){
					/* we have to update main memory */
					update_main_memory(i, offset, index);
				}
				write_in_cache(main_memory[address], i, index, offset, tag);
				break;
			}
		}
	}
}

void load_block(int address){
	address = address >> 5;
	address = address << 5;
	for (int i = 0; i < BLOCK_SIZE; ++i){
		int offset, index, tag;
		calculate_offset_index_tag(address, &offset, &index, &tag);
		load_byte(address, offset, index, tag);
        ++address;
	}
}

int get_way(int offset, int index, int tag){
	/* returns n if address is in cache; with n = way in which is present; 
	 * -1 otherwise */
	for (int i = 0; i < VIAS; ++i){
		if (cache[i][index][offset].tag == tag){
			return i;
		}
	}
	return -1;
}

void read_byte(int address){
	/* calculate offset, index, tag */
	int offset, index, tag, way;
	calculate_offset_index_tag(address, &offset, &index, &tag);

	/* read from cache */
	if ((way = get_way(offset, index, tag)) != -1){
		printf("%c\n", cache[way][index][offset].data);
	} else {
		/* case: data is not in the cache */
		/* update missed accesses */
		++missed_accesses;
		/* return requested value */
		printf("%i\n", -1);
		/* load block */
		load_block(address);
	}
	/* adjsut miss rate */
	++total_accesses;
	miss_rate = missed_accesses/total_accesses;
}

void write_byte(int address, char value){
	/* calculate offset, index, tag */
	int offset, index, tag, way;
	calculate_offset_index_tag(address, &offset, &index, &tag);

	if ((way = get_way(offset, index, tag)) != -1){
		printf("%i\n", 0);
	} else {
		/* case: data is not in the cache */
		/* update missed accesses */
		++missed_accesses;
		/* return requested value */
		printf("%i\n", -1);
		/* load block */
		load_block(address);
		way = get_way(offset, index, tag);
	}
	write_in_cache(value, way, offset, index, tag);
	/* adjsut miss rate */
	++total_accesses;
	miss_rate = missed_accesses/total_accesses;
}

void get_miss_rate(){
	printf("Miss rate: %f.\n", miss_rate);
}

int main(int argc, char*argv[]){
	init();

	/* check number of arguments. Only arguments should 
	 * be program itself and the filepath 
	 * */
	if (argc != 2){
		exit(EXIT_FAILURE);
	}
	FILE* file = fopen(argv[1], "r");
	if (file == NULL){
		exit(EXIT_FAILURE);
	}
	/* read file by line */
	char *line = NULL;
	size_t len = 0;
	while ((getline(&line, &len, file)) != -1){
		char *command;
		if ((sscanf(line, "%s", command)) == -1){
			exit(EXIT_FAILURE);	
		}
		if (strcmp("R", command) == 0){
			int address =  0;
			if ((sscanf(line, "%c%i", command, &address)) == -1){
				exit(EXIT_FAILURE);
			}
			read_byte(address);
		} else if (strcmp("W", command) == 0){
			int address = 0;
			char char_to_write = 'a';
			sscanf(line, "%c%i,%c", command, &address, &char_to_write);
			write_byte(address, char_to_write);
		} else if (strcmp("MR", command) == 0 ) {
			get_miss_rate();
		} else {
			printf("* * * * Warning! Invalid command received! * * * *\n");
		}
	}
    free(line);
	if (fclose(file) != 0){
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}

