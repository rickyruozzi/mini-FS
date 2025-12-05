#include "./FS.h"

// Function prototypes
void printBitmap(ui8 *blockBitmap, ui32 totalBlocks);
void printInodeTable(struct inode *inodeTable, ui32 inodeCount);

int init_fs(const char *img, ui32 totalBlocks){
    FILE* F=fopen(img,"wb");
    if (F==NULL){
        printf("Errore nella creazione dell'immagine del file system.\n");
        return -1; //errore nell'apertura del file
    }
    //inizializzazione superblocco
    struct superblock *sb = malloc(sizeof(struct superblock));
    sb->magic = FS_MAGIC;
    sb->block_size = BLOCK_SIZE;
    sb->total_blocks = totalBlocks;
    sb->inode_table_blocks = (MAX_INODES * sizeof(struct inode) + BLOCK_SIZE -1) / BLOCK_SIZE;
    /*calcolo del numero di blocchi da cui è composta la tabela degli inodes
    (numero di nodi * dimensione di un inode + blocco separatore -1)/dimensione dei blocchi */
    sb->inode_count = MAX_INODES; //massimo numero di inode
    sb->free_block_bitmap_start = 1; //superblocco occupa il blocco 0
    sb->inode_table_start = sb->free_block_bitmap_start + (totalBlocks + 7) / 8 / BLOCK_SIZE + 1;
    /*calcolo del blocco di inizio della tabella degli inode*/
    sb->data_start = sb->inode_table_start + sb->inode_table_blocks;
    //trova il primo blocco dedicato ai dati dopo la tabella degli inode
    //scrittura del superblocco
    fwrite(sb, sizeof(struct superblock), 1, F);
    free(sb); //scriviamo il superblocco nel file e liberiamo la memoria

    //inizializziamo la bitmap dei blocchi liberi 
    ui32 bitmapSize = (totalBlocks + 7) / 8; 
     /*dimensione della bitmap data dal numero totale di blocchi arrotondato al blocco superiore,
     diviso per il numero di bit nel byte */
    ui8 *blockBitmap = malloc(bitmapSize); //la dimensione della bitmap in byte
    fseek(F, BLOCK_SIZE * sb->free_block_bitmap_start, SEEK_SET); //spostiamo il puntatore di scrittura all'inizio della bitmap
    memset(blockBitmap, 0, bitmapSize); 
    //memset inizializza la bitmap a 0 (tutti i blocchi liberi)
    fwrite(blockBitmap, bitmapSize, 1, F);
    //scrive la bitmap dei blocchi liberi nel file
    free(blockBitmap); //libera lo spazio della bitmap in RAM

    //inizializziamo la tabella degli inode 
    struct inode *inodeTable = malloc(sb->inode_count * sizeof(struct inode));
    //allochiamo lo spazio per inode_count inode nella tabella
    memset(inodeTable, 0, sb->inode_count * sizeof(struct inode)); //inizializziamo tutti gli inode a zero
    fseek(F, BLOCK_SIZE * sb->inode_table_start, SEEK_SET); //spostiamo il puntatore di scrittura all'inizio della tabella degli inode
    fwrite(inodeTable, sb->inode_count * sizeof(struct inode), 1, F); //scriviamo la tabella degli inode inizializzata nel file
    free(inodeTable); //liberiamo lo spazio della tabella degli inode in RAM

    fclose(F); //chiudiamo il file
    return 0; //il valore di ritorno 0 indica che il processo è andato a buon fine
}

