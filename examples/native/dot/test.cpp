
#include "mtk/mtkgit.h"
#include "mtk/mtkeditor.h"
#include "mtkcpp/ui.h"
#include <fstream>
#include <cryptoneat/base64.h>

int main (int argc, char **argv);

// main UI controller

class Controller
{
public:

    Controller()
    {
        ui
        .register_widgets( mtk_filetree_get_type, mtk_webview_get_type, mtk_editor_get_type )
        .load( "text.ui.xml" )
        .bind(this)
        .show();

        main = ui.get<GtkWindow>("mainWindow");
        gtk_window_set_title(main, cwd().c_str() );

        web = ui.get<MtkWebView>("web");
        webkit_bind(web,this);

        tree = ui.get<MtkFiletree>("tree");

        gobj<MtkFile> file = mtk_gitfile_new( cwd().c_str() );
        mtk_filetree_add_root( tree, *file, TRUE, ".*\\.dot");

        editor = ui.get<MtkEditor>("editor");

        notebook = ui.get<GtkNotebook>("sidePane");

        ui.add_accelerator("<control>h",&Controller::onHelp,this);

        ui.status_bar( cwd().c_str() );      
    }

    // JavaScript handlers
    void onDocumentLoad()
    {
        onViewStatus(0);
    }

    void onSubmitCommit(std::string msg)
    {
        gobj<MtkFile> file = mtk_filetree_get_selected_file(tree);

        gstring status;
        gstring content;
        mtk_git_commit( *file,msg.c_str(),&status,&content);

        send_request( web, "setPlainText", *status, *content );      
    }    


    void onCreateBranch(std::string branch)
    {
        gobj<MtkFile> file = mtk_filetree_get_selected_file(tree);
        mtk_git_create_branch( *file,branch.c_str());

        onGitShowBranches(0);
    }


    void onSelectBranch(std::string branch)
    {
        gobj<MtkFile> file = mtk_filetree_get_selected_file(tree);
        mtk_git_switch_branch( *file,branch.c_str());

        onGitShowBranches(0);
    }


    void onDeleteBranch(std::string branch)
    {
        gobj<MtkFile> file = mtk_filetree_get_selected_file(tree);

        gstring status;
        gstring content;
        mtk_git_cmd( *file,MTK_GIT_DEFAULT_BRANCH,&status,&content);

        if( strncmp(branch.c_str(),*content,branch.size()) == 0 )
        {
            return;
        }

        mtk_git_delete_branch( *file,branch.c_str());

        onGitShowBranches(0);
    }

    void onSaveImage(std::string data)
    {
        static std::string png_header = "data:image/png;base64,";
        size_t pos = data.find(png_header);
        if(pos != 0)
        {
            return;
        }
        data = data.substr(png_header.size());

        std::string bytes = cryptoneat::Base64::decode(data);

        std::string f = ui.showFileDialog(
            GTK_FILE_CHOOSER_ACTION_SAVE,
            "Please choose target to save this .png image file", 
            "pngFilter"
        );

        if(f.empty())
        {
            return;
        }

        std::ofstream of;
        of.open(f,std::ios::binary);
        if(of)
        {
            of.write(bytes.c_str(),bytes.size());
            of.close();
        }
    }

    void onClickGraph(std::string uri)
    {
        if( !scrape_if_modified() )
        {
            onViewFile(0);
            return;
        }

        gobj<MtkFile> file = mtk_filetree_get_selected_file(tree);

        gstring parent = mtk_file_get_parent( *file );

        std::ostringstream oss;
        oss << *parent << "/" << uri;

        std::cout << "load: " << oss.str() << std::endl;

        mtk_editor_load(editor, oss.str().c_str() );
    }


    // Menu Handlers
    void onFileOpenDir(GtkWidget*)
    {
        if( !scrape_if_modified() )
        {
            onViewFile(0);
            return;
        }

        std::string d = ui.showFileDialog( GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,"Please choose a folder");
        if(d.empty())
        {
            return;
        }

        ui.status_bar( d.c_str() );
        gtk_window_set_title(main, d.c_str() );
    
        mtk_filetree_clear(tree);

        gobj<MtkFile> f = mtk_gitfile_new(d.c_str());
        mtk_filetree_add_root(tree, *f, TRUE, ".*");
        
        onViewFiles(0);
        onViewStatus(0);
    }

