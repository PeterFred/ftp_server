server.exe		: 	server.o 
	g++ ftpserver.cpp -lws2_32 -o server.exe 
			
server.o		:	ftpserver.cpp 
	g++ -c -Wall -O2 -fconserve-space ftpserver.cpp

clean:
	del *.o
	del *.exe