int open_fs(const char *img, bool printBlocks, bool printInodes){
    FILE* F=fopen(img, "rb");
    if (F==NULL){
        printf("Errore nell'apertura dell'immagine del file system.\n");
        return -1; //errore nell'apertura del file
    }
    struct superblock *sb = malloc(sizeof(struct superblock));
    fread(&(*sb), sizeof(struct superblock),1,F); //leggiamo e inseriamo il superblock nel buffer sb
    if (sb->magic!=FS_MAGIC){
        printf("Errore: immagine del file system non valida.\n");
        fclose(F);
        return -1; //errore: immagine del file system non valida
    }
    ui32 bitmapSize = (sb->total_blocks + 7) / 8; 
    ui8 *blockBitmap = malloc(bitmapSize); 
    fseek(F, BLOCK_SIZE * sb->free_block_bitmap_start, SEEK_SET); //spostiamo la testina per leggere all'inizio della bitmap
    fread(blockBitmap, bitmapSize, 1, F); //leggiamo la bitmap dei blocchi liberi nel buffer blockBitmap
    

    struct inode *inodeTable = malloc(sb->inode_count * sizeof(struct inode));
    fseek(F, BLOCK_SIZE * sb->inode_table_start, SEEK_SET); //spostiamo la testina per leggere all'inizio della tabella degli inode
    fread(inodeTable, sb->inode_count * sizeof(struct inode), 1, F); //leggiamo la tabella degli inode nel buffer inodeTable
   
    free(sb);
    
    
    if(printBlocks==true){
        printBitmap(blockBitmap, sb->total_blocks);
    }
    if(printInodes==true){
        printInodeTable(inodeTable, sb->inode_count);
    }
    free(blockBitmap);
    free(inodeTable);
    fclose(F);
    return 0;
}

void printBitmap(ui8 *blockBitmap, ui32 totalBlocks){
    printf("bitmap dei blocchi liberi:\n");
    for(ui32 i=0; i<totalBlocks; i++){ //per ogni blocco (4 kb)
        ui8 byte = blockBitmap[i/8]; //prendiamo il byte corrispondente
        ui8 bit = (byte >> (i%8))&1; //prendiamo il bit corrispondente
        // shiftiamo il byte di i%8 posizioni a destra e facciamo l'AND con 1 per ottenere il bit
        printf("Blocco %u: %s\n", i, bit ? "Occupato" : "Libero");  //1 - bloccato, 0 - libero
        //stampa lo stato del blocco
    }
}

void printInodeTable(struct inode *inodeTable, ui32 inodeCount){
    printf("Tabella degli inode: \n"); //stampa la tabella degli inode
    for(ui32 i=0; i<inodeCount; i++){
        struct inode currentInode = inodeTable[i]; //prende l'elemento i-esimo dalla tabella
        if(currentInode.isUsed){ //se il nodo è in uso
            printf("Inode %u:\n", i); //stampa il numero di inode
            printf("    Dimensione del file: %u byte\n", currentInode.size); //stampa la dimensione del file
            printf("    Blocchi diretti: "); //stampa i blocchi diretti con il ciclo che segue
            for(ui32 j=0; j<INODE_DIRECT; j++){
                if(currentInode.directBlocks[j] != 0){ //se il blocco diretto non è 0 allora stampo il contenuto
                    printf("%u ", currentInode.directBlocks[j]);
                }
            }
            printf("\n");
            if(currentInode.indirectBlock != 0){
                printf("  Blocco indiretto: %u\n", currentInode.indirectBlock); //stampo il blocco indiretto se non è 0
            }
            printf("  Creato il: %u\n", currentInode.created_at); //stampo il timestamp di creazione
            printf("  Ultima modifica: %u\n", currentInode.modified_at); //stampo il timestamp dell'ultima modifica
        }
    }
}

block_t block_alloc(ui8 *blockBitmap, ui32 totalBlocks){
    for(ui32 i=0; i<totalBlocks; i++){
        ui8 byte = blockBitmap[i/8];
        ui8 bit = (byte >> (i%8)) & 1;
        if(bit==0){
            blockBitmap[i/8] = byte | (1 << (i%8)); //setto il bit a 1 per indicare che il blocco è occupato
            /*
            i è l'indice del blocco corrente, byte è il byte corrente della bitmap (ottenuto con i/8),
            l'operazione (1<<(i%8)) sposta il bit 1 a sinistra di (i%8) posizioni per creare una maschera, 
            l'operazioone OR (|) tra byte e la maschera imposta il bit corrispondente a 1, indicando che il blocco è ora allocato. 
            */
            return i; //ritorno il blocco allocato
        }
    }
}