    void onExit(GtkWidget* )
    {
        gtk_main_quit();
    }

    void onGitAdd(GtkWidget*)
    {
        do_git(MTK_GIT_ADD,true);
    }

    void onGitCommit(GtkWidget*)
    {
        do_git(MTK_GIT_DIFF_CACHED,false,"setCommit");
    }

    void onGitRestore(GtkWidget*)
    {
        do_git(MTK_GIT_RESTORE,true);        
    }

    void onGitRestoreStaged(GtkWidget*)
    {
        do_git(MTK_GIT_RESTORE_STAGED,true);                
    }

    void onGitRestoreOrigin(GtkWidget*)
    {
        do_git(MTK_GIT_RESTORE_ORIGIN,true);                
    }

    void onGitPull(GtkWidget*)
    {
        do_git(MTK_GIT_PULL,true);                
    }

    void onGitPush(GtkWidget*)
    {
        do_git(MTK_GIT_PUSH,true);                
    }

    void onGitShowBranches(GtkWidget*)
    {
        gobj<MtkFile> file = mtk_filetree_get_selected_file(tree);

        gstring status;
        gstring content;

        mtk_git_cmd( *file,MTK_GIT_BRANCHES,&status,&content);

        send_request( web, "setBranches", *status, *content );      
    }

    void onViewStatus(GtkWidget* w)
    {
        do_git(MTK_GIT_STATUS);
    }

    void onViewDiff(GtkWidget* w)
    {
        do_git(MTK_GIT_DIFF,false,"setDiff");    
    }

    void onGitDiffCached(GtkWidget* w)
    {
        do_git(MTK_GIT_DIFF_CACHED,false,"setDiff");
    }    

    void onGitDiffOrigin(GtkWidget* w)
    {
        do_git(MTK_GIT_DIFF_ORIGIN,false,"setDiff");
    }    

    void onViewRefresh(GtkWidget*)
    {
        gobj<MtkFile> file = tree->root;
        file.ref();

        mtk_filetree_clear(tree);
        mtk_filetree_add_root( tree, *file, FALSE, ".*");

    }

    void onHelp(GtkWidget*)
    {
        ui.alert("This is a native .dot file viewer and editor build on top of gtkwebkit2.");

    }

    // context Menues

    gboolean onContext(GtkWidget *widget, GdkEvent* event)
    {
        if( event->button.button == 3) // right click
        {
            GdkEventButton* b = (GdkEventButton*)event;

            gobj<MtkFile> f =  mtk_filetree_file_at_pos(tree, b->x, b->y);

            GtkMenu* m = nullptr;

            if( f->is_directory)
            {
                m = ui.get<GtkMenu>("directoryContextMenu");
            }
            else
            {
                m = ui.get<GtkMenu>("fileContextMenu");
            }

            gtk_menu_popup_at_pointer( m, event);
        }

        // allow further processing
        return FALSE;
    }    
 

    gboolean onWebContext(
        WebKitWebView* web_view,
        WebKitContextMenu* context_menu,
        GdkEvent* event,
        WebKitHitTestResult* hit_test_result )
    {
        GtkMenu* m = ui.get<GtkMenu>("webContextMenu");
        gtk_menu_popup_at_pointer( m, event);

        // prevent default menu
        return TRUE;
    }  

    // selection in file tree widget changed
    void onSelect( GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn* column)
    {
        if( !scrape_if_modified())
        {
            onViewFile(nullptr);
            return;
        }

        gobj<MtkFile> f = mtk_filetree_get_selected_file(tree);
        ui.status_bar(f->file_name);

        if(f->is_directory)
        {
            onViewStatus(nullptr);
        }
        else
        {
            mtk_editor_load(editor,f->file_name);
        }
    }

    void onSourceChanged(void*)
    {
        gstring txt = mtk_editor_get_text(editor);

        if(*txt)
        {
            send_request(web, "setDot", std::string(editor->path), std::string(*txt) );
        }
    }

