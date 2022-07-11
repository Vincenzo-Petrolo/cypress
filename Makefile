compile:
	gcc -O3 cypress.c main.c cypress.h -o cypress
test_compression:
	echo "cccciiiiaaaaoooo" > test_file
	./cypress -c test_file
	hexdump -C test_file.cpr
test_extract:
	rm test_file
	./cypress -x test_file.cpr
	hexdump -C test_file
clean_test:
	rm *.cpr