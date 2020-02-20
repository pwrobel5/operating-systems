CC = gcc -Wall -std=gnu99
out = -o
client_code = cw06_sysV_client.c
server_code = cw06_sysV_server.c
client_bin = cw06_sysV_client.x
server_bin = cw06_sysV_server.x

all:
	$(CC) $(out) $(client_bin) $(client_code)
	$(CC) $(out) $(server_bin) $(server_code)

clean:
	rm -f *.x
