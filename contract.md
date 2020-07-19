# contract


all msg use interface org.oha7.webkit.WebKitDBus on DBus

each instance uses a guid postfix in DBUS object paths

## python host request to view 

### request 

the python hosts sends function calls to the webview
with
 - object_path set to /org/oha7/webkit/WebKitDBus/controller/request/<guid>
 - signal_name set to the bound View javascript function name to call

and the GVariant* parameters is a gvariant tuple of two elements
 - a gvariant string holding the a msg uid
 - a gvrariant string holding a JSON encoded array of parameters

### response

 the vebview answers this request with a response message
 with
 - object_path set to /org/oha7/webkit/WebKitDBus/controller/response/<guid>
 - signal_name set to "response"

and the GVariant* parameters is a gvariant tuple of two elements
 - a gvariant string holding the the msg uid from the initial request
 - a gvrariant string holding a JSON decoded object with members result and exception.


## request request to python host

the webview sends function calls to the python host
with
 - object_path set to /org/oha7/webkit/WebKitDBus/view/request/<guid>
 - signal_name set to the bound Controller python function name to call

and the GVariant* parameters is a gvariant tuple of two elements
 - a gvariant string holding the a msg uid
 - a gvrariant string holding a JSON encoded array of parameters

### response

 the python host answers this request with a response message
 with
 - object_path set to /org/oha7/webkit/WebKitDBus/view/response/<guid>
 - signal_name set to "response"

and the GVariant* parameters is a gvariant tuple of two elements
 - a gvariant string holding the the msg uid from the initial request
 - a gvrariant string holding a JSON decoded object with members result and exception.
