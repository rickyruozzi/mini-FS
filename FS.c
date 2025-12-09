#include "./FS.h"

// Function prototypes
void printBitmap(struct filesystem *fs);
void printInodeTable(struct filesystem *fs);

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

    free(sb); //scriviamo il superblocco nel file e liberiamo la memoria

    fclose(F); //chiudiamo il file
    return 0; //il valore di ritorno 0 indica che il processo è andato a buon fine
}

struct filesystem *open_fs(const char *img, bool printBlocks, bool printInodes){
    FILE* F=fopen(img, "r+b");
    if (F==NULL){
        printf("Errore nell'apertura dell'immagine del file system.\n");
        return NULL; //errore nell'apertura del file
    }
    struct filesystem *fs = malloc(sizeof(struct filesystem));
    fread(&(fs->sb), sizeof(struct superblock),1,F); //leggiamo e inseriamo il superblock nel buffer sb
    if (fs->sb.magic!=FS_MAGIC){
        printf("Errore: immagine del file system non valida.\n");
        fclose(F);
        free(fs);
        return NULL; //errore: immagine del file system non valida
    }
    ui32 bitmapSize = (fs->sb.total_blocks + 7) / 8;
    fs->blockBitmap = malloc(bitmapSize);
    fseek(F, BLOCK_SIZE * fs->sb.free_block_bitmap_start, SEEK_SET); //spostiamo la testina per leggere all'inizio della bitmap
    fread(fs->blockBitmap, bitmapSize, 1, F); //leggiamo la bitmap dei blocchi liberi nel buffer blockBitmap


    fs->inodeTable = malloc(fs->sb.inode_count * sizeof(struct inode));
    fseek(F, BLOCK_SIZE * fs->sb.inode_table_start, SEEK_SET); //spostiamo la testina per leggere all'inizio della tabella degli inode
    fread(fs->inodeTable, fs->sb.inode_count * sizeof(struct inode), 1, F); //leggiamo la tabella degli inode nel buffer inodeTable

    fs->img = F; //assign the file pointer

    if(printBlocks==true){
        printBitmap(fs);
    }
    if(printInodes==true){
        printInodeTable(fs);
    }
    return fs;
}

void printBitmap(struct filesystem *fs){
    printf("bitmap dei blocchi liberi:\n");
    for(ui32 i=0; i<fs->sb.total_blocks; i++){ //per ogni blocco (4 kb)
        ui8 byte = fs->blockBitmap[i/8]; //prendiamo il byte corrispondente
        ui8 bit = (byte >> (i%8))&1; //prendiamo il bit corrispondente
        // shiftiamo il byte di i%8 posizioni a destra e facciamo l'AND con 1 per ottenere il bit
        printf("Blocco %u: %s\n", i, bit ? "Occupato" : "Libero");  //1 - bloccato, 0 - libero
        //stampa lo stato del blocco
    }
}

