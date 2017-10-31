#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define CACHE_CAPACITY 4		//in KB
#define BLOCK_SIZE 32			//in bytes
#define NUMBER_OF_SETS 64
#define INSTRUCTION_SIZE 16		//in bits
#define MAIN_MEMORY_SIZE 65536	//in bytes; 64 KB

/* define needed bits for each part of address */
#define OFFSET 5
#define INDEX 6
#define TAG 5

#define CACHE_SIZE 32			//2^OFFSET

struct cache_block {
	unsigned int tag:TAG;
	unsigned int validity_bit:1;
	unsigned int dirty_bit:1;
	char data;
};

/* main and cache memory variables */
struct cache_block cache[CACHE_SIZE][BLOCK_SIZE];
char main_memory[MAIN_MEMORY_SIZE];

/* statistics variables */
unsigned int total_accesses = 0;
unsigned int missed_accesses = 0;
double miss_rate;

void init(){
	/* set all main memory to 0 */
	memset(&main_memory, 0, MAIN_MEMORY_SIZE);
	/* set all cache blocks as invalid */
	for (int i = 0; i < CACHE_SIZE; ++i){
		for (int j = 0; j < BLOCK_SIZE; ++j){
			cache[i][j].validity_bit = 0;
		}
	}
	/* set miss rate to 0 */
	miss_rate = 0;
}

void calculate_offset_index_tag(int address, int *offset, int *index, int *tag){
	/* offset */
	*offset = (int)pow(2, OFFSET) & address;

	/* index */
	*index = pow(2, INDEX);
	*index = *index << OFFSET;

	/* tag */
	*tag = pow(2, TAG);
	*tag = *tag << (OFFSET + INDEX);
}

void read_byte(int address){
	/* calculate offset, index, tag */
	int offset, set_index, tag;
	calculate_offset_index_tag(address, &offset, &set_index, &tag);

	/* read from cache */
	if (cache[offset][set_index].tag != tag){
		/* case: data is not in the cache */
		/* */
		++missed_accesses;
		/* return requested value */
		printf("%i", -1);
		/* load block */
		cache[offset][set_index].tag = tag;
		cache[offset][set_index].validity_bit = 1;
		/* dirty bit is 0 because data has just been read from memory, 
		 * so data in cache and in memory is the same */
		cache[offset][set_index].dirty_bit = 0;
		cache[offset][set_index].data = main_memory[address];
	} else {
		printf("%c", cache[offset][set_index].data);
	}
	/* adjsut miss rate */
	++total_accesses;
}

void write_byte(int address, char value){
	/* calculate offset, index, tag */
	int offset, set_index, tag;
	calculate_offset_index_tag(address, &offset, &set_index, &tag);

	
}

void get_miss_rate(){
	printf("Miss rate: %f.", miss_rate);
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
	size_t *len = 0;
	while ((getline(&line, len, file)) != -1){
		char *command = "a";
		if ((scanf(line, "%c", command)) == -1){
			exit(EXIT_FAILURE);	
		}
		if (strcmp("R", command) == 0){
			int address =  0;
			if ((scanf(line, "%c%i", command, address)) == -1){
				exit(EXIT_FAILURE);
			}
			read_byte(address);
		} else if (strcmp("W", command) == 0){
			int address = 0;
			char char_to_write = 'a';
			scanf(line, "%c%i,%c", command, address, char_to_write);
			write_byte(address, char_to_write);
		} else if (strcmp("MR", command) == 0 ) {
			get_miss_rate();
		} else {
			printf("* * * * Warning! Invalid command received! * * * *");
		}
	}
	if (fclose(file) != 0){
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}

