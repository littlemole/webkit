import os.path 

import gi
gi.require_versions({
    'Gtk':  '3.0',
    'WebKit2' : '4.0'
})
from gi.repository import Gio, Gtk, Gdk, GObject, GLib, WebKit2
from gi.repository.WebKit2 import WebView, WebContext


class WebView2(WebKit2.WebView):

    __gtype_name__ = "mtkWebView"

    dir = os.path.dirname(__file__)

#    __gsignals__ = {
#        'changed': (GObject.SIGNAL_RUN_FIRST, None,())
#    }

#    language = GObject.Property(nick="language",type=str, default="")    


    def __init__(self,*args,**kargs):

        WebKit2.WebView.__init__(self)

        self.path = ""
        self.uid = "no more uids"


    def onChanged(self,*args):

        self.emit("changed")

    def load_local_uri(self,url):

        path = url if not url is None else "index.html"

        fp = ""
        if path[0:1] == "/":
            fp = "file://" + path
        else:
            fp = "file://" + WebView2.dir + "/" + path

        print("LOAD: " + fp)
        self.load_uri(fp)


def init_ext(webContext):

    d = os.path.dirname(__file__) + "/mtkext" 
    print("init ext:" +d)
    webContext.set_web_extensions_directory(d)
    webContext.set_web_extensions_initialization_user_data( GLib.Variant.new_string("no more uid") )


ctx = WebContext.get_default()

ctx.connect( "initialize-web-extensions", init_ext )
