SRC=$(wildcard ./*.c)
OBJ=$(patsubst %.c, %.o, $(SRC))
LIB=../../lib/libparse_configure_file.a

$(LIB):$(OBJ)
	ar -cr $@ $^

./%.o:./%.c
	gcc -c $< -o $@

.PHONY:clean
clean:
	rm $(OBJ) $(LIB)
