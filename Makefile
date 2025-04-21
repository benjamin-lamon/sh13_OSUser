comp = gcc
server_port = 3210
player1 = Player1
port1 = 4569
player2 = Player2
port2 = 3570
player3 = Player3
port3 = 6969
player4 = Player4
port4 = 9696

all: server sh13

server: server.c 
	$(comp) -o server server.c 

sh13: sh13.c
	$(comp) -o sh13 -I/usr/include/SDL2 sh13.c `sdl2-config --cflags --libs` -lSDL2_image -lSDL2_ttf -lSDL2 -lpthread 

clean:
	rm -f server
	rm -f *.o
	rm -f sh13

run_server: server
	./server $(server_port)

run_client1: sh13
	./sh13 localhost $(server_port) localhost $(port1) $(player1)

run_client2: sh13
	./sh13 localhost $(server_port) localhost $(port2) $(player2)

run_client3: sh13
	./sh13 localhost $(server_port) localhost $(port3) $(player3)

run_client4: sh13
	./sh13 localhost $(server_port) localhost $(port4) $(player4)