all:
	cd src/mtkext && make  
	cd src/mtk && make && make static
	mkdir -p lib/mtkext
	cp src/mtkext/build/webkit2_web_extension.so lib/mtkext
	-ln -s ../lib/mtkext pymtk/mtkext
	cp src/mtk/build/libmtk.a lib/
	cp src/mtk/build/libmtk.so lib/
	-bash -c "cd lib && ln -s libmtk.so libmtk.so.0"
	cp src/mtk/build/Mtk-0.1.gir lib/
	cp src/mtk/build/Mtk-0.1.typelib lib/

cpp: all
	cd src/mtkcpp && make
	cp src/mtkcpp/build/libmtkcpp.a lib/


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
	cp lib/Mtk-0.1.typelib /usr/lib/x86_64-linux-gnu/girepository-1.0/
	cp lib/libmtk.so /usr/lib/x86_64-linux-gnu/
	bash -c "cd /usr/lib/x86_64-linux-gnu/ && ln -s libmtk.so libwebview.so.0"
	mkdir -p /usr/lib/x86_64-linux-gnu/mtkext
	cp lib/mtkext/webkit2_web_extension.so /usr/lib/x86_64-linux-gnu/mtkext/
	cp -r pymtk /usr/lib/python3/dist-packages/
	cp glade/*.xml /usr/share/glade/catalogs/

uninstall:
	-rm /usr/lib/x86_64-linux-gnu/girepository-1.0/Mtk-0.1.typelib
	-rm /usr/lib/x86_64-linux-gnu/libmtk.so.0
	-rm /usr/lib/x86_64-linux-gnu/libmtk.so
	-rm /usr/lib/x86_64-linux-gnu/mtkext/webkit2_web_extension.so
	-rm -rf /usr/lib/python3/dist-packages/pymtk
	-rm /usr/share/glade/catalogs/mtk.xml
	-rm /usr/share/glade/catalogs/pymtk.filetree.xml
	-rm /usr/share/glade/catalogs/pymtk.editor.xml
	-rm /usr/share/glade/catalogs/pymtk.web.xml
