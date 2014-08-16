#ifndef __MO_WEBKIT_H__
#define __MO_WEBKIT_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

#define PY_WEBKIT_TYPE		\
    (py_webkit_get_type())
#define PY_WEBKIT(o)			\
    (G_TYPE_CHECK_INSTANCE_CAST ((o), PY_WEBKIT_TYPE, PyWebKit))
#define PY_WEBKIT_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_CAST ((c), PY_WEBKIT_TYPE, PyWebKitClass))
#define PY_IS_WEBKIT(o)		\
    (G_TYPE_CHECK_INSTANCE_TYPE ((o), PY_WEBKIT_TYPE))
#define PY_IS_PYBKIT_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_TYPE ((c),  PY_WEBKIT_TYPE))
#define PY_WEBKIT_GET_CLASS(o)	\
    (G_TYPE_INSTANCE_GET_CLASS ((o), PY_WEBKIT_TYPE, PyWebKitClass))

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _PyWebKit		    PyWebKit;
typedef struct _PyWebKitClass		PyWebKitClass;

struct _PyWebKit {
/*< public >*/
    WebKitWebView parent;
};

struct _PyWebKitClass {
    WebKitWebViewClass parent;
};

GType		py_webkit_get_type	() G_GNUC_CONST;

PyWebKit*	py_webkit_new		();


#ifdef __cplusplus
}
#endif

#endif /* __MO_WEBKIT_H__ */
