build: stdio.c
	gcc -fPIC -c stdio.c -o stdio.o
	gcc -shared stdio.o -o libso_stdio.so

exec: stdio.c 
	gcc -o stdio stdio.c -g

clean:
	rm libso_stdio.so stdio.o