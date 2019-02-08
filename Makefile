CC=			gcc
CFLAGS=		-g -Wall -std=gnu99
TARGETS=	shell

all:		$(TARGETS)

shell:		shell.c
			@echo Compiling shell.c...
			$(CC) $(CFLAGS) $< -o $(TARGETS)

clean:		
			@echo Removing the executable...
			rm $(TARGETS)