int free_block(ui8 *blockBitmap, block_t blockNum){
    ui8 byte = blockBitmap[blockNum/8];
    ui8 bit = (byte >> (blockNum%8)) & 1; //ottengo il bit corrispondente al blocco
    if(bit==1){
        blockBitmap[blockNum/8] = byte & ~(1 << (blockNum%8)); 
        /*
        l'operazione (1 << (blockNum%8)) crea una maschera con il bit corrispondente al blocco impostato a 1,
        l'operazione NOT (~) inverte la maschera, impostando quel bit a 0 e tutti gli altri a 1,
        l'operazione AND (&) tra byte e la maschera aggiornata imposta il bit corrispondente al blocco a 0, 
        indicando che il blocco è ora libero.
        */
        return 0; //ritorno 0 per indicare che il blocco è stato liberato con successo
    } else {
        return -1; //ritorno -1 per indicare che il blocco era già libero
    }
}

int read_block(FILE *F, block_t block_num, void *buffer){
    fseek(F, block_num * BLOCK_SIZE, SEEK_SET); //spostiamo la testina di lettura al blocco desiderato
    size_t result = fread(buffer, BLOCK_SIZE, 1, F); //inseriamo in result il numero di blocchi che abbiamo letto 
    //abbiamo inoltre inserito nel buffer che abbiamo passato il contenuto del blocco letto con fread
    if (result != 1) {
        return -1; //errore nella lettura del blocco
    }
    return 0; 
}

int write_block(FILE *F, block_t block_num, void *buffer){
    fseek(F, block_num * BLOCK_SIZE, SEEK_SET); //spostiamo la testina di scrittura nel blocco in cui vogliamo scrivere
    size_t result = fwrite(buffer, BLOCK_SIZE, 1, F); //scriviamo il contenuto del buffer nel blocco 
    if(result !=1){
        return -1; //errore nella scrittura del blocco
    }
    return 0; 
}

inode_t inode_alloc(struct inode *inodeTable, ui32 inodecount){
    for(ui32 i=0; i<inodecount; i++){
        if(inodeTable[i].isUsed==0){ //se l'inode non è in uso
            inodeTable[i].isUsed=1; //lo segniamo come in uso
            inodeTable[i].size=0; //inizializziamo la dimensione del file a 0
            inodeTable[i].created_at = (ui32)time(NULL); //impostiamo il timestamp di creazione
            inodeTable[i].modified_at = (ui32)time(NULL); //impostiamo
            inodeTable[i].indirectBlock = 0; //inizializziamo il puntatore al blocco indiretto a 0
            memset(inodeTable[i].directBlocks, 0, INODE_DIRECT * sizeof(ui32)); //inizializziamo i blocchi diretti legati al file a 0
            return i; //ritorniamo il numero dell'inode allocato
        }
    }
    return -1; //nessun inode disponibile
}

int free_inode(struct inode *inodeTable, inode_t inodeNum){
    if(inodeTable[inodeNum].isUsed==1){
        inodeTable[inodeNum].isUsed=0;
        return 0;
    }
    return -1; //l'inode era già libero
}

int main() {
    // Test della creazione e apertura del file system
    printf("Inizializzazione del file system...\n");
    if (init_fs("test.img", MAX_BLOCKS) == 0) {
        printf("File system inizializzato con successo.\n");

        // Test semplice di scrittura e lettura blocco
        FILE *F = fopen("test.img", "r+b");
        if (F != NULL) {
            char writeData[BLOCK_SIZE] = "Ciao, questo è un test di scrittura blocco!";
            printf("Scrittura blocco 10...\n");
            if (write_block(F, 10, writeData) == 0) {
                printf("Blocco scritto con successo.\n");

                char readData[BLOCK_SIZE];
                printf("Lettura blocco 10...\n");
                if (read_block(F, 10, readData) == 0) {
                    printf("Contenuto letto: %s\n", readData);
                } else {
                    printf("Errore nella lettura del blocco.\n");
                }
            } else {
                printf("Errore nella scrittura del blocco.\n");
            }
            fclose(F);
        } else {
            printf("Errore nell'apertura del file per test blocco.\n");
        }

        printf("Apertura e lettura del file system...\n");
        if (open_fs("test.img", false, false) == 0) {
            printf("File system aperto e letto con successo.\n");
        } else {
            printf("Errore nell'apertura del file system.\n");
        }
    } else {
        printf("Errore nell'inizializzazione del file system.\n");
    }
    return 0;
}
