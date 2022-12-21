#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hp_file.h"


#define VALUE_CALL_OR_DIE(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}


#define POINTER_CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      return NULL;            \
    }                         \
  }


int HP_CreateFile(char *fileName) {

    /* Δημιουργία Heap File */
    VALUE_CALL_OR_DIE(BF_CreateFile(fileName))

    int fileDescriptor;

    /* Άνοιγμα Heap File */
    VALUE_CALL_OR_DIE(BF_OpenFile(fileName, &fileDescriptor))

    BF_Block *block;
    BF_Block_Init(&block);


    /* Allocation του 1ου Block του Heap File */
    VALUE_CALL_OR_DIE(BF_AllocateBlock(fileDescriptor, block))


    /* Αντιγραφή του HEAP_FILE_IDENTIFIER στο 1ο byte του 1ου Block */
    char *blockData = BF_Block_GetData(block);
    char heapFileID = HEAP_FILE_IDENTIFIER;
    memcpy(blockData, &heapFileID, sizeof(char));


    /* Μετακίνηση του δείκτη κατά 1 byte */
    blockData += sizeof(char);

    /* Αντιγραφή των μεταδεδομένων στο 1ο Block
     * Στον File Descriptor γίνεται ανάθεση της τιμής NONE αφού το Process ενδέχεται σε μελλοντικό άνοιγμα του αρχείου να αναθέσει κάποιο διαφορετικό File Descriptor */
    HP_info headerMetadata;
    headerMetadata.fileDescriptor = NONE;
    headerMetadata.blockIndex = HEADER_BLOCK;
    headerMetadata.lastBlock = HEADER_BLOCK;
    headerMetadata.maximumRecords = MAX_RECORDS;

    memcpy(blockData, &headerMetadata, sizeof(HP_info));

    BF_Block_SetDirty(block);

    VALUE_CALL_OR_DIE(BF_UnpinBlock(block))
    BF_Block_Destroy(&block);

    VALUE_CALL_OR_DIE(BF_CloseFile(fileDescriptor))

    return HP_OK;
}

HP_info *HP_OpenFile(char *fileName) {


    int fileDescriptor;
    POINTER_CALL_OR_DIE(BF_OpenFile(fileName, &fileDescriptor))


    BF_Block *block;
    BF_Block_Init(&block);


    POINTER_CALL_OR_DIE(BF_GetBlock(fileDescriptor, HEADER_BLOCK, block))
    char *blockData = BF_Block_GetData(block);


    char heapFileIdentifier = HEAP_FILE_IDENTIFIER;

    /* Το πρώτο byte του Heap File πρέπει να έχει αξία ιση με HASH_FILE_IDENTIFIER */
    if (memcmp(blockData, &heapFileIdentifier, sizeof(char)) != 0) {

        printf("First byte of HEADER-BLOCK should be : %c\n", HEAP_FILE_IDENTIFIER);

        /* Unpin του 1ου Block */
        POINTER_CALL_OR_DIE(BF_UnpinBlock(block))

        /* Κλείσιμο του εκάστοτε αρχείου */
        POINTER_CALL_OR_DIE(BF_CloseFile(fileDescriptor))

        /* Αποδέσμευση του BF_Block */
        BF_Block_Destroy(&block);

        return NULL;
    }

    /* Μετακίνηση του δείκτη κατά 1 byte */
    blockData += sizeof(char);


    /* Αντιγραφή των μεταδεδομένων του 1ου Block στη δομή HP_info */
    HP_info *headerMetadata = (HP_info *) malloc(sizeof(HP_info));
    memcpy(headerMetadata, blockData, sizeof(HP_info));


    /* Ρητή ανάθεση του File Descriptor που παρείχε το Process για το αρχείο */
    headerMetadata->fileDescriptor = fileDescriptor;


    /* Unpin του 1ου Block */
    POINTER_CALL_OR_DIE(BF_UnpinBlock(block))

    /* Αποδέσμευση του BF_Block */
    BF_Block_Destroy(&block);

    return headerMetadata;
}

