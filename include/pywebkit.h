#ifndef __MO_WEBKIT_H__
#define __MO_WEBKIT_H__

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>


#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _PywebkitWebview		    PywebkitWebview;
typedef struct _PywebkitWebviewClass	PywebkitWebviewClass;

struct _PywebkitWebview {
/*< public >*/
    WebKitWebView parent;
    gchar* uid;
    gchar* localpath;
};


struct _PywebkitWebviewClass {
/*< public >*/
    WebKitWebViewClass parent;
    gchar* dir;
};

GType		pywebkit_webview_get_type	() G_GNUC_CONST;

PywebkitWebview*	pywebkit_webview_new		();

void	pywebkit_webview_load_local_uri(PywebkitWebview *web, const gchar* localpath);

void  pywebkit_webview_class_set_dir(PywebkitWebviewClass* clazz, const gchar* dir);

#ifdef __cplusplus
}
#endif

#define PY_WEBKIT_TYPE		\
    (pywebkit_webview_get_type())
#define PY_WEBKIT(o)			\
    (G_TYPE_CHECK_INSTANCE_CAST ((o), PY_WEBKIT_TYPE, PywebkitWebview))
#define PY_WEBKIT_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_CAST ((c), PY_WEBKIT_TYPE, PywebkitWebviewClass))
#define PY_IS_WEBKIT(o)		\
    (G_TYPE_CHECK_INSTANCE_TYPE ((o), PY_WEBKIT_TYPE))
#define PY_IS_PYBKIT_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_TYPE ((c),  PY_WEBKIT_TYPE))
#define PY_WEBKIT_GET_CLASS(o)	\
    (G_TYPE_INSTANCE_GET_CLASS ((o), PY_WEBKIT_TYPE, PywebkitWebviewClass))


#endif /* __MO_WEBKIT_H__ */
