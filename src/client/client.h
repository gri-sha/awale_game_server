#ifndef CLIENT_H
#define CLIENT_H

void app(const char *address, const char *name);
int init_connection(const char *address);
void end_connection(int sock);
int read_from_server(int sock, char *buffer);
void write_to_server(int sock, const char *buffer);
void process_command(int sock, const char *input);

#endif