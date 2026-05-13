C := gcc


%.so: %.o
	gcc -shared -o $@ $^

%.o: %.c
	gcc -c -o $@ $^

run:
	gcc -o main main.c

