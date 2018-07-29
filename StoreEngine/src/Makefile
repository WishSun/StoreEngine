SRC=$(wildcard ./*.c)
OBJ=$(patsubst %.c, %.o, $(SRC))
BIN=./StoreEngine
CFLAG=-g -Wall

#-lrt : aio库
#-lpcap : libpcap库
#-lconfig : libconfig库
#-lzlog : libzlog库
#注意: 依赖其他库的库一定要放在被依赖库的前面，否则将编译出错
LFLAG=-L../lib -lctrl -lmylog -lqueue -lget_and_deal_data -lwrite_disk -lparse_configure_file -lsignal_deal -lconfig -lzlog -lpthread -lrt -lpcap 
	
$(BIN):$(OBJ)
	gcc $^ -o $@ $(LFLAG)

./%.o:./%.c
	gcc -c $< -o $@ $(CFLAG)

.PHONY:clean
clean:
	rm -rf $(BIN) $(OBJ)
