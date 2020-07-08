from gi.repository import GLib

import dbus
import dbus.service
import dbus.mainloop.glib
import pprint

def make_variant(arg):

    if isinstance(arg, int):
        return GLib.Variant('x',arg)

    if isinstance(arg, str):
        return GLib.Variant('s',arg)

    if isinstance(arg, bool):
        return GLib.Variant('b',arg)

    if isinstance(arg, (list,tuple)):#
        # builder = GLib.VariantBuilder(GLib.VariantType.new("av"))
        r = []
        n = "("
        for a in arg:
            v = make_variant(a)
            pprint.pprint(v)    
            #builder.add_value(v)
            r.append(v)
            n += "v"
        n += ")"
        return GLib.Variant(n,r)

    if isinstance(arg, dict):
        d = GLib.VariantDict.new()

        #builder = GLib.VariantBuilder(GLib.VariantType.new("a{sv}"))
        for key in arg:
            value = arg[key]
            #k = make_variant(key)
            v = make_variant(value)

            #pprint.pprint(k)      
            pprint.pprint(v)    
            d.insert_value(key,v)
#            d = GLib.Variant.new_dict_entry(key,v)
#            pprint.pprint(d)      
 #           builder.add_value(d)
        ret = d.end()
        pprint.pprint(ret)      
        return ret

def make_dbus(arg):

    if isinstance(arg, int):
        return arg

    if isinstance(arg, str):
        return arg

    if isinstance(arg, bool):
        return arg

    if isinstance(arg, (list,tuple)):
        ret = []
        for a in arg:
            v = make_variant(a)
            v.append(v)
        return ret

    if isinstance(arg, dict):
        ret = {}
        for key in arg:
            value = arg[key]
            v = make_variant(value)
            ret[key] = v

        pprint.pprint(ret)      

        return ret

class TestObject(dbus.service.Object):
   # def __init__(self, conn, object_path='/com/example/TestService/object'):
   #     dbus.service.Object.__init__(self, conn, object_path)

    @dbus.service.signal('com.example.TestService',signature='a{sv}x')
    def HelloSignal(self, message,value):
        # The signal is emitted when this method exits
        # You can have code here if you wish
        pass

    @dbus.service.method('com.example.TestService')
    def emitHelloSignal(self):
        #you emit signals by calling the signal's skeleton method
        print ("emit signal")
#        self.HelloSignal( "hu",42)#{ "keyy" : "hu", "values" : [1,2,3] },42)#, 'values' : [1,2,3] },42)

        data = { "keyy" : "hu", "index" : { "subobj": [4,3], 'subkey': [4711]}}
        #v = make_dbus(data) #GLib.Variant("a{s*}", { "keyy" : "hu", "index" : { "subobj": [4,3]}} )
        ##pprint.pprint(v)      
        self.HelloSignal( data , 42)# { "keyy" : "hu", "index" : { "subobj": [4,3]}},42)#, 'values' : [1,2,3] },42)
        return 'Signal emitted'

    @dbus.service.method("com.example.TestService",
                         in_signature='', out_signature='')
    def Exit(self):
        loop.quit()

if __name__ == '__main__':
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    session_bus = dbus.SessionBus()
    name = dbus.service.BusName('com.example2.TestService', session_bus)
    object = TestObject(session_bus,'/com/example/TestService/object')

    loop = GLib.MainLoop()
    print ("Running example signal emitter service.")
    loop.run()
