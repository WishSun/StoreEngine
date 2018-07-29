SRC=$(wildcard ./*.c)
OBJ=$(patsubst %.c, %.o, $(SRC))
LIB=../../lib/libsignal_deal.a

$(LIB):$(OBJ)
	ar -cr $@ $^

./%.o:./%.c
	gcc -c $< -o $@

.PHONY:clean
clean:
	rm $(OBJ) $(LIB)
