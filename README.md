Webkit
======


academic example of using embedded webkit and python extensions in a gnome.3 python application.

the joy is the direct interaction between python as application controller using
webkit to display HTML as the application view with javascript in the view being able to call
back into the python controller. in other words this is an example for Javascript <-> Python Interop.

Implementing the Marshaling glue was great fun.

build:

```make clean && make```

afterwards:

```run.sh examples/curl/curl.py```
  
to launch any of the examples in the examples directory.

to install:

```sudo make install```

which will install into system directories and allows to run without run.sh, eg:

```python3 examples/diff/diff.py ```


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
