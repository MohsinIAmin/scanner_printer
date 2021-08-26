
printer:
	gcc server_main.c multi_server.c get_ip_address.c process_request.c server_to_printer.c serve_admin.c get_printserver.c print.h server.h -o printer.out -Wall

scanner:
	gcc client_main.c printer_client.c admin.c scanner.c client.h -o scanner.out -Wall

printd:
	gcc printd.c demonize.c get_printserver.c ipp.h print.h -lpthread -Wall -o printd

clean:
	rm scanner
	rm printer
