hp:
	@echo "Compiling HEAP-FILE Main!";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/hp_main.c ./src/record.c ./src/hp_file.c -lbf -o ./examples/hp_main -O2;

ht:
	@echo "Compiling HASH-FILE Main!";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/ht_main.c ./src/record.c ./src/ht_table.c -lbf -o ./examples/ht_main -O2

st:
	@echo "Compiling HASH-FILE-STATISTICS Main!";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/ht_st_main.c ./src/record.c ./src/ht_table.c -lbf -o ./examples/ht_st_main -O2;