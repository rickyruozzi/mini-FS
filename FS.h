#ifndef MY_FS_H
#define MY_FS_H

#include <stdint.h>

#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h> 
#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define FS_MAGIC 0xF5F15F5F //magic number del file system
#define BLOCK_SIZE 4096 //dimensione di un blocco di byte (4 KB)
#define MAX_BLOCKS 8192 //numero massimo di blocchi nel file system
//avremo 4096*8192=32MB di spazio totale
#define MAX_INODES 1024 //massimo numero di inode
#define INODE_DIRECT 12 //numero di blocchi diretti in un inode
#define INODE_INDIRECT 1 //numero di blocchi indiretti in un inode
//ogni inode può puntare ad un altro blocco con altri puntatori
#define FNAME_LEN 256 //lunghezza massima del nome del file

typedef uint32_t block_t; //dimensione di un blocco 
typedef uint32_t inode_t; //dimensione di un inode
typedef uint32_t ui32; //unsigned int a 32 bit
typedef uint8_t ui8; //unsigned int a 8 bit

struct superblock {
    ui32 magic; //magic number del file system
    ui32 block_size; //dimensione di un blocco
    ui32 total_blocks; //numero totale di blocchi 
    ui32 inode_table_blocks; // numero di blocchi riservati alla tabella degli inode
    ui32 inode_count; //numero totale di inode
    ui32 free_block_bitmap_start; //blocco di inizio della bitmap dei blocchi liberi
    ui32 inode_table_start; //blocco di inizio della tabella degli inode
    ui32 data_start;       //blocco di inizio dell'area dati        
};

struct inode{
    ui32 size; //dimensione del file in byte
    ui32 directBlocks[INODE_DIRECT]; //puntatori diretti ai blocchi di dati 
    ui32 indirectBlock; //puntatore indiretto ai blocchi di dati
    ui32 isUsed; //indica se l'inode è in uso, true quando il file esiste
    ui32 created_at; //timestamp di creazione
    ui32 modified_at; //timestamp di ultima modifica
};

struct dirEntry{
    char fname[FNAME_LEN]; //nome del file
    inode_t inodeNum; //numero dell'inode corrispondente 
};

struct filesystem {
    FILE *img;                 //file immagine del file system
    struct superblock sb;       //superblocco del file system
    ui8 *blockBitmap;          //bitmap dei blocchi liberi
    struct inode *inodeTable;   //tabella degli inode
};

// Function prototypes
struct filesystem *open_fs(const char *img, bool printBlocks, bool printInodes);
void printBitmap(struct filesystem *fs);
void printInodeTable(struct filesystem *fs);
block_t block_alloc(struct filesystem *fs);
int free_block(struct filesystem *fs, block_t blockNum);
int read_block(FILE *F, block_t block_num, void *buffer);
int write_block(FILE *F, block_t block_num, void *buffer);
inode_t inode_alloc(struct filesystem *fs);
int free_inode(struct filesystem *fs, inode_t inodeNum);
struct inode inode_read(struct filesystem *fs, inode_t inodenum);
int inode_write(struct filesystem *fs, inode_t inodenum);
int dir_lookup(struct filesystem *fs, struct inode *dir_inode, const char* name, struct inode *result);
int dir_add_entry(struct filesystem *fs, inode_t dir_inode_num, const char *name, inode_t inodeNum);
int dir_remove_entry(struct filesystem *fs, struct inode *dir_inode, const char *name);

#endif
