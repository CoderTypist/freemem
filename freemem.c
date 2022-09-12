#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


// NOTE:
//     memset(0xFF) leads to (prev == next == 0xFFFFFFFFFFFFFFFF) 
//     This is because free() sometimes doesn't write to prev and next (not sure why)
//     memset(0) makes sure prev and next are NULL by default
//     This way if free() doesn't set prev and next, those values will be NULL


#define FLAG printf("FLAG: %d\n", __LINE__);

#define FREEMEM(p) ( (FreeMem*)(((char*)p)-sizeof(unsigned long)) )
#define LEN_LINE 87

int page_size = 0;
#define PAGE_SIZE ({ if( 0 == page_size ) { page_size = sysconf(_SC_PAGESIZE); } page_size; })

extern char end;


typedef struct FreeMem {
    unsigned long size;
    void* prev;
    void* next;
} FreeMem;


void usage(char* progName);
void line(char c, int n);
void heap_bounds();

FreeMem* FreeMem_free_list_get_start(FreeMem* pMem);
unsigned int FreeMem_free_list_len(FreeMem* pMem);
void FreeMem_print(FreeMem* pMem);
void FreeMem_print_free_list(FreeMem* pMem);

FreeMem* alloc_nowrite_free(unsigned long size);
FreeMem* alloc_write_free(unsigned long size);
void* alloc_nowrite_nofree(unsigned long size);
void* alloc_write_nofree(unsigned long size);

void test_01_size_double_all_free(FreeMem*(*fnFree)(unsigned long));
void test_02_size_double_alternate_free(FreeMem*(*fnFree)(unsigned long), void*(*fnNofree)(unsigned long));
void test_03_size_page_all_free(FreeMem*(*fnFree)(unsigned long));
void test_04_size_page_alternate_free(FreeMem*(*fnFree)(unsigned long), void*(*fnNofree)(unsigned long));


int main(int argc, char** argv) {

    if( argc == 2 && ( 0 == strcmp(argv[1], "-h") || 0 == strcmp(argv[1], "--help") ) ) {
        usage(argv[0]);
        exit(-1);
    }

    if( argc != 4 ) {
        printf("Insufficient arguments:\n");
        usage(argv[0]);
        exit(-1);
    }

    char* flagFree = argv[1];
    FreeMem*(*fnFree)(unsigned long);
    
    if( 0 == strcmp(flagFree, "-n") || 0 == strcmp(flagFree, "--nowrite") ) {
        fnFree = alloc_nowrite_free;
    }
    else if( 0 == strcmp(flagFree, "-w") || 0 == strcmp(flagFree, "--write") ) {
        fnFree = alloc_write_free;
    }
    else {
        printf("Invalid arguments:\n");
        usage(argv[0]);
        exit(-1);
    }

    char* flagNofree = argv[2];
    void*(*fnNofree)(unsigned long);

    if( 0 == strcmp(flagFree, "-n") || 0 == strcmp(flagFree, "--nowrite") ) {
        fnNofree = alloc_nowrite_nofree;
    }
    else if( 0 == strcmp(flagFree, "-w") || 0 == strcmp(flagFree, "--write") ) {
        fnNofree = alloc_write_nofree;
    }
    else {
        printf("Invalid arguments:\n");
        usage(argv[0]);
        exit(-1);
    }

    int test = atoi(argv[3]);
    switch(test) {
        case 1:
            test_01_size_double_all_free(fnFree);
            break;
        case 2:
            test_02_size_double_alternate_free(fnFree, fnNofree);
            break;
        case 3:
            test_03_size_page_all_free(fnFree);
            break;
        case 4:
            test_04_size_page_alternate_free(fnFree, fnNofree);
            break;
        default:
            printf("Invalid test: %d\n", test);
    }

    return 0;
}


