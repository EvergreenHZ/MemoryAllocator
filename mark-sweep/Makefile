BIN=markandsweep

.PHONY : clean 

$(BIN) : main.c
	$(CC)  -g -std=gnu99  main.c -o $(BIN)

clean :
	rm -f $(BIN) *~

run : $(BIN)
	valgrind  --leak-check=yes ./$(BIN)

