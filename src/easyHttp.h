
#include "XOPStandardHeaders.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#define MAX_URL_LENGTH 2048
#define MAX_PASSLEN 200

#define REQUIRES_IGOR_500 1 + FIRST_XOP_ERR

/* Prototypes */
HOST_IMPORT void main(IORecHandle ioRecHandle);
static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data);
static void *myrealloc(void *ptr, size_t size);
static size_t write_filedata(void *ptr, size_t size, size_t nmemb, void *stream);
    
struct MemoryStruct {
	char *memory;
	size_t size;
};
typedef struct MemoryStruct MemoryStruct;
  
  static void *myrealloc(void *ptr, size_t size)
  {
    /* There might be a realloc() out there that doesn't like reallocing
       NULL pointers, so we take care of it here */
    if(ptr)
      return realloc(ptr, size);
    else
      return malloc(size);
  }
  
 static size_t
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
  
  static size_t write_filedata(void *ptr, size_t size, size_t nmemb, void *stream)
{
	int written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}