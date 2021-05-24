CFLAGS?=-g -Wall -Wno-unused-value

all: caltrain 

caltrain: caltrain-runner.c caltrain.c pintos_thread.h
	$(CC) $(CFLAGS) -o caltrain caltrain-runner.c -lpthread 


run: caltrain 
	./caltrain

clean:
	$(RM) caltrain 
	$(RM) -r *.dSYM
