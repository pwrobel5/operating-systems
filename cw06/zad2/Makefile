CC = gcc -Wall -std=gnu99
out = -o
link = -lrt
client_code = cw06_posix_client.c
server_code = cw06_posix_server.c
client_bin = cw06_posix_client.x
server_bin = cw06_posix_server.x

all:
	$(CC) $(out) $(client_bin) $(client_code) $(link)
	$(CC) $(out) $(server_bin) $(server_code) $(link)

clean:
	rm -f *.x