# Ler Theng Loo (s5212872)
# 2803ICT - Assignment 2

# Makefile

all: server client

server: server.o
	cc server.o -lpthread -o server

client: client.o 
	cc client.o -lpthread -o client

server.o: server.c include.h
	cc -c -lpthread server.c

client.o: client.c include.h
	cc -c -lpthread client.c
