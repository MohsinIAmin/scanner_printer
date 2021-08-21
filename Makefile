
printer:
	gcc server_main.c multi_server.c get_ip_address.c process_request.c server_to_printer.c serve_admin.c server.h -o printer.out -Wall

scanner:
	gcc client_main.c printer_client.c admin.c scanner.c client.h -o scanner.out -Wall

clean:
	rm scanner
	rm printer