void usage(char* progName) {

    printf("\nUSAGE:\n");
    printf("\n");

    printf("    FREE(alloc, write) && NOFREE(alloc, write):\n");
    printf("        %s -w -w <#>\n", progName);
    printf("        %s --write --write <test #>\n", progName);
    printf("\n");

    printf("    FREE(alloc, write) && NOFREE(alloc, nowrite):\n");
    printf("        %s -w -n <#>\n", progName);
    printf("        %s --write --nowrite <test #>\n", progName);
    printf("\n");

    printf("    FREE(alloc, nowrite) && NOFREE(alloc, write):\n");
    printf("        %s -n -w <#>\n", progName);
    printf("        %s --nowrite --write <test #>\n", progName);
    printf("\n");

    printf("    FREE(alloc, nowrite) && NOFREE(alloc, nowrite):\n");
    printf("        %s -n -n <#>\n", progName);
    printf("        %s --nowrite --nowrite <test #>\n", progName);
    printf("\n");

    printf("    TESTS:\n");
    printf("        1 - double size of requested memory, free immediately\n");
    printf("        2 - double size of requested memory, free every other alloc\n");
    printf("        3 - request page size of memory, free immediately\n");
    printf("        4 - request page size of memory, free every other alloc\n");
    printf("\n");

    printf("    help:\n");
    printf("        %s -h\n", progName);
    printf("        %s --help\n", progName);
    printf("\n");
}


void line(char c, int n) {

    int i;
    for( i = 0 ; i < n ; i++ ) {
        printf("%c", c);
    }
    printf("\n");
}


void heap_bounds() {
    void* pHeapStart = (void*)&end;
    void* pHeapEnd = sbrk(0);
    ptrdiff_t heapSize = pHeapEnd - pHeapStart;
    printf("heap size = %td (start = %p, end = %p)\n", heapSize, pHeapStart, pHeapEnd);
}


FreeMem* FreeMem_free_list_get_start(FreeMem* pMem) {

    assert(pMem != NULL);
    FreeMem* pCur = pMem;
    while( pCur->prev != NULL ) {
        // printf("BEFORE: FreeMem_free_list_get_start(): pCur: %p\n", pCur);
        // printf("BEFORE: FreeMem_free_list_get_start(): pCur: %ld\n", pCur->size);
        // printf("BEFORE: FreeMem_free_list_get_start(): pCur: %p\n", pCur->prev);
        // printf("BEFORE: FreeMem_free_list_get_start(): pCur: %p\n", pCur->next);
        pCur = pCur->prev;
        // printf("AFTER: FreeMem_free_list_get_start(): pCur: %p\n", pCur);
        // printf("AFTER: FreeMem_free_list_get_start(): pCur: %ld\n", pCur->size);
        // printf("AFTER: FreeMem_free_list_get_start(): pCur: %p\n", pCur->prev);
        // printf("AFTER: FreeMem_free_list_get_start(): pCur: %p\n", pCur->next);
    }
    assert(pCur != NULL); // REMOVE
    return pCur;
}


unsigned int FreeMem_free_list_len(FreeMem* pMem) {

    assert(pMem != NULL);
    FreeMem* pStart = FreeMem_free_list_get_start(pMem);

    assert(pStart != NULL);

    FreeMem* pCur = pStart;
    unsigned int len = 1;
    while( pCur->prev != NULL || pCur->next != NULL ) {
        len++;
        pCur = pCur->next;
    }
    return len;
}


void FreeMem_print(FreeMem* pMem) {
    printf("| %-18p | %-20ld | %-18p | %-18p |\n", pMem, pMem->size, pMem->prev, pMem->next);
}


void FreeMem_print_free_list(FreeMem* pMem) {
    
    FreeMem* pStart = FreeMem_free_list_get_start(pMem);
    FreeMem* pCur = pStart;

    while( pCur->prev != NULL && pCur->next != NULL ) {
        line('-', LEN_LINE);
        FreeMem_print(pCur);
        pCur = pCur->next;
    }

    if( pCur->prev == NULL && pCur->next == NULL ) {
        line('-', LEN_LINE);
        FreeMem_print(pCur);
        line('-', LEN_LINE);
    }
    else {
        line('-', LEN_LINE);
        FreeMem_print(pCur);
        line('-', LEN_LINE);
        FreeMem_print(pCur->next);
        line('-', LEN_LINE);
    }
}


FreeMem* alloc_nowrite_free(unsigned long size) {

    printf("Routine: .F (%ld) \n\n", size);

    printf("    - before: ");
    heap_bounds();

    void* pAlloc = malloc(size);
    assert(pAlloc != NULL);
    free(pAlloc);
    FreeMem* pMem = FREEMEM(pAlloc);

    printf("    - after:  ");
    heap_bounds();
    printf("\n");

    printf("FREELIST (len == %d):\n", FreeMem_free_list_len(pMem));
    FreeMem_print_free_list(pMem);

    printf("\n");
    line('.', LEN_LINE);
    printf("\n");

    return pMem;
}


