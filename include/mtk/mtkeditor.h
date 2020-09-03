#ifndef __MO_MTK_EDITOR_H__
#define __MO_MTK_EDITOR_H__

#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

#ifdef __cplusplus
extern "C"
{
#endif


/////////////////////////////////////////////////////////////////////////////

typedef struct _MtkEditor		MtkEditor;
typedef struct _MtkEditorClass	MtkEditorClass;

/**
 * MtkEditor: 
 */

struct _MtkEditor {
/*< public >*/
    GtkSourceView parent;
    GtkSourceFile* source;
    GtkSourceLanguageManager* lang_manager;
    gchar* path;
    gchar* lang;
};

/**
 * MtkEditorClass: 
 */

struct _MtkEditorClass {
/*< public >*/
    GtkSourceViewClass parent;
};

GType		mtk_editor_get_type	() G_GNUC_CONST;

/**
 * mtk_editor_new: (constructor)
 * Returns: (transfer full): a #MtkEditor
 */
MtkEditor*	mtk_editor_new		();

/**
 * mtk_editor_is_modified: 
 * @self: A #MtkEditor
 *
 * Returns: a #gboolean
 */
gboolean mtk_editor_is_modified(MtkEditor *self);

/**
 * mtk_editor_load: 
 * @self: A #MtkEditor
 * @file: a #gchar*
 */
void mtk_editor_load(MtkEditor *self, const gchar* file);

/**
 * mtk_editor_save: 
 * @self: A #MtkEditor
 */
void mtk_editor_save(MtkEditor *self);

/**
 * mtk_editor_save_as: 
 * @self: A #MtkEditor
 * @file: a #gchar*
 */
void mtk_editor_save_as(MtkEditor *self, const gchar* file);

/**
 * mtk_editor_get_text: 
 * @self: A #MtkEditor
 *
 * Returns: a #gchar*
 */
gchar* mtk_editor_get_text(MtkEditor *self);

/**
 * mtk_editor_set_text: 
 * @self: A #MtkEditor
 * @txt: a #gchar*
 *
 */
void mtk_editor_set_text(MtkEditor *self, const gchar* txt);



#ifdef __cplusplus
}


#endif


#define MTK_EDITOR_TYPE		\
    (mtk_editor_get_type())
#define MTK_EDITOR(o)			\
    (G_TYPE_CHECK_INSTANCE_CAST ((o), MTK_EDITOR_TYPE, MtkEditor))
#define MTK_EDITOR_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_CAST ((c), MTK_EDITOR_TYPE, MtkEditorClass))
#define MTK_EDITOR_IS_EDITOR(o)		\
    (G_TYPE_CHECK_INSTANCE_TYPE ((o), MTK_EDITOR_TYPE))
#define MTK_EDITOR_IS_EDITOR_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_TYPE ((c),  MTK_EDITOR_TYPE))
#define MTK_EDITOR_GET_CLASS(o)	\
    (G_TYPE_INSTANCE_GET_CLASS ((o), MTK_EDITOR_TYPE, MtkEditorClass))



#endif 
