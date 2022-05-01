PROJ=proj2
CFLAGS=-std=gnu99 -pthread -Wall -Werror -Wextra
CC=gcc
RM=rm -f

$(PROJ): $(PROJ).c
	$(CC) $(CFLAGS) -o $(PROJ) $(PROJ).c

clean:
	$(RM)*.o $(PROJ)
