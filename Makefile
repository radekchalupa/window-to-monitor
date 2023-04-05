LDFLAGS = -lX11 -lXinerama -lm
CFLAGS = -O2 -Wall -Werror -pedantic -std=c99

objects = window-to-monitor.o
target = rc-window-to-monitor

%.o : %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c $<

$(target) : $(objects)
	$(CC) $(CFLAGS) $(objects) $(LDFLAGS) -o $(target) 

.PHONY : clean
clean: 
	rm -f $(objects) $(target)
