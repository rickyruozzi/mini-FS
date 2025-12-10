#include "../FS.h"
#include <stdio.h>
#include <string.h>

// count functions are defined in FS.c but not declared in FS.h; declare here
extern int count_free_blocks(ui8 *bitmap, ui32 total_blocks);
extern int count_free_bytes(ui8 *bitmap, ui32 total_blocks);
extern int count_free_mBs(ui8 *bitmap, ui32 total_blocks);

int run_case(ui32 total_blocks, ui8 *bitmap, int expected_blocks){
    int b = count_free_blocks(bitmap, total_blocks);
    int bytes = count_free_bytes(bitmap, total_blocks);
    int mbs = count_free_mBs(bitmap, total_blocks);
    printf("total_blocks=%u -> free_blocks=%d, free_bytes=%d, free_mBs=%d\n", total_blocks, b, bytes, mbs);
    if (b != expected_blocks){
        printf("  FAIL: expected %d free blocks\n", expected_blocks);
        return 1;
    }
    if (bytes != b * BLOCK_SIZE){
        printf("  FAIL: bytes mismatch (%d != %d)\n", bytes, b * BLOCK_SIZE);
        return 1;
    }
    if (mbs != (bytes / 1000)){
        printf("  FAIL: mBs mismatch (%d != %d)\n", mbs, bytes / 1000);
        return 1;
    }
    printf("  PASS\n");
    return 0;
}

int main(void){
    int fails = 0;

    // Case 1: all free for 16 blocks
    ui32 tb1 = 16;
    ui8 bm1[(16+7)/8];
    memset(bm1, 0x00, sizeof(bm1)); // all bits 0 => all free
    fails += run_case(tb1, bm1, 16);

    // Case 2: all used for 16 blocks
    ui32 tb2 = 16;
    ui8 bm2[(16+7)/8];
    memset(bm2, 0xFF, sizeof(bm2)); // all bits 1 => all used
    fails += run_case(tb2, bm2, 0);

    // Case 3: mixed for 20 blocks: mark blocks 0,2,4.. as used (even used)
    ui32 tb3 = 20;
    ui8 bm3[(20+7)/8];
    memset(bm3, 0x00, sizeof(bm3));
    for (ui32 i=0;i<tb3;i++){
        if ((i % 2) == 0){ // used
            bm3[i/8] |= (1u << (i % 8));
        }
    }
    // free blocks are odd indices => expected 10
    fails += run_case(tb3, bm3, 10);

    // Case 4: last byte has spare bits beyond total_blocks: total_blocks=10, ensure they ignored
    ui32 tb4 = 10;
    ui8 bm4[(10+7)/8];
    memset(bm4, 0xFF, sizeof(bm4)); // all used
    // free block 9 only
    bm4[9/8] &= ~(1u << (9%8));
    fails += run_case(tb4, bm4, 1);

    if (fails == 0) printf("All bitmap count tests passed\n");
    else printf("%d tests failed\n", fails);

    return fails ? 1 : 0;
}