FreeMem* alloc_write_free(unsigned long size) {

    printf("Routine: WF (%ld) \n\n", size);

    printf("    - before: ");
    heap_bounds();

    void* pAlloc = malloc(size);
    assert(pAlloc != NULL);
    
    // ensure pMem->prev and pMem->next are NULL in the case free doesn't write to those fields
    // a write shorter than (2 * sizeof(void*)) bytes might leave garbage data in those fields
    if( size >= ( 2 * sizeof(void*) ) )
        memset(pAlloc, 0, size);
    else
        memset(pAlloc, 0, 2 * sizeof(void*) );
    
    free(pAlloc);
    FreeMem* pMem = FREEMEM(pAlloc);

    printf("    - after:  ");
    heap_bounds();
    printf("\n");

    printf("FREELIST (len == %d):\n", FreeMem_free_list_len(pMem));
    FreeMem_print_free_list(pMem);

    printf("\n");
    line('.', LEN_LINE);
    printf("\n");

    return pMem;
}


void* alloc_nowrite_nofree(unsigned long size) {

    printf("Routine: ..\n\n");

    printf("    - before: ");
    heap_bounds();

    void* pAlloc = malloc(size);
    assert(pAlloc != NULL);

    printf("    - after:  ");
    heap_bounds();
    printf("\n"); 

    printf("( %ld @ %p )\n", size, FREEMEM(pAlloc));
    printf("\n");
    line('.', LEN_LINE);
    printf("\n");
}


void* alloc_write_nofree(unsigned long size) {

    printf("Routine: W. (%ld)\n\n", size);

    printf("    - before: ");
    heap_bounds();

    void* pAlloc = malloc(size);
    assert(pAlloc != NULL);

    // ensure pMem->prev and pMem->next are NULL in the case free doesn't write to those fields
    // a write shorter than (2 * sizeof(void*)) bytes might leave garbage data in those fields
    if( size >= ( 2 * sizeof(void*) ) )
        memset(pAlloc, 0, size);
    else
        memset(pAlloc, 0, 2 * sizeof(void*) );

    printf("    - after:  ");
    heap_bounds();
    printf("\n"); 

    printf("( %ld @ %p is %ld )\n", size, FREEMEM(pAlloc), FREEMEM(pAlloc)->size);
    printf("\n");
    line('.', LEN_LINE);
    printf("\n");
}


void test_01_size_double_all_free(FreeMem*(*fnFree)(unsigned long)) {

    // >>> for i in range(16):
    // ...     print(f'fnFree({2**i});')
    // ...

    fnFree(1);
    fnFree(2);
    fnFree(4);
    fnFree(8);
    fnFree(16);
    fnFree(32);
    fnFree(64);
    fnFree(128);
    fnFree(256);
    fnFree(512);
    fnFree(1024);
    fnFree(2048);
    fnFree(4096);
    fnFree(8192);
    fnFree(16384);
    fnFree(32768);
}


void test_02_size_double_alternate_free(FreeMem*(*fnFree)(unsigned long), void*(*fnNofree)(unsigned long)) {
    
    // >>> for i in range(16):
    // ...     if i % 2 == 0:
    // ...         print(f'fnFree({2**i});')
    // ...     else:
    // ...         print(f'fnNofree({2**i});')
    // ...

    fnFree(1);
    fnNofree(2);
    fnFree(4);
    fnNofree(8);
    fnFree(16);
    fnNofree(32);
    fnFree(64);
    fnNofree(128);
    fnFree(256);
    fnNofree(512);
    fnFree(1024);
    fnNofree(2048);
    fnFree(4096);
    fnNofree(8192);
}


void test_03_size_page_all_free(FreeMem*(*fnFree)(unsigned long)) {
    
    // >>> for i in range(16):
    // ...     print(f'fnFree(PAGE_SIZE);')
    // ...

    fnFree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
}


void test_04_size_page_alternate_free(FreeMem*(*fnFree)(unsigned long), void*(*fnNofree)(unsigned long)) {

    // >>> for i in range(16):
    // ...     if i % 2 == 0:
    // ...         print(f'fnFree(PAGE_SIZE);')
    // ...     else:
    // ...         print(f'fnNofree(PAGE_SIZE);')
    // ...

    fnFree(PAGE_SIZE);
    fnNofree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnNofree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnNofree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnNofree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnNofree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnNofree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnNofree(PAGE_SIZE);
    fnFree(PAGE_SIZE);
    fnNofree(PAGE_SIZE);
}