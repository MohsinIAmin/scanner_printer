
all:
	gcc file_handel.c main.c network_printer.c printer_data.h -o scanner_printer

clean:
	rm scanner_printer
