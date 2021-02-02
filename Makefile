make:
	gcc *.c -o xlsxtosql -lxlsxio_read -lmariadb -g

run:
	./xlsxtosql
