# Felicia Chen-She 261044333
# Christine Pan 260986437

CC=gcc
CFLAGS=-g3

myshell: shell.c interpreter.c shellmemory.c pcb.c ready_queue.c kernel.c
	$(CC) $(CFLAGS) -D FRAME_STORE_SIZE=$(framesize) -D VAR_STORE_SIZE=$(varmemsize) -c -g shell.c interpreter.c shellmemory.c kernel.c pcb.c ready_queue.c
	$(CC) $(CFLAGS) -o myshell shell.o interpreter.o shellmemory.o kernel.o pcb.o ready_queue.o

clean: 
	rm myshell; rm *.o
