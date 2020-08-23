#ifndef __MTK_WEBKIT_H__
#define __MTK_WEBKIT_H__

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>


#ifdef __cplusplus
extern "C"
{
#endif


typedef struct _MtkWebView		    MtkWebView;
typedef struct _MtkWebViewClass	MtkWebViewClass;

struct _MtkWebView {
/*< public >*/
    WebKitWebView parent;
    gchar* uid;
    gchar* localpath;
};


struct _MtkWebViewClass {
/*< public >*/
    WebKitWebViewClass parent;
    gchar* dir;
};

GType		mtk_webview_get_type	() G_GNUC_CONST;

/**
 * mtk_webview_new:
 *
 * Allocates a new #MtkWebView.
 *
 * Return value: a new #MtkWebView.
 */

MtkWebView*	mtk_webview_new		();


/**
 * mtk_webview_load_local_uri:
 * @web: #MtkWebView*
 * @localpath: #gchar*
 *
 * loads a new HTML document into #PyWebKit.
 *
 */

void  mtk_webview_load_local_uri(MtkWebView *web, const gchar* localpath);

void  mtk_webview_class_set_dir(MtkWebViewClass* clazz, const gchar* dir);


#ifdef __cplusplus
}
#endif


#define MTK_WEBVIEW_TYPE		\
    (mtk_webview_get_type())

#define MTK_WEBVIEW(o)			\
    (G_TYPE_CHECK_INSTANCE_CAST ((o), MTK_WEBVIEW_TYPE, MtkWebView))

#define MTK_WEBVIEW_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_CAST ((c), MTK_WEBVIEW_TYPE, MtkWebViewClass))

#define MTK_IS_WEBVIEW(o)		\
    (G_TYPE_CHECK_INSTANCE_TYPE ((o), MTK_WEBVIEW_TYPE))

#define MTK_IS_WEBVIEW_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_TYPE ((c),  MTK_WEBVIEW_TYPE))

#define MTK_WEBVIEW_GET_CLASS(o)	\
    (G_TYPE_INSTANCE_GET_CLASS ((o), MTK_WEBVIEW_TYPE, MtkWebViewClass))

#endif /* __MO_WEBKIT_H__ */
