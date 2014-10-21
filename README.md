webkit
======

academic example of using embedded webkit and python extensions in a gnome.3 python application.

the joy is the direct interaction between python as application controller using
webkit to display HTML as the application view with javascript in the view being able to call
back into the python controller. in other words this is an example for Javascript <-> Python Interop.

Implementing the Marshaling glue was great fun.

build:

  run make

afterwards:

  bash test.sh
  
to launch the test application implemented in curl.py and curl.html.
