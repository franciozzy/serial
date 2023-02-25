all:
	gcc -o serial serial.c -I/opt/homebrew/Cellar/libserialport/0.1.1/include -L/opt/homebrew/Cellar/libserialport/0.1.1/lib -lserialport
