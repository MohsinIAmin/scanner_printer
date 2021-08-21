#define MAX_CLIENT 100

int start_server(int server_port);
int get_ip_address(char server_ip[16]);
int process_request(int client_socket);
int server_to_printer(char buffer[8192]);
int serve_admin(int client_socket);