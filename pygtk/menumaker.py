
import gi
gi.require_versions({
    'Gtk':  '3.0'
})

from gi.repository import Gtk

import pygtk.WebKitDBus as WebKitDBus

class MenuCB(object):

    def __init__(self,cb):
        self.cb = cb

    def __call__(self,*args):
        WebKitDBus.run_async( self.cb(*args) )
        return False


class MenuMaker(object):

    def __init__(self,data):

        self.lookup = {}
        self.items = []

        for key in data:
            mainMenuItem = Gtk.MenuItem(key)

            subMenu = self.add_submenu_items(data[key])
        
            mainMenuItem.set_submenu(subMenu)

            self.lookup[key] = subMenu
            self.items.append(mainMenuItem)


    def populate(self,menubar):

        for item in self.items:
            menubar.append(item)


    def add_submenu_items(self,data):

        subMenu = Gtk.Menu()

        for entry in data:

            if callable(entry[1]):

                subMenuItem = Gtk.MenuItem( label=entry[0] )
                subMenuItem.action_target_value = entry[0]
                subMenu.append(subMenuItem)

                mcb = MenuCB(entry[1])
                subMenuItem.connect("activate", mcb )
                
            else:
                subMenuItem = Gtk.MenuItem( label=entry[0] )
                menu = self.add_submenu_items( entry[1] )
                subMenu.append(menu)

        return subMenu        
            

    def menu(self,name):

        return self.lookup[name]


    def popup(self,name,event):

        m = self.menu(name)
        Gtk.Menu.popup_at_pointer(m,event)    