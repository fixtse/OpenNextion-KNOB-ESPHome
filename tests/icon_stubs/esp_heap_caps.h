#pragma once
#include <cstdlib>
#include <cassert>
#include <map>
#include <cstring>
constexpr int MALLOC_CAP_SPIRAM=1,MALLOC_CAP_8BIT=2;
inline std::map<void *,size_t> external_allocations;
inline size_t largest_external_allocation=0;
inline void *heap_caps_malloc(size_t size,int caps){
 assert(caps==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
 void *p=std::malloc(size);if(p)external_allocations[p]=size;
 if(size>largest_external_allocation)largest_external_allocation=size;return p;
}
inline void *heap_caps_calloc(size_t n,size_t size,int caps){
 auto *p=heap_caps_malloc(n*size,caps);if(p)std::memset(p,0,n*size);return p;
}
inline void heap_caps_free(void *p){if(p){assert(external_allocations.erase(p)==1);std::free(p);}}
inline void *heap_caps_realloc(void *p,size_t size,int caps){
 assert(caps==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
 auto *next=std::realloc(p,size);
 if(next){external_allocations.erase(p);external_allocations[next]=size;}return next;
}
