
#include "mtk/mtkgit.h"
#include "mtkcpp/ui.h"

int main (int argc, char **argv);

extern std::map<std::string,std::string> resources;

// main UI controller

class Controller
{
public:

    static void webKitURISchemeReqquestCB(WebKitURISchemeRequest *request, gpointer user_data)
    {
        Controller* controller = (Controller*)user_data;
        controller->onRequest(request);
    }

    Controller()
    {
        webkit_web_context_register_uri_scheme(
            webkit_web_context_get_default(),
            "local",
            &Controller::webKitURISchemeReqquestCB,
            this,
            NULL
        );

        ui
        .register_widgets( mtk_filetree_get_type, mtk_webview_get_type )
        .load_string( resources["test.ui.xml"].c_str() )
        .bind(this)
        .show();

        web = ui.get<MtkWebView>("web");
        webkit_bind(web,this);

        tree = ui.get<MtkFiletree>("tree");

        MtkGitFile* file = mtk_gitfile_new( cwd().c_str() );
        mtk_filetree_add_root( tree, (MtkFile*)file, FALSE, ".*");
        g_object_unref(file);

        ui.add_accelerator("<control>h",&Controller::onHelp,this);

        ui.status_bar( cwd().c_str() );      
    }

    void onRequest(WebKitURISchemeRequest *request)
    {
        std::string uri = webkit_uri_scheme_request_get_uri(request);
        std::string path = webkit_uri_scheme_request_get_path(request);

        std::cout << "URI: " << uri << std::endl << "PATH: " << path << std::endl;

        if ( path[0] == '/' )
        {
            path = path.substr(1);
        }

        if ( resources.count(path) != 0)
        {
            std::string& data = resources[path];

            GInputStream* stream = g_memory_input_stream_new_from_data( data.c_str(), data.size(), NULL );
        
            webkit_uri_scheme_request_finish(request,stream,data.size(),"text/html");

            return;
        }

        std::string data = "<html><body><h1>HELLO LOCAL URL</h1></body></html>";

        gchar* d = g_strdup(data.c_str());

        GInputStream* stream = g_memory_input_stream_new_from_data( d, data.size(), g_free );
        
        webkit_uri_scheme_request_finish(request,stream,data.size(),"text/html");
    }

    // JavaScript handlers
    void onDocumentLoad()
    {
        onViewStatus(0);
    }

    void onSubmitCommit(std::string msg)
    {
        MtkFile* file = mtk_filetree_get_selected_file(tree);

        gchar* status = 0;
        gchar* content = 0;
        mtk_git_commit(file,msg.c_str(),&status,&content);

        send_request( web, "setPlainText", status, content );      

        g_object_unref(file);
        g_free(status);
        g_free(content);
    }    


    void onCreateBranch(std::string branch)
    {
        MtkFile* file = mtk_filetree_get_selected_file(tree);
        mtk_git_create_branch(file,branch.c_str());

        g_object_unref(file);       

        onGitShowBranches(0);
    }


    void onSelectBranch(std::string branch)
    {
        MtkFile* file = mtk_filetree_get_selected_file(tree);
        mtk_git_switch_branch(file,branch.c_str());

        g_object_unref(file);       

        onGitShowBranches(0);
    }


    void onDeleteBranch(std::string branch)
    {
        MtkFile* file = mtk_filetree_get_selected_file(tree);

        gchar* status = 0;
        gchar* content = 0;
        mtk_git_cmd(file,MTK_GIT_DEFAULT_BRANCH,&status,&content);

        if( strncmp(branch.c_str(),content,branch.size()) == 0 )
        {
            return;
        }

        mtk_git_delete_branch(file,branch.c_str());

        g_object_unref(file);       
        g_free(status);
        g_free(content);

        onGitShowBranches(0);
    }

    // Menu Handlers
    void onFileOpen(GtkWidget*)
    {
        std::string d = ui.showFileDialog( GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,"Please choose a folder");
        if(d.empty())
        {
            return;
        }

        ui.status_bar( d.c_str() );
    
        mtk_filetree_clear(tree);

        MtkGitFile* f = mtk_gitfile_new(d.c_str());
        mtk_filetree_add_root(tree, (MtkFile*)f, TRUE, ".*");
        
        g_object_unref(f);

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
        MtkFile* file = mtk_filetree_get_selected_file(tree);

        gchar* status = 0;
        gchar* content = 0;
        mtk_git_cmd(file,MTK_GIT_BRANCHES,&status,&content);

        send_request( web, "setBranches", status, content );      

        g_object_unref(file);
        g_free(status);
        g_free(content);
    }

    void onViewStatus(GtkWidget* w)
    {
        if( !sync_radio(w,0,"ViewStatusMenuItem","tb_status") )
            return;

        do_git(MTK_GIT_STATUS);
    }

    void onViewDiff(GtkWidget* w)
    {
        if( !sync_radio(w,1,"ViewDiffMenuItem","tb_diff") )
            return;

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

    void onViewFile(GtkWidget* w)
    {
        if( !sync_radio(w,2,"ViewFileMenuItem","tb_file") )
            return;

        do_git(MTK_GIT_VIEW_FILE);
    }

    void onViewRefresh(GtkWidget*)
    {
        MtkFile* file = tree->root;
        g_object_ref(file);

        mtk_filetree_clear(tree);
        mtk_filetree_add_root( tree, file, FALSE, ".*");

        g_object_unref(file);
    }

    void onHelp(GtkWidget*)
    {
        ui.alert("HELLO WRLD!");
        webkit_web_view_load_uri(ui.get<WebKitWebView>("web"),"local:///index.html");

    }

    // context Menues

    gboolean onContext(GtkWidget *widget, GdkEvent* event)
    {
        if( event->button.button == 3) // right click
        {
            GtkMenu* m = ui.get<GtkMenu>("GitSubMenu");
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
        GtkMenu* m = ui.get<GtkMenu>("ViewSubMenu");
        gtk_menu_popup_at_pointer( m, event);

        // prevent default menu
        return TRUE;
    }  

    // selection in file tree widget changed
    void onSelect( GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn* column)
    {
        switch(radio)
        {
            case 0 :
            {
                onViewStatus(0);
                break;
            }
            case 1 :
            {
                onViewDiff(0);                
                break;
            }
            case 2 :
            {
                onViewFile(0);                                
                break;
            }
        }
    }

private:

    // the glade UI
    Gui ui;

    // main widgets
    MtkFiletree* tree = nullptr;
    MtkWebView* web = nullptr;

    // current view mode radio toggle
    int radio = 0;

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

    // sync a menu radio-group with a toolbar radio-group
    bool sync_radio(GtkWidget* w, int r, const char* menuItem, const char* tbbutton)
    {
        if(!w)
            return true;

        gval active;
        g_object_get_property( (GObject*) w, "active", &active);
        bool b = active.get_bool();

        if(!b) return false;

        if(radio==r) return false;
        radio = r;

        if(  (GObject*) w == ui.get_object(menuItem))
        {
            GtkToggleToolButton* butt = ui.get<GtkToggleToolButton>(tbbutton);
            gtk_toggle_tool_button_set_active(butt,TRUE);
        }
        else
        {
            GtkCheckMenuItem* menu = ui.get<GtkCheckMenuItem>(menuItem);
            gtk_check_menu_item_set_active(menu,TRUE);
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
			meta::mem_fun("onFileOpen",         &Controller::onFileOpen),
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
			meta::mem_fun("onExit",             &Controller::onExit)
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
