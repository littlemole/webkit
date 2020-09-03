#ifndef __MO_MTK_H__
#define __MO_MTK_H__

#include <gtk/gtk.h>
#include "mtkfile.h"

#ifdef __cplusplus
extern "C"
{
#endif



/////////////////////////////////////////////////////////////////////////////

typedef struct _MtkFiletree		MtkFiletree;
typedef struct _MtkFiletreeClass	MtkFiletreeClass;

/**
 * MtkFiletree: 
 */

struct _MtkFiletree {
/*< public >*/
    GtkTreeView parent;
    MtkFile* root;
    GtkTreeStore* treeModel;
    MtkFile* place_holder;
    MtkFile* empty_dir;
    gboolean show_hidden;
    gchar* filter;
    gchar* cursel;
};

/**
 * MtkFiletreeClass: 
 */

struct _MtkFiletreeClass {
/*< public >*/
    GtkTreeViewClass parent;
    gchar* dir;
};

GType		mtk_filetree_get_type	() G_GNUC_CONST;

/**
 * mtk_filetree_new: (constructor)
 * Returns: (transfer full): a #MtkFiletree
 */
MtkFiletree*	mtk_filetree_new		();

/**
 * mtk_filetree_add_root: 
 * @self: A #MtkFiletree
 * @file: A #MtkFile
 * @show_hidden: A #gboolean
 * @filter: A #gchar*
 */
void mtk_filetree_add_root(MtkFiletree *self, MtkFile* file, bool show_hidden, const gchar* filter);

/**
 * mtk_filetree_clear: 
 * @self: A #MtkFiletree
 */
void mtk_filetree_clear(MtkFiletree *self);

/**
 * mtk_filetree_get_selected_file: 
 * @self: A #MtkFiletree
 * Returns: (transfer full): a #MtkFile
 */
MtkFile* mtk_filetree_get_selected_file(MtkFiletree *self);

/**
 * mtk_filetree_file_at_pos: 
 * @self: A #MtkFiletree
 * @x: A #gint
 * @y: A #gint
 *
 * Returns: (transfer full): a #MtkFile
 */
MtkFile* mtk_filetree_file_at_pos(MtkFiletree *self, gint x, gint y);


#ifdef __cplusplus
}


#endif


#define MTK_TREE_TYPE		\
    (mtk_filetree_get_type())
#define MTK_TREE(o)			\
    (G_TYPE_CHECK_INSTANCE_CAST ((o), MTK_TREE_TYPE, MtkFiletree))
#define MTK_TREE_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_CAST ((c), MTK_TREE_TYPE, MtkFiletreeClass))
#define MTK_TREE_IS_FILETREE(o)		\
    (G_TYPE_CHECK_INSTANCE_TYPE ((o), MTK_TREE_TYPE))
#define MTK_TREE_IS_FILETREE_CLASS(c)		\
    (G_TYPE_CHECK_CLASS_TYPE ((c),  MTK_TREE_TYPE))
#define MTK_TREE_GET_CLASS(o)	\
    (G_TYPE_INSTANCE_GET_CLASS ((o), MTK_TREE_TYPE, MtkFiletreeClass))



#endif /* __MO_WEBKIT_H__ */
