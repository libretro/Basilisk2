#include <string.h>

#include "memmap.h"
#include "memalign.h"
#include "vm_alloc.h"

int vm_init(){return 0;}
void vm_exit(){}

void* vm_acquire(size_t size, int options){
   void* region = memalign_alloc(64,size);
   memset(region,0x00,size);
   return region;
}

int vm_release(void* addr, size_t size){
   memalign_free(addr);
   return 0;
}

int vm_get_page_size(void){return 65536;}

//should always fail
int vm_acquire_fixed(void* addr, size_t size, int options){return -1;}
int vm_protect(void* addr, size_t size, int prot){return -1;}
int vm_get_write_watch(void* addr, size_t size,void** pages, unsigned int* n_pages,int options){return -1;}
int vm_reset_write_watch(void* addr, size_t size){return -1;}