    void onSwitchPage(GtkNotebook* notebook, GtkWidget* page, guint num)
    {
        if (num == 0)
        {
            gtk_check_menu_item_set_active( ui.get<GtkCheckMenuItem>("ViewFilesMenuItem"), TRUE);
        }
        else
        {
            gtk_check_menu_item_set_active( ui.get<GtkCheckMenuItem>("ViewEditorMenuItem"), TRUE);
        }
    }

    void onWindowState(GtkWidget* w, GdkEvent* e)
    {
        if(e)
        {
            GdkEventWindowState* state = (GdkEventWindowState*)e;

            if( state->new_window_state & GDK_WINDOW_STATE_FULLSCREEN )
            {
                fullscreen = true;
            }
            else
            {
                fullscreen = false;
            }
        }
    }

    void onViewFiles(void*)
    {
        gtk_notebook_set_current_page(notebook,0);
    }

    void onViewFile(void*)
    {
        gtk_notebook_set_current_page(notebook,1);
    }

    void onViewResetZoom(void*)
    {
        send_request(web, "resetZoom");
    }

    void onViewFullscreen(void*)
    {
        if(fullscreen)
        {
            gtk_window_unfullscreen(main);
        }
        else
        {
            gtk_window_fullscreen(main);
        }
    }

    void onExportGraph(void*)
    {
        send_request(web,"onGetGraphImage");
    }

    void onNewDotfile(void*)
    {
        if( !scrape_if_modified())
        {
            onViewFile(nullptr);
            return;
        }

        gobj<MtkFile> f = mtk_filetree_get_selected_file(tree);
        gstring p = g_strdup(f->file_name);
        if(!f->is_directory)
        {
            p = mtk_file_get_parent( *f );
        }        

        std::string new_file = ui.showFileDialog( GTK_FILE_CHOOSER_ACTION_SAVE,"path to new .dot file", "dotFilter", p.str());

        if(new_file.empty())
        {
            return;
        }

        std::ofstream touch(new_file);
        mtk_editor_load(editor,new_file.c_str());
        onViewRefresh(nullptr);
    }


    void onFileOpen(void*)
    {
        if( !scrape_if_modified())
        {
            onViewFile(nullptr);
            return;
        }

        std::string path = ui.showFileDialog( GTK_FILE_CHOOSER_ACTION_OPEN,"Please choose a .dot file", "dotFilter");
        if(path.empty())
        {
            return;
        }

        ui.status_bar(path.c_str());
        mtk_editor_load(editor,path.c_str());

    }


    void onFileSave(void*)
    {
        mtk_editor_save(editor);

        gobj<MtkFile> f = mtk_file_new(editor->path);
        gstring d = mtk_file_get_basename( *f );

        std::ostringstream oss;
        oss << "file " << *d << " saved.";

        ui.status_bar( oss.str().c_str() );
    }


    void onFileSaveAs(void*)
    {
        std::string file = ui.showFileDialog( GTK_FILE_CHOOSER_ACTION_SAVE,"Please choose target to save this .dot file", "dotFilter");
        if(file.empty())
        {
            return;
        }

        mtk_editor_save_as(editor,file.c_str());

        gobj<MtkFile> f = mtk_file_new(file.c_str());
        gstring d = mtk_file_get_basename( *f );

        std::ostringstream oss;
        oss << "file " << *d << " saved.";

        ui.status_bar( oss.str().c_str() );
        onViewRefresh(nullptr);
    }

private:

    // the glade UI
    Gui ui;

    // main widgets
    MtkFiletree* tree = nullptr;
    MtkWebView* web = nullptr;
    MtkEditor* editor = nullptr;
    GtkWindow* main = nullptr;
    GtkNotebook* notebook = nullptr;

    // fullscreen
    bool fullscreen = false;


    // execute a git command
    void do_git(MtkGitCmd cmd, bool refresh = false, const gchar* mode = "setPlainText", MtkFile* file  = NULL )
    {
        if(!file)
        {
            file = mtk_filetree_get_selected_file(tree);
        }

        gchar* status = 0;
        gchar* content = 0;
        mtk_git_cmd(file,cmd,&status,&content);

        send_request( web, mode, status, content );      

        if(refresh)
        {
            onViewRefresh(0);  
        }

        g_object_unref(file);
        g_free(status);
        g_free(content);
    }


