#include <stdlib.h>
#include "bf.h"
#include "ht_table.h"

#define FILE_NAME "data.db"


int main() {

    BF_Init(LRU);

    int status = HashStatistics(FILE_NAME);
    if (status != HT_OK) {
        BF_Close();
        exit(1);
    }


    BF_Close();

    return 0;
}
