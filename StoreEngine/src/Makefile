SRC=$(wildcard ./*.c)
OBJ=$(patsubst %.c, %.o, $(SRC))
BIN=./main
CFLAG=-g -Wall

#-lrt : aio库
#-lpcap : libpcap库
#-lconfig : libconfig库
#-lzlog : libzlog库
LFLAG=-L../lib -lconfig -lzlog -lpthread -lrt -lpcap

%.o:%.c
	gcc -c $< -o $@ $(CFLAG)
	
$(BIN):$(OBJ)
	gcc $^ -o $@ $(LFLAG)

.PHONY:clean
clean:
	rm -rf $(BIN) $(OBJ)
