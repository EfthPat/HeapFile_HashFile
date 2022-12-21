#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "bf.h"
#include "hp_file.h"

#define RECORDS_NUM 4269
#define FILE_NAME "data.txt"

int main() {

    BF_Init(LRU);

    /* Κατά τη 2η + εκτέλεση του προγράμματος θα προκύψει το σφάλμα : BF Error: The file is already being used
     * Το παραπάνω οφείλεται στο οτι το αρχείο υπάρχει ηδη και έτσι θα αποτύχει η δημιουργία του, συνεπώς κατά τη 2η + εκτέλεση αγνοήστε το */
    HP_CreateFile(FILE_NAME);

    HP_info *info = HP_OpenFile(FILE_NAME);
    if (info == NULL) {
        BF_Close();
        exit(1);
    }

    Record record;
    srand(time(NULL));

    for (int i = 0; i < RECORDS_NUM; i++) {
        record = randomRecord();

        int blockIndex = HP_InsertEntry(info, record);
        if (blockIndex == HP_ERROR) {
            BF_Close();
            exit(1);
        }

        printf("##########\n");
        printf("Inserted Record\n");
        printRecord(record);
        printf("in Block %d\n", blockIndex);
        printf("##########\n\n");
    }

    int value = rand() % RECORDS_NUM;
    printf("Looking for value %d\n", value);

    int blocksRequested = HP_GetAllEntries(info, value);
    if (blocksRequested == HP_ERROR) {
        BF_Close();
        exit(1);
    }

    printf("%d Block(s) requested for value %d\n", blocksRequested, value);


    /* Προαιρετικά */
    int status = completeHeapFile(info);
    if (status == HP_ERROR) {
        BF_Close();
        exit(1);
    }

    status = HP_CloseFile(info);
    if (status == HP_ERROR) {
        BF_Close();
        exit(1);
    }

    BF_Close();

    return 0;
}
