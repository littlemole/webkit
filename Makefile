
all:
	cd py && make  
	cd ext && make
	cd webkit && make
	cp py/build/WebKit.so pygtk/
clean:
	cd py && make clean 
	cd ext && make clean 
	cd webkit && make clean
	-rm pygtk/WebKit.so
	-find -name "__pycache__" -exec rm -rf {} \;
