Webkit
======


academic example of using embedded webkit and python extensions in a gnome.3 python application.

the joy is the direct interaction between python as application controller using
webkit to display HTML as the application view with javascript in the view being able to call
back into the python controller. in other words this is an example for Javascript <-> Python Interop.

Implementing the Marshaling glue was great fun.

Then came the update of webkit now using the (more secure) process based model, where all 
the HTML related processing happens in a dedicated browser (per webview). That changes our
challenge from in-process to out of process. yummy.


# pre-requisites

## debian/ubuntu
```
sudo apt install build-essential 
sudo apt install python3-dev python-gobject-2 python-gobject-2-dev libwebkit2gtk-4.0-dev gobject-introspection libgirepository1.0-dev libgtksourceview-4-dev libgtksourceview-4-0 libtool-bin
``` 
## fedora
```
sudo dnf install @development-tools
sudo dnf install python3-devel gtk3-devel webkit2gtk3-devel gobject-introspection-devel gtksourceview4 gtksourceview4-devel
```

# components

- mtkext shared library - a webkit extension library to inject into the webkit viewer processes
- pymtk - a python library with Gtk helpers
- mtkglue - header only cpp library to implement gobject/gtk objects
- mtk gtk shared library - extensions to WebView, SourceView and TreeView Gtk widgets plus some Git support
- mtkcpp static library - cpp glue to build native gtk apps using mtk in c++

# directory layout

- examples
    - python - GTK UI using embedded webkit from python
    - native - GTK UI using embedded webkit from native C++
- src
    - mtkext - the webkit extension library, C++
    - mtk - custom gtk widgets. C interface, C++ implementation
    - mtkcpp - static cpp library - C++
- include
    - mtk - custom gtk widgets includes (plain C)
    - mtkcpp - headers for the static cpp library (C++)
    - glue - various single header "minilibraries" to ease implementation (C++)
- lib
    - mtkext
        - mtk_web_extension.so
    - libmtk.so
    - libmtk-0.1.gir
    - libmtk-0.1.typelib
    - libmtkcpp.a
- pymtk - python directory with pure and native (pywebkit.so) python modules
    - webkitext -- symlink to lib/wekbitext
    - glade - glade catalog files for python based widgets
- glade - glade UI builder catalog files for widgets


# install
```
git clone https://github.com/littlemole/webkit.git
cd webkit
make
``` 


# run example GUI apps

git diff viewer:
``` ./run.sh examples/python/diff/diff.py ```

git dot graph builder:
``` ./run.sh examples/python/dot/dot.py ```


optional: install into system default lib folders (allows to avoid run.sh)

```
make install
```

use ```make uninstall``` to remove an installation
