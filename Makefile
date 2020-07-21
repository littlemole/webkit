
all:
	cd py && make  
	cd ext && make
	cd webkit && make
	cp py/build/WebKitDBus.so pygtk/
clean:
	cd py && make clean 
	cd ext && make clean 
	cd webkit && make clean
	-rm pygtk/WebKitDBus.so
proto:
	-rm build/gdbus.o
	g++ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -g -c gdbus.cpp -o build/gdbus.o
	g++ build/gdbus.o -lgio-2.0 -lgobject-2.0 -lglib-2.0 -o a.out
	./a.out 