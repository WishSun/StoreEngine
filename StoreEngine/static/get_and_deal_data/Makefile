SRC=$(wildcard ./*.c)
OBJ=$(patsubst %.c, %.o, $(SRC))
LIB=../../lib/libget_and_deal_data.a

$(LIB):$(OBJ)
	ar -cr $@ $^ 

./%.o:./%.c
	gcc -c $< -o $@ -L../../lib -lpcap

.PHONY:clean
clean:
	rm $(OBJ) $(LIB)