    bool scrape_if_modified()
    {
        bool is_modified = mtk_editor_is_modified(editor);

        if (is_modified)
        {
            int r = ui.alert(
               "scrape unsaved changes in editor?", 
               GTK_BUTTONS_OK_CANCEL, 
               GTK_MESSAGE_WARNING, 
               GTK_BUTTONS_CANCEL
            );

            if (r == GTK_RESPONSE_CANCEL)
            {
                return false;
            }
        }
        return true;
    }


    // the current directory
    std::string cwd() 
    {
        char result[ PATH_MAX ];
        return getcwd(result, PATH_MAX);
    }
};

// metadata used for mapping both glade and JavaScript event handlers

template<>
struct meta::Data<Controller>
{
    static constexpr auto meta()
    {
		return meta::data(
			meta::mem_fun("onFileOpenDir",      &Controller::onFileOpenDir),
			meta::mem_fun("onGitAdd",           &Controller::onGitAdd),
			meta::mem_fun("onGitCommit",        &Controller::onGitCommit),
			meta::mem_fun("onSubmitCommit",     &Controller::onSubmitCommit),
			meta::mem_fun("onGitRestore",       &Controller::onGitAdd),
			meta::mem_fun("onGitRestoreStaged", &Controller::onGitRestoreStaged),
			meta::mem_fun("onGitRestoreOrigin", &Controller::onGitRestoreOrigin),
			meta::mem_fun("onGitPull",          &Controller::onGitPull),
			meta::mem_fun("onGitPush",          &Controller::onGitPush),
			meta::mem_fun("onGitShowBranches",  &Controller::onGitShowBranches),
			meta::mem_fun("onSelectBranch",     &Controller::onSelectBranch),
			meta::mem_fun("onCreateBranch",     &Controller::onCreateBranch),
			meta::mem_fun("onDeleteBranch",     &Controller::onDeleteBranch),
			meta::mem_fun("onViewStatus",       &Controller::onViewStatus),
			meta::mem_fun("onViewDiff",         &Controller::onViewDiff),
			meta::mem_fun("onGitDiffCached",    &Controller::onGitDiffCached),
			meta::mem_fun("onGitDiffOrigin",    &Controller::onGitDiffOrigin),
			meta::mem_fun("onViewFile",         &Controller::onViewFile),
			meta::mem_fun("onViewRefresh",      &Controller::onViewRefresh),
			meta::mem_fun("onContext",          &Controller::onContext),
			meta::mem_fun("onWebContext",       &Controller::onWebContext),
			meta::mem_fun("onSelect",           &Controller::onSelect),
			meta::mem_fun("onDocumentLoad",     &Controller::onDocumentLoad),
			meta::mem_fun("onHelp",             &Controller::onHelp),
			meta::mem_fun("onExit",             &Controller::onExit),
			meta::mem_fun("onExportGraph",      &Controller::onExportGraph ),
			meta::mem_fun("onViewFullscreen",   &Controller::onViewFullscreen ),
			meta::mem_fun("onViewResetZoom",    &Controller::onViewResetZoom ),
			meta::mem_fun("onViewFiles",        &Controller::onViewFiles ),
			meta::mem_fun("onWindowState",      &Controller::onWindowState ),
			meta::mem_fun("onSwitchPage",       &Controller::onSwitchPage ),
			meta::mem_fun("onSourceChanged",    &Controller::onSourceChanged ),
			meta::mem_fun("onSaveImage",        &Controller::onSaveImage ),
			meta::mem_fun("onClickGraph",       &Controller::onClickGraph ),

			meta::mem_fun("onNewDotfile",       &Controller::onNewDotfile ),
			meta::mem_fun("onFileOpen",         &Controller::onFileOpen ),
			meta::mem_fun("onFileSave",         &Controller::onFileSave ),
			meta::mem_fun("onFileSaveAs",       &Controller::onFileSaveAs )

		);        
    }
};


// main application entry point

int main (int argc, char **argv)
{
    gtk_init (&argc, &argv);

    Controller controller;

    gtk_main();

    return 0;
}
