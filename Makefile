all:
	cd src/mtkext && make  
	cd src/mtk && make && make static
	cd src/mtkcpp && make
	mkdir -p lib/mtkext
	cp src/mtkext/build/webkit2_web_extension.so lib/mtkext
	-ln -s ../lib/mtkext pymtk/mtkext
	cp src/mtk/build/libmtk.a lib/
	cp src/mtk/build/libmtk.so lib/
	-bash -c "cd lib && ln -s libmtk.so libmtk.so.0"
	cp src/mtkcpp/build/libmtkcpp.a lib/
	cp src/mtk/build/Mtk-0.1.gir lib/
	cp src/mtk/build/Mtk-0.1.typelib lib/


clean:
	cd src/mtkext && make clean 
	cd src/mtk && make clean 
	cd src/mtkcpp && make clean
	-rm pymtk/mtkext
	-rm lib/mtkext/*.so	
	-rm lib/libmtk.so.0
	-rm lib/libmtk.so
	-rm lib/libmtkcpp.a
	-rm lib/*.gir
	-rm lib/*.typelib
	-find -name "__pycache__" -exec rm -rf {} \;
	-find -name "*~" -exec rm -rf {} \;


install: 
	cp lib/Pywebkit-0.1.typelib /usr/lib/x86_64-linux-gnu/girepository-1.0/
	cp lib/libwebview.so /usr/lib/x86_64-linux-gnu/
	bash -c "cd /usr/lib/x86_64-linux-gnu/ && ln -s libwebview.so libwebview.so.0"
	mkdir -p /usr/lib/x86_64-linux-gnu/webkitext
	cp lib/webkitext/webkit2_web_extension.so /usr/lib/x86_64-linux-gnu/webkitext/
	cp -r pygtk /usr/lib/python3/dist-packages/
	cp glade/pywebkit.xml /usr/share/glade/catalogs/
	cp glade/pygtk.filetree.xml /usr/share/glade/catalogs/
	cp glade/pygtk.editor.xml /usr/share/glade/catalogs/

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
	-rm /usr/share/glade/catalogs/pywebkit.xml
	-rm /usr/share/glade/catalogs/pygtk.filetree.xml
	-rm /usr/share/glade/catalogs/pygtk.editor.xml