void printInodeTable(struct filesystem *fs){
    printf("Tabella degli inode: \n"); //stampa la tabella degli inode
    for(ui32 i=0; i<fs->sb.inode_count; i++){
        struct inode currentInode = fs->inodeTable[i]; //prende l'elemento i-esimo dalla tabella
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

block_t block_alloc(struct filesystem *fs){
    for(ui32 i=fs->sb.data_start; i<fs->sb.total_blocks; i++){
        ui8 byte = fs->blockBitmap[i/8];
        ui8 bit = (byte >> (i%8)) & 1;
        if(bit==0){
            fs->blockBitmap[i/8] = byte | (1 << (i%8)); //setto il bit a 1 per indicare che il blocco è occupato
            /*
            i è l'indice del blocco corrente, byte è il byte corrente della bitmap (ottenuto con i/8),
            l'operazione (1<<(i%8)) sposta il bit 1 a sinistra di (i%8) posizioni per creare una maschera, 
            l'operazioone OR (|) tra byte e la maschera imposta il bit corrispondente a 1, indicando che il blocco è ora allocato. 
            */
            return i; //ritorno il blocco allocato
        }
    }
    return (block_t)-1; // no free block found
}

int free_block(struct filesystem *fs, block_t blockNum){
    ui8 byte = fs->blockBitmap[blockNum/8];
    ui8 bit = (byte >> (blockNum%8)) & 1; //ottengo il bit corrispondente al blocco
    if(bit==1){
        fs->blockBitmap[blockNum/8] = byte & ~(1 << (blockNum%8)); 
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

inode_t inode_alloc(struct filesystem *fs){
    for(ui32 i=0; i<fs->sb.inode_count; i++){
        if(fs->inodeTable[i].isUsed==0){ //se l'inode non è in uso
            fs->inodeTable[i].isUsed=1; //lo segniamo come in uso
            fs->inodeTable[i].size=0; //inizializziamo la dimensione del file a 0
            fs->inodeTable[i].created_at = (ui32)time(NULL); //impostiamo il timestamp di creazione
            fs->inodeTable[i].modified_at = (ui32)time(NULL); //impostiamo
            fs->inodeTable[i].indirectBlock = 0; //inizializziamo il puntatore al blocco indiretto a 0
            memset(fs->inodeTable[i].directBlocks, 0, INODE_DIRECT * sizeof(ui32)); //inizializziamo i blocchi diretti legati al file a 0
            return i; //ritorniamo il numero dell'inode allocato
        }
    }
    return (inode_t)-1; //nessun inode disponibile
}

int free_inode(struct filesystem *fs, inode_t inodeNum){
    if(fs->inodeTable[inodeNum].isUsed==1){
        fs->inodeTable[inodeNum].isUsed=0;
        return 0;
    }
    return -1; //l'inode era già libero
}

struct inode inode_read(struct filesystem *fs, inode_t inodenum){ //stampa le informazioni di un determinato inode
    if(fs->inodeTable[inodenum].isUsed==1){ //se l'inode è in uso
        printf("Inode %u:\n", inodenum); //stampa il numero di inode
        printf("    Dimensione del file: %u byte\n", fs->inodeTable[inodenum].size); //stampoa la dimensione del file
        printf("    Blocchi diretti: "); //stampa i blocchi diretti con il ciclo che segue
        for(ui32 j=0; j<INODE_DIRECT; j++){ //per ogni blocco diretto 
            if(fs->inodeTable[inodenum].directBlocks[j] != 0){
                printf("%u ", fs->inodeTable[inodenum].directBlocks[j]); //stampa il blocco diretto se non è 0
            }
        }
        printf("\n");
        if(fs->inodeTable[inodenum].indirectBlock != 0){
            printf("  Blocco indiretto: %u\n", fs->inodeTable[inodenum].indirectBlock);
        }
        printf("  Creato il: %u\n", fs->inodeTable[inodenum].created_at);
        printf("  Ultima modifica: %u\n", fs->inodeTable[inodenum].modified_at);
        return fs->inodeTable[inodenum]; //ritorna l'inode letto
    }
    struct inode emptyInode;
    memset(&emptyInode, 0, sizeof(struct inode)); //dichiara un nuovo inode vuoto e inizializza tutti i suoi campi a 0 
    return emptyInode; //poilo restituisce
}

int inode_write(struct filesystem *fs, inode_t inodenum){
    if(fs->inodeTable[inodenum].isUsed==1){
        fs->inodeTable[inodenum].modified_at = (ui32)time(NULL); //aggiorniamo il timestamp di modifica
        fseek(fs->img, BLOCK_SIZE * fs->sb.inode_table_start + inodenum * sizeof(struct inode), SEEK_SET);
        fwrite(&fs->inodeTable[inodenum], sizeof(struct inode), 1, fs->img);
        return 0; //scrittura inode avvenuta con successo
    }
    return -1; //l'inode non è in uso
}

int dir_lookup(struct filesystem *fs, struct inode *dir_inode, const char* name, struct inode *result){
    int entries_per_block = BLOCK_SIZE / sizeof(struct dirEntry); //numero di entry contenute in un blocco
    char buffer[BLOCK_SIZE]; //creiamo un buffer che conterrà il blocco letto volta per volta
    struct dirEntry *entries = (struct dirEntry*)buffer; //eseguiamo il casting del buffer come nel metodo precedente
    for (ui32 i=0; i<INODE_DIRECT; i++){ //scorriamo i blocchi diretti da leggere
        if(dir_inode->directBlocks[i]==0){
            continue; //se il blocco diretto è 0, salta al prossimo
        }
        if(read_block(fs->img, dir_inode->directBlocks[i], buffer) != 0){ //legge il blocco diretto, se riceve -1 allora il blocco non è leggibile
            return -1; //errore nella lettura del blocco
        }
        for(ui32 j=0; entries_per_block; j++){ //scorriamo tutte le entries nel blocco che abbiamo scritto nel nostro buffer
            if(strncmp(entries[j].fname, name, FNAME_LEN)==0){ //compariamo il nome dell'entry con quello passato come parametro
                *result = inode_read(fs, entries[j].inodeNum); //legge l'inode corrispondente al file trovato
                return 0; //file trovato con successo
            }
        }
    }
    return -1; //file non trovato
}

int dir_add_entry(struct filesystem *fs, inode_t dir_inode_num, const char *name, inode_t inodeNum){ //metodo che aggiungerà un file creato ad una directtory
    struct inode *dir_inode = &fs->inodeTable[dir_inode_num]; //prendiamo l'inode della directory
    char buffer[BLOCK_SIZE]; //creiamoun buffer che conterrà il blocco letto volta per volta
    struct dirEntry *entries = (struct dirEntry*)buffer; //eseguiamo il casting del buffer a struct dirEntry
    int entries_per_block = BLOCK_SIZE / sizeof(struct dirEntry); //numero di entry contenute in un blocco
    for(ui32 i=0; i<INODE_DIRECT; i++){ //scorriamo i blocchi diretti della directory, in ogni bloccoscorreremo le relative entries
        if(dir_inode->directBlocks[i]!=0){ //se il blocco diretto non è 0, quindi esiste
            if (read_block(fs->img,dir_inode->directBlocks[i], buffer)!=0) //legge il blocco diretto e se riceve -1, quindi il blocco non è leggibile, ritorna un errore
            {
                //il contenuto del blocco, quindi le entries finiscono così in entries, il quale è un array di struct dirEntry
                return -1; //il blocco non è leggibile
            }
            for(ui32 j=0; j< entries_per_block; j++){ //BLOCK_SIZE / sizeof(struct dirEntry) è il numero massimo di entry che possono essere contenute in un blocco
                if(entries[j].inodeNum==0){ //se l'inodeNum è 0, quindi l'entry è libera
                    strncpy(entries[j].fname, name, sizeof(entries[j].fname)); //copia il nome del file nella nuova entry della directory
                    entries[j].inodeNum=inodeNum; //imposta il riferimento all'inode nel file creato
                    if(write_block(fs->img, dir_inode->directBlocks[i], buffer)!=0){ //scrive il blocco diretto
                        //questo perchè il blocco aggiornato è ancora solo in locale
                        return -1; //errore nella scrittura del blocco
                    }
                    inode_write(fs, dir_inode_num); //persist the inode
                    return 0; //entry aggiunta con successo
                }
            }
        } else {
            block_t newBlock = block_alloc(fs); //allochiamo un nuovo blocco
            if(newBlock == (block_t)-1){
                return -1; //errore nell'allocazione del blocco
            }
            dir_inode->directBlocks[i] = newBlock; //impostiamo il nuovo blocco diretto nella directory
            memset(buffer, 0, BLOCK_SIZE); //inizializziamo tutte le entries a 0
            strncpy(entries[0].fname, name, sizeof(entries[0].fname)); //copia il nome del file nella dirEntry
            entries[0].inodeNum = inodeNum; //impostiamo l'inodeNum nella dirEntry
            if(write_block(fs->img, newBlock, buffer)!=0){
                return -1; //problema nella scrittura del blocco
            }
            inode_write(fs, dir_inode_num); //persist the inode
            return 0; //entry aggiunta con successo
        }
    }
    return -1; //nessuno spazio disponibile per nuove entry
}

int dir_remove_entry(struct filesystem *fs, struct inode *dir_inode, const char *name){
    char buffer[BLOCK_SIZE]; //creiamo un buffer che conterrà il blocco letto volta per volta
    struct dirEntry *entries = (struct dirEntry*)buffer; //eseguiamo nuovamente il casting del buffer come nel metodo precedente
    int entries_per_block = BLOCK_SIZE / sizeof(struct dirEntry); //numero di entry contenute in un blocco
    for(ui32 i=0; i<INODE_DIRECT; i++){ //scorriamo i blocchi diretti da leggere
        if(dir_inode->directBlocks[i]==0){
            continue; //se il blocco diretto è 0, salta al prossimo perchè non è allocato
        }
        if(read_block(fs->img,dir_inode->directBlocks[i],buffer)!=0){ //leggiamo il blocco, se la lettura avviene correttamente restituisce zero
            return -1; //errore nella lettura del blocco
        }
        for(ui32 j=0; j<entries_per_block; j++){ //leggiamo tutte le entries nel blocco
            if(strncmp(entries[j].fname, name, FNAME_LEN)==0){ //compariamo il nome
                memset(&entries[j], 0, sizeof(struct dirEntry)); //azzera l'entry trovata in posizione i
                if (write_block(fs->img, dir_inode->directBlocks[i], buffer)!=0){ //inserisce il blocco aggiornato al posto del precedente
                    //directBlocks[i] è il blocco della directory in cui si trova l'entry da rimuovere
                    return -1; //errore nell'aggiornamento del blocco
                }
                return 0; //se la riscrittura è andata bene restituisce zer0
            }
        }
    }
    return -1; //il file non è stato trovato
}

int dir_list_entries(struct filesystem *fs, struct inode *dir_inode){
    char buffer[BLOCK_SIZE];
    struct dirEntry *entries = (struct dirEntry*)buffer;
    int entries_per_block = BLOCK_SIZE / sizeof(struct dirEntry); //numero di entry contenute in un blocco
    printf("elenco dei file nella directory: \n");
    for( ui32 i=0; i<INODE_DIRECT; i++){
        if(dir_inode->directBlocks[i]==0){
            continue; //se il blocco diretto è 0, salta al prossimo perchè non è allocato
        }
        if(read_block(fs->img, dir_inode->directBlocks[i], buffer)!=0){
            return -1; //blocco letto non correttamente
        }
        for(ui32 j=0; j<entries_per_block; j++){
            if(entries[j].inodeNum != 0){
                printf("File: %s, Inode; %u\n",entries[j].fname, entries[j].inodeNum); //stampa i dettagli della entry
            }
        }
    }
    return 0; //operazione completata
}

int fs_create_file(struct filesystem *fs, struct inode *dir, const char *name, uint32_t type){
    inode_t new_inode = inode_alloc(fs); //allochiamo il nostro inode
    if (new_inode==(inode_t)-1) //error handling 
    {
        return -1; //errore nell'allocazione dell'inode
    }
    if(dir_add_entry(fs, dir->directBlocks[0], name, new_inode)!=0){ //tentiamo di aggiungere l'entry nella directory
        return -1; //errore nell'aggiunta del file nella directory
    }
    return 0; //come vediamo l'agguna nel file consiste nella creazione di un inode e nell'aggiunta del riferimento ad esso nella directory
}

int fs_delete_file(struct filesystem *fs, struct inode *dir,  const char * name){
    struct inode file_inode; //creiamo un inode temporaneo per leggere l'inode del file da eliminare
    if(dir_lookup(fs, dir, name, &file_inode)!=0){ //cerchiamo il file della directory
        return -1; //file non trovato
    }
    for(ui32 i=0; i<INODE_DIRECT; i++){
        if(file_inode.directBlocks[i]!=0){
            if(free_block(fs, file_inode.directBlocks[i])!=0){ //eliminiamo ogni blocco usato dal file
                return -1; //blocco non eliminato correttamente
            }
        }
    if(free_inode(fs, file_inode.isUsed)!=0){
        return -1; //inode non eliminato correttamente
    }
    if(dir_remove_entry(fs, dir, name)!=0){ //eliminiamo la entry dalla directory
        return -1; //entry non rimossa correttamente
    }
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
            printf("Scrittura blocco 20...\n");
            if (write_block(F, 20, writeData) == 0) {
                printf("Blocco scritto con successo.\n");

                char readData[BLOCK_SIZE];
                printf("Lettura blocco 20...\n");
                if (read_block(F, 20, readData) == 0) {
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
        struct filesystem *fs = open_fs("test.img", false, false);
        if (fs != NULL) {
            printf("File system aperto e letto con successo.\n");

            // Test allocazione inode
            printf("Test allocazione inode...\n");
            inode_t new_inode = inode_alloc(fs);
            if (new_inode != (inode_t)-1) {
                printf("Inode allocato: %u\n", new_inode);
                inode_read(fs, new_inode);
            } else {
                printf("Errore nell'allocazione dell'inode.\n");
            }

            // Test allocazione blocco
            printf("Test allocazione blocco...\n");
            block_t new_block = block_alloc(fs);
            if (new_block != (block_t)-1) {
                printf("Blocco allocato: %u\n", new_block);
            } else {
                printf("Errore nell'allocazione del blocco.\n");
            }

            // Test metodi directory
            printf("Test metodi directory...\n");
            inode_t dir_inode_num = inode_alloc(fs);
            if (dir_inode_num != (inode_t)-1) {
                printf("Inode directory allocato: %u\n", dir_inode_num);
                struct inode *dir_inode = &fs->inodeTable[dir_inode_num];

                // Aggiungi entry
                if (dir_add_entry(fs, dir_inode_num, "testfile.txt", new_inode) == 0) {
                    printf("Entry aggiunta con successo.\n");
                } else {
                    printf("Errore nell'aggiunta dell'entry.\n");
                }

                // Lista entries
                if (dir_list_entries(fs, dir_inode) == 0) {
                    printf("Entries listate con successo.\n");
                } else {
                    printf("Errore nel listare le entries.\n");
                }

                // Lookup entry
                struct inode result;
                if (dir_lookup(fs, dir_inode, "testfile.txt", &result) == 0) {
                    printf("Entry trovata: inode %u\n", result.isUsed ? dir_inode_num : 0);
                } else {
                    printf("Entry non trovata.\n");
                }

                // Rimuovi entry
                if (dir_remove_entry(fs, dir_inode, "testfile.txt") == 0) {
                    printf("Entry rimossa con successo.\n");
                } else {
                    printf("Errore nella rimozione dell'entry.\n");
                }
            } else {
                printf("Errore nell'allocazione dell'inode directory.\n");
            }

            // TODO: Close the filesystem if needed
        } else {
            printf("Errore nell'apertura del file system.\n");
        }
    } else {
        printf("Errore nell'inizializzazione del file system.\n");
    }
    return 0;
}
