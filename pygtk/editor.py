import os.path 

import gi
gi.require_versions({
    'Gtk':  '3.0',
    'Pywebkit': '0.1'
})
#gi.require_version("GtkSource", "4.0")
from gi.repository import Gio, Gtk, Gdk, GObject, GLib, Pywebkit, GtkSource
from gi.repository.Pywebkit import Webview 
from gi.repository.GtkSource import View
from pygtk.bind import synced,idle_add
from pygtk.ui import UI,DirectoryTree,radio_group
from pygtk.git import Git, GitFile
from pygtk import WebKit
import pygtk

class Editor(object):

    def __init__(self,sourceView,monospace=True):

        self.path = ""
        self.sourceView = sourceView
        self.monospace = monospace
        self.sourcefile = GtkSource.File()
        self.lang_manager = GtkSource.LanguageManager()

        if self.monospace:
            self.sourceView.set_monospace(True)


    def load(self,file):

        self.path = file
        buffer = self.sourceView.get_buffer()        
                
        self.sourcefile.set_location(Gio.File.new_for_path(file))
        buffer.set_language(self.lang_manager.get_language("dot"))
        loader = GtkSource.FileLoader.new(buffer, self.sourcefile)

        loader.load_async(0, None, None, None, None, None)

    
    def save(self):

        buffer = self.sourceView.get_buffer()        

        saver = GtkSource.FileSaver.new(buffer, self.sourcefile)
        saver.save_async(0, None, None, None, None, None)


    def saveAs(self,file):

        buffer = self.sourceView.get_buffer()        

        targetfile = Gio.File.new_for_path(file)

        saver = GtkSource.FileSaver.new_with_target(buffer, self.sourcefile, targetfile)
        saver.save_async(0, None, None, None, None, None)


    def onChanged(self,cb):

        buffer = self.sourceView.get_buffer()        
        buffer.connect( "changed", cb )


    def get_text(self):

        buffer = self.sourceView.get_buffer()        
        return buffer.get_text( buffer.get_start_iter(), buffer.get_end_iter(),False)


    def set_text(self,txt):

        buffer = self.sourceView.get_buffer()        
        return buffer.set_text( txt, -1)