int HP_CloseFile(HP_info *hp_info) {

    BF_Block *block;
    BF_Block_Init(&block);

    int fileDescriptor = hp_info->fileDescriptor;

    /* Ανάκτηση του 1ου Block του Heap File */
    VALUE_CALL_OR_DIE(BF_GetBlock(fileDescriptor, HEADER_BLOCK, block))

    /* Ανάκτηση και μετακίνηση του δείκτη κατά 1 byte */
    char *blockData = BF_Block_GetData(block) + sizeof(char);

    HP_info headerMetadata;
    memcpy(&headerMetadata, blockData, sizeof(HP_info));

    /* Έλεγχος για τον αν η δομή HP_info* hp_info διαφέρει με τα μεταδεδομένα του 1ου Block του Heap file */
    if (headerMetadata.lastBlock != hp_info->lastBlock || headerMetadata.fileDescriptor != hp_info->fileDescriptor) {

        headerMetadata.lastBlock = hp_info->lastBlock;
        headerMetadata.fileDescriptor = fileDescriptor;

        memcpy(blockData, &headerMetadata, sizeof(HP_info));

        BF_Block_SetDirty(block);
    }


    /* Unpin του 1ου Block του Heap File */
    VALUE_CALL_OR_DIE(BF_UnpinBlock(block))

    /* Αποδέσμευση του BF_Block */
    BF_Block_Destroy(&block);

    /* Κλείσιμο του Heap File */
    VALUE_CALL_OR_DIE(BF_CloseFile(fileDescriptor))

    /* Αποδέσμευση μνήμης εφόσον η δομή HP_info* hp_info δεσμεύθηκε δυναμικά */
    free(hp_info);

    return HP_OK;
}

int HP_InsertEntry(HP_info *hp_info, Record record) {

    BF_Block *block;
    BF_Block_Init(&block);
    int fileDescriptor = hp_info->fileDescriptor;

    /* Στην περίπτωση που το τελευταίο Block του Heap File δεν είναι το 1o Block */
    if (hp_info->lastBlock != HEADER_BLOCK) {

        /* Ανάκτηση του τελευταίου Block του Heap File */
        VALUE_CALL_OR_DIE(BF_GetBlock(fileDescriptor, hp_info->lastBlock, block))

        char *blockData = BF_Block_GetData(block) + BF_BLOCK_SIZE - sizeof(HP_block_info);

        /* Ανάκτηση των μεταδεδομένων του τελευταίου Block του Heap File */
        HP_block_info blockMetaData;
        memcpy(&blockMetaData, blockData, sizeof(HP_block_info));

        /* Το εκάστοτε Block που ανακτήθηκε έχει αρκετό χώρο για την εισαγωγή του εκάστοτε Record */
        if (blockMetaData.totalRecords < hp_info->maximumRecords) {

            /* Ανανέωση των μεταδεδομένων του τελευταίου Block του Heap File */
            blockMetaData.totalRecords += 1;
            memcpy(blockData, &blockMetaData, sizeof(HP_block_info));

            /* Αντιγραφή του εκάστοτε Record στην κατάλληλη θέση του Block */
            blockData = BF_Block_GetData(block) + ((blockMetaData.totalRecords - 1) * sizeof(Record));
            memcpy(blockData, &record, sizeof(Record));


            BF_Block_SetDirty(block);

            VALUE_CALL_OR_DIE(BF_UnpinBlock(block))
            BF_Block_Destroy(&block);
        }

            /* Το εκάστοτε Block που ανακτήθηκε ΔΕΝ έχει αρκετό χώρο για την εισαγωγή του εκάστοτε Record */
        else {


            BF_Block *newBlock;
            BF_Block_Init(&newBlock);

            /* Allocation ενός νέου Block */
            VALUE_CALL_OR_DIE(BF_AllocateBlock(fileDescriptor, newBlock))

            /* Ανανέωση του τελευταίου Block της δομής HP_info */
            hp_info->lastBlock += 1;

            /* Ανανέωση του εκάστοτε Block που ανακτήθηκε και ΔΕΝ ΕΙΧΕ αρκετό χώρο για την εισαγωγή του εκάστοτε Record */
            blockMetaData.nextBlock = hp_info->lastBlock;
            blockData = BF_Block_GetData(block) + BF_BLOCK_SIZE - sizeof(HP_block_info);
            memcpy(blockData, &blockMetaData, sizeof(HP_block_info));

            BF_Block_SetDirty(block);

            /* Αρχικοποίηση και αντιγραφή των μεταδεδομένων του νέου τελευταίου Block του Heap File */
            blockData = BF_Block_GetData(newBlock) + BF_BLOCK_SIZE - sizeof(HP_block_info);
            blockMetaData.totalRecords = 1;
            blockMetaData.nextBlock = NONE;
            memcpy(blockData, &blockMetaData, sizeof(HP_block_info));

            /* Αντιγραφή του εκάστοτε Record στην αρχή του νέου τελευταίου Block του Heap File */
            blockData = BF_Block_GetData(newBlock);
            memcpy(blockData, &record, sizeof(Record));

            BF_Block_SetDirty(newBlock);

            VALUE_CALL_OR_DIE(BF_UnpinBlock(block))
            BF_Block_Destroy(&block);

            VALUE_CALL_OR_DIE(BF_UnpinBlock(newBlock))
            BF_Block_Destroy(&newBlock);
        }

    }


        /* Στην περίπτωση που το τελευταίο Block του Heap File ΕΊΝΑΙ το 1o Block */
    else {

        /* Allocation ενός νέου Block */
        VALUE_CALL_OR_DIE(BF_AllocateBlock(fileDescriptor, block))

        /* Αρχικοποίηση και αντιγραφή των μεταδεδομένων του νέου τελευταίου Block του Heap File */
        char *dataPointer = BF_Block_GetData(block) + BF_BLOCK_SIZE - sizeof(HP_block_info);
        HP_block_info blockMetaData;
        blockMetaData.totalRecords = 1;
        blockMetaData.nextBlock = NONE;
        memcpy(dataPointer, &blockMetaData, sizeof(HP_block_info));

        /* Αντιγραφή του εκάστοτε Record στην αρχή του Block */
        dataPointer = BF_Block_GetData(block);
        memcpy(dataPointer, &record, sizeof(Record));

        BF_Block_SetDirty(block);

        /* Ανανέωση του τελευταίου Block της δομής HP_info */
        hp_info->lastBlock = 1;


        VALUE_CALL_OR_DIE(BF_UnpinBlock(block))
        BF_Block_Destroy(&block);

    }

    return hp_info->lastBlock;
}

