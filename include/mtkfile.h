#ifndef __MO_MTK_FILE_H__
#define __MO_MTK_FILE_H__

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _MtkFile		MtkFile;
typedef struct _MtkFileClass	MtkFileClass;

/**
 * MtkFile: 
 */
struct _MtkFile {
/*< public >*/
    GObject parent;
    GtkTreeIter* root;
    gchar* file_name;
    gboolean is_place_holder;
    gboolean is_directory;
    gboolean is_empty;
    gboolean is_hidden;
};

/**
 * MtkFileClass: 
 */

struct _MtkFileClass {
/*< public >*/
    GObjectClass parent;

    gchar* (*get_tooltip)(MtkFile* self);
    GList* (*get_children)(MtkFile* self, GtkTreeIter* iter);

    void (*tree_cell_render_file)(
            MtkFile* self,
            GtkTreeViewColumn *tree_column,
            GtkCellRenderer *cell,
            GtkTreeModel *tree_model,
            GtkTreeIter *iter,
            gpointer data    
    );

    void (*tree_cell_render_pix)(
            MtkFile* self,
            GtkTreeViewColumn *tree_column,
            GtkCellRenderer *cell,
            GtkTreeModel *tree_model,
            GtkTreeIter *iter,
            gpointer data    
    );

    gpointer padding[12];
};

GType		mtk_file_get_type	() G_GNUC_CONST;


/**
 * mtk_file_new: 
 * @fn : A #gchar*
 * Returns: (transfer full): a #MtkFile
 */
MtkFile*	mtk_file_new( const gchar* fn);

/**
 * mtk_file_set_path: 
 * @self: A #MtkFile
 * @fn : A #gchar*
 */
void mtk_file_set_path(MtkFile* self,  gchar* fn);

/**
 * mtk_file_get_path: 
 * @self: A #MtkFile
 *
 * Returns: (transfer none): a #gchar*
 */
gchar* mtk_file_get_path(MtkFile* self);

/**
 * mtk_file_get_parent: 
 * @self: A #MtkFile
 *
 * Returns: (transfer full): a #gchar*
 */
gchar* mtk_file_get_parent(MtkFile* self);

/**
 * mtk_file_get_basename: 
 * @self: A #MtkFile
 *
 * Returns: (transfer full): a #gchar*
 */
gchar* mtk_file_get_basename(MtkFile* self);

/**
 * mtk_file_get_tooltip: 
 * @self: A #MtkFile
 *
 * Returns: (transfer none): a #gchar*
 */
gchar* mtk_file_get_tooltip(MtkFile* self);

/**
 * mtk_file_get_children: 
 * @self: A #MtkFile
 * @iter: A #GtkTreeIter
 *
 * Returns: (element-type MtkFile) (transfer full): List of #MtkFile
 */
GList* mtk_file_get_children(MtkFile* self, GtkTreeIter* iter);

/**
 * mtk_file_tree_cell_render_file: 
 * @self: A #GtkTreeViewColumn
 * @tree_column: A #GtkTreeIter
 * @cell: A #GtkCellRenderer
 * @tree_model: A #GtkTreeModel
 * @iter: A #GtkTreeIter
 * @data: A #gpointer
 */
void mtk_file_tree_cell_render_file(
        MtkFile* self,
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data    
);


/**
 * mtk_file_tree_cell_render_pix: 
 * @self: A #GtkTreeViewColumn
 * @tree_column: A #GtkTreeIter
 * @cell: A #GtkCellRenderer
 * @tree_model: A #GtkTreeModel
 * @iter: A #GtkTreeIter
 * @data: A #gpointer
 */
void mtk_file_tree_cell_render_pix(
        MtkFile* self,
        GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data    
);

//////////////////////////////////////////////////////////////////////////////

/**
 * mtk_bash: 
 * @cmd: A #gchar*
 * @status: (out caller-allocates) (optional) : A #gint
 * Returns: (transfer full): a #gchar*
 */
gchar* mtk_bash(const gchar* cmd, gint* status);

/**
 * MtkAsyncBashCallbackFunc: 
 * @status: A #gint
 * @out: A #gchar*
 * @user_data: A #gpointer
 *
 */

typedef void (*MtkAsyncBashCallbackFunc)(int status, const gchar* out, gpointer user_data);

#define MTK_ASYNC_BASH_CALLBACK(f) ((MtkAsyncBashCallbackFunc) (void (*)(int,gchar*,gpointer)) (f))

/**
 * mtk_bash_async: 
 * @cmd: A #gchar*
 * @callback: (closure user_data) (scope async): A #MtkAsyncBashCallbackFunc
 * @user_data: (closure): a #gpointer
 *
 */
void mtk_bash_async(const gchar* cmd, MtkAsyncBashCallbackFunc callback, gpointer user_data);

#ifdef __cplusplus
}
#endif

#define MTK_FILE_TYPE		\
    (mtk_file_get_type())
#define MTK_FILE(o)			\
    (G_TYPE_CHECK_INSTANCE_CAST ((o), MTK_FILE_TYPE, MtkFile))
#define MTK_FILE_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_CAST ((c), MTK_FILE_TYPE, MtkFileClass))
#define MTK_IS_FILETREE_FILE(o)		\
    (G_TYPE_CHECK_INSTANCE_TYPE ((o), MTK_FILE_TYPE))
#define MTK_IS_FILETREE_FILE_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_TYPE ((c),  MTK_FILE_TYPE))
#define MTK_FILE_GET_CLASS(o)	\
    (G_TYPE_INSTANCE_GET_CLASS ((o), MTK_FILE_TYPE, MtkFileClass))


#endif /* __MO_WEBKIT_H__ */
