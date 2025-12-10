#include "../FS.h"
#include <stdio.h>

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
                {
                    struct inode tmp;
                    if (inode_read(fs, new_inode, &tmp) == 0) {
                        printf("  inode.isUsed=%u size=%u\n", tmp.isUsed, tmp.size);
                    } else {
                        printf("  Errore nella lettura dell'inode %u\n", new_inode);
                    }
                }
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
                inode_t found_num;
                if (dir_lookup(fs, dir_inode, "testfile.txt", &result, &found_num) == 0) {
                    printf("Entry trovata: inode %u\n", found_num);
                } else {
                    printf("Entry non trovata.\n");
                }

                // Rimuovi entry
                if (dir_remove_entry(fs, dir_inode, "testfile.txt") == 0) {
                    printf("Entry rimossa con successo.\n");
                } else {
                    printf("Errore nella rimozione dell'entry.\n");
                }

                // Test fs_create_file
                if (fs_create_file(fs, dir_inode_num, "created_file.txt", 0) == 0) {
                    printf("File creato con successo.\n");
                } else {
                    printf("Errore nella creazione del file.\n");
                }

                // Lista entries dopo creazione
                if (dir_list_entries(fs, dir_inode) == 0) {
                    printf("Entries listate dopo creazione.\n");
                }

                // Test fs_delete_file
                if (fs_delete_file(fs, dir_inode, "created_file.txt") == 0) {
                    printf("File eliminato con successo.\n");
                } else {
                    printf("Errore nell'eliminazione del file.\n");
                }

                // Lista entries dopo eliminazione
                if (dir_list_entries(fs, dir_inode) == 0) {
                    printf("Entries listate dopo eliminazione.\n");
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
