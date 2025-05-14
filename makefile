main:
	rm -f main
	gcc main.c -o main -lcurl -lpthread

clean:
	rm -f main
