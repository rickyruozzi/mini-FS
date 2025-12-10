#include "FS.h"
#include <stdio.h>
#include <string.h>

int main(void){
    const char *img = "path_test.img";
    ui32 totalBlocks = 1024;

    // create image
    if(init_fs(img, totalBlocks) != 0){
        printf("init_fs failed\n");
        return 1;
    }

    // open fs
    struct filesystem *fs = open_fs(img, false, false);
    if(!fs){
        printf("open_fs failed\n");
        return 1;
    }

    // initialize root inode (inode 0)
    fs->inodeTable[0].isUsed = 1;
    fs->inodeTable[0].created_at = (ui32)time(NULL);
    fs->inodeTable[0].modified_at = (ui32)time(NULL);
    memset(fs->inodeTable[0].directBlocks, 0, INODE_DIRECT * sizeof(ui32));

    // allocate a data block for root directory
    block_t root_block = block_alloc(fs);
    if(root_block == (block_t)-1){
        printf("block_alloc for root failed\n");
        return 1;
    }
    fs->inodeTable[0].directBlocks[0] = root_block;

    // persist bitmap to disk
    ui32 bitmapSize = (fs->sb.total_blocks + 7) / 8;
    fseek(fs->img, BLOCK_SIZE * fs->sb.free_block_bitmap_start, SEEK_SET);
    if(fwrite(fs->blockBitmap, bitmapSize, 1, fs->img) != 1){
        printf("Failed to write bitmap to disk\n");
        return 1;
    }

    // create a file inode
    inode_t file_inode = inode_alloc(fs);
    if(file_inode == (inode_t)-1){
        printf("inode_alloc failed\n");
        return 1;
    }

    // prepare directory block: ., .., hello.txt
    char blockbuf[BLOCK_SIZE];
    memset(blockbuf, 0, BLOCK_SIZE);
    struct dirEntry *entries = (struct dirEntry*)blockbuf;
    strncpy(entries[0].fname, ".", FNAME_LEN);
    entries[0].inodeNum = 0;
    strncpy(entries[1].fname, "..", FNAME_LEN);
    entries[1].inodeNum = 0;
    strncpy(entries[2].fname, "hello.txt", FNAME_LEN);
    entries[2].inodeNum = file_inode;

    // write directory block to disk
    if(write_block(fs->img, root_block, blockbuf) != 0){
        printf("write_block failed\n");
        return 1;
    }

    // update root inode size and persist inode table entry
    fs->inodeTable[0].size = 3 * sizeof(struct dirEntry);
    if(inode_write(fs, 0) != 0){
        printf("inode_write failed for root\n");
        return 1;
    }

    // now test path_solver for "hello.txt"
    struct inode result;
    // debug: print root inode and block info
    printf("root directBlocks[0]=%u, allocated root_block=%u\n", fs->inodeTable[0].directBlocks[0], root_block);
    char readbuf[BLOCK_SIZE];
    if(read_block(fs->img, root_block, readbuf) != 0){
        printf("read_block of root failed\n");
        return 1;
    }
    struct dirEntry *rents = (struct dirEntry*)readbuf;
    printf("entries in root block:\n");
    for(int i=0;i<4;i++){
        printf("  [%d] name='%s' inode=%u\n", i, rents[i].fname, rents[i].inodeNum);
    }

    if(path_solver(fs, "hello.txt", &result) != 0){
        printf("path_solver failed to find hello.txt\n");
        return 1;
    }

    // find result's inode number by scanning inodeTable
    inode_t found = (inode_t)-1;
    for(inode_t i=0; i<fs->sb.inode_count; i++){
        if(memcmp(&fs->inodeTable[i], &result, sizeof(struct inode)) == 0){
            found = i;
            break;
        }
    }

    if(found == (inode_t)-1){
        printf("Could not map result inode to table index\n");
        return 1;
    }

    if(found != file_inode){
        printf("Test failed: expected inode %u, got %u\n", file_inode, found);
        return 1;
    }

    printf("Test passed: path_solver resolved 'hello.txt' to inode %u\n", found);

    // cleanup
    fclose(fs->img);
    free(fs->blockBitmap);
    free(fs->inodeTable);
    free(fs);

    return 0;
}
