compile:
	gcc -O3 cypress.c -o cypress
test_compression:
	echo "cccciiiiaaaaoooo" > test_file
	./cypress -c test_file
	cat test_file.cpr
test_extract:
	rm test_file
	./cypress -x test_file.cpr
	cat test_file
clean_test:
	rm *.cpr