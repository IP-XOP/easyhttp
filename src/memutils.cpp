#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace std;

//reallocation of memory utility
/**
initialisation:
	
	struct MemoryStruct chunk;
	chunk.memory=NULL; 
	chunk.size = 0;    
*/

/** usage:
	WriteMemoryCallback(src_ptr, sizeof(char), numchar, &chunk);
	if(chunk.memory == NULL){
		error("Mem allocation failed\n"); 
		continue; 
	}
*/

/**
	if(chunk.memory!=NULL){
		free(chunk.memory);
		chunk.size=0;
		chunk.memory = NULL;
	}
*/

struct MemoryStruct {
	char *memory;
	size_t size;
};
typedef struct MemoryStruct MemoryStruct;

void *myrealloc(void *src_ptr, size_t size)
{
    /* There might be a realloc() out there that doesn't like reallocing
	NULL pointers, so we take care of it here */
    if(src_ptr)
		return realloc(src_ptr, size);
    else
		return malloc(size);
}

size_t
WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)data;
	
    mem->memory = (char *)myrealloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory) {
		memcpy(&(mem->memory[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
    }
    return realsize;
}