int HP_GetAllEntries(HP_info *hp_info, int value) {

    BF_Block *block;
    BF_Block_Init(&block);

    int fileDescriptor = hp_info->fileDescriptor;

    for (int i = 1; i <= hp_info->lastBlock; i++) {

        /* Ανάκτηση του κατάλληλου Block */
        VALUE_CALL_OR_DIE(BF_GetBlock(fileDescriptor, i, block))

        /* Αντιγραφή των μεταδεδομένων του Block στην proxy δομή HP_block_info blockMetadata */
        char *blockData = BF_Block_GetData(block) + BF_BLOCK_SIZE - sizeof(HP_block_info);
        HP_block_info blockMetadata;
        memcpy(&blockMetadata, blockData, sizeof(HP_block_info));

        Record record;
        blockData = BF_Block_GetData(block);
        for (int j = 0; j < blockMetadata.totalRecords; j++) {

            memcpy(&record, blockData, sizeof(Record));

            if (record.id == value)
                printRecord(record);

            /* Μετακίνηση του δείκτη στο επόμενο Record */
            blockData += sizeof(Record);
        }

        VALUE_CALL_OR_DIE(BF_UnpinBlock(block))
    }


    BF_Block_Destroy(&block);

    /* Επιστροφή του lastBlock αφού στο πλαίσιο του Heap File ανακτώνται αναγκαστικά ολα τα Blocks.
     * Στην περίπτωση που το Heap File δεν έχει Records θα επιστραφεί 0 αφού κατά την αρχικοποίηση του 1ου Block του Heap File ορίζεται οτι το τελευταίο Block είναι το Block 0 */
    return hp_info->lastBlock;
}


int completeHeapFile(HP_info *hp_info) {


    printf("##########\n");
    printf("Complete Heap File\n");
    printf("Max Records per Block : %d\n", hp_info->maximumRecords);
    printf("Total Blocks of Heap File : %d\n", hp_info->lastBlock + 1);

    BF_Block *block;
    BF_Block_Init(&block);

    int fileDescriptor = hp_info->fileDescriptor;

    for (int i = 1; i <= hp_info->lastBlock; i++) {

        /* Ανάκτηση του κατάλληλου Block */
        VALUE_CALL_OR_DIE(BF_GetBlock(fileDescriptor, i, block))

        /* Αντιγραφή των μεταδεδομένων του Block στην proxy δομή HP_block_info blockMetadata */
        char *blockData = BF_Block_GetData(block) + BF_BLOCK_SIZE - sizeof(HP_block_info);
        HP_block_info blockMetadata;
        memcpy(&blockMetadata, blockData, sizeof(HP_block_info));

        printf("##########\n");
        printf("Block %d has %d Records\n", i, blockMetadata.totalRecords);


        Record record;
        blockData = BF_Block_GetData(block);
        for (int j = 0; j < blockMetadata.totalRecords; j++) {

            memcpy(&record, blockData, sizeof(Record));

            printRecord(record);


            /* Μετακίνηση του δείκτη στο επόμενο Record */
            blockData += sizeof(Record);
        }

        VALUE_CALL_OR_DIE(BF_UnpinBlock(block))

        printf("##########\n\n");
    }


    BF_Block_Destroy(&block);


    return HP_OK;
}




