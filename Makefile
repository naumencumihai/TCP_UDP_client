# Protocoale de Comunicatii - Tema 2 2022
# Naumencu Mihai 336CA

all : server subscriber

# compile server.c
server: server.c map.c -lm

# compile subscriber.c
subscriber: subscriber.c map.c -lm

# remove compiled files
clean:
	rm -f server subscriber