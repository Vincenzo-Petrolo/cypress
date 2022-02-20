compile:
	gcc -O3 cipress.c -o cipress
test_compression:
	echo "cccciiiiaaaaoooo" > test_file
	./cipress -c test_file
	cat test_file.cpr
test_extract:
	rm test_file
	./cipress -x test_file.cpr
	cat test_file
clean_test:
	rm *.cpr