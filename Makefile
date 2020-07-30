
all:
	cd py && make  
	cd ext && make
	cd webkit && make
	cp py/build/WebKit.so pygtk/
	cp ext/build/webkit2_web_extension.so lib/webkitext/
	cp webkit/build/libwebview.so lib/
	bash -c "cd lib && ln -s libwebview.so libwebview.so.0"
	cp webkit/build/Pywebkit-0.1.gir lib/
	cp webkit/build/Pywebkit-0.1.typelib lib/


clean:
	cd py && make clean 
	cd ext && make clean 
	cd webkit && make clean
	-rm pygtk/WebKit.so
	-rm lib/webkitext/*.so	
	-rm lib/libwebview.so.0
	-rm lib/libwebview.so
	-find -name "__pycache__" -exec rm -rf {} \;


install: 
	cp lib/Pywebkit-0.1.typelib /usr/lib/x86_64-linux-gnu/girepository-1.0/
	cp lib/libwebview.so /usr/lib/x86_64-linux-gnu/
	bash -c "cd /usr/lib/x86_64-linux-gnu/ && ln -s libwebview.so libwebview.so.0"
	mkdir -p /usr/lib/x86_64-linux-gnu/webkitext
	cp lib/webkitext/webkit2_web_extension.so /usr/lib/x86_64-linux-gnu/webkitext/
	cp -r pygtk /usr/lib/python3/dist-packages/

# /usr/lib/[/x86_64-linux-gnu/]girepository-1.0/ for the typelib
# /usr/lib/x86_64-linux-gnu/ for the *.so Pywebkit.so
# /usr/lib/x86_64-linux-gnu/webkitext/ for the webstension.so

# pygtk to /usr/lib/python3/dist-packages/

uninstall:
	-rm /usr/lib/x86_64-linux-gnu/girepository-1.0/Pywebkit-0.1.typelib
	-rm /usr/lib/x86_64-linux-gnu/libwebview.so.0
	-rm /usr/lib/x86_64-linux-gnu/libwebview.so
	-rm /usr/lib/x86_64-linux-gnu/webkitext/webkit2_web_extension.so
	-rm -rf /usr/lib/python3/dist-packages/pygtk

