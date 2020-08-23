#include <gtk/gtk.h>
#include <metacpp/meta.h>
#include "mtkwebview.h"
#include "connector.h"
#include "mtkwebkit.h"
#include "mtkgit.h"

static std::string cwd() 
{
    char result[ PATH_MAX ];
    return getcwd(result, PATH_MAX);
}

void onActive(GObject* x, GParamSpec* s, gpointer userdata)
{
     g_print("active\n");
}

class Controller
{
public:
    MtkFiletree* tree;
    MtkWebView* web;

    Gui ui;

    int radio = 0;

    Controller()
    {
        ui
        .register_widgets( mtk_filetree_get_type, mtk_webview_get_type )
        .load("test.ui.xml")
        .bind(this)
        .show();

        //window = (GtkWidget*) ui.get_object("mainWindow");

        web = ui.get<MtkWebView>("web");
        webkit_bind(web,this);
        //pywebkit_webview_load_local_uri( (PywebkitWebview*)web,"index.html");

        tree = ui.get<MtkFiletree>("tree");

        MtkGitFile* file = mtk_gitfile_new( cwd().c_str() );
        mtk_filetree_add_root( tree, (MtkFile*)file, FALSE, ".*");

        ui.add_accelerator("<control>h",&Controller::onHelp,this);

        //g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
//        ui.bind(this);
/*
        GBinding* b1 = g_object_bind_property(
            ui.get_object("ViewStatusMenuItem"), "active",
            ui.get_object("tb_status"), "active",
            G_BINDING_BIDIRECTIONAL)
        ;
*/
       // connect(ui.get_object("ViewStatusMenuItem"), "notify::active", &Controller::onActive, this);
  //      g_signal_connect( G_OBJECT (ui.get_object("ViewStatusMenuItem")), "notify::active", G_CALLBACK(::onActive), this);

//        g_print("binding: %i\n", (void*)b1);
/*
        g_object_bind_property(
            ui.get_object("ViewDiffMenuItem"), "active",
            ui.get_object("tb_diff"), "active",
            G_BINDING_DEFAULT)
        ;
        g_object_bind_property(
            ui.get_object("ViewFileMenuItem"), "active",
            ui.get_object("tb_file"), "active",
            G_BINDING_DEFAULT)
        ;
*/        
    }


    bool sync_radio(GtkWidget* w, int r, const char* menuItem, const char* tbbutton)
    {
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

    void onViewStatus(GtkWidget* w)
    {
        if( !sync_radio(w,0,"ViewStatusMenuItem","tb_status") )
            return;

        g_print("onViewStatus\n");
    }

    void onViewDiff(GtkWidget* w)
    {
        if( !sync_radio(w,1,"ViewDiffMenuItem","tb_diff") )
            return;

        g_print("onViewDiff\n");
    }

    void onViewFile(GtkWidget* w)
    {
        if( !sync_radio(w,2,"ViewFileMenuItem","tb_file") )
            return;

        g_print("onViewDiff\n");
    }

    void onActive(GtkWidget* )
    {
        g_print("Active\n");
    }

    void onExit(GtkWidget* )
    {
        g_print("Exit\n");
        gtk_main_quit();
    }

    gboolean onFileOpen(GtkWidget* button)
    {
        g_print("CONTROLLER CLICK\n");

        std::function<void(ResultFuture<std::string>)> cb = [](auto f)
        {
            std::string s = f.get();
            g_print("CONTROLLER CLICKED %s\n", s.c_str());
        };

        Json::Value json(Json::arrayValue);
        json.append(Json::Value("CONTROLLER CLICK"));
        json.append(Json::Value("CONTROLLER CLICK2"));

        send_request( 
            web, 
           // cb, 
            "recvResponse", 
            json
        )
;//        .then(cb);

        return FALSE;
    }    

    int on_web(std::string msg)
    {
        g_print("web: %s\n", msg.c_str());
        return 42;
    }

    void onHelp(GtkWidget*)
    {
        ui.alert("HELLO WRLD!");
    }

    constexpr static auto meta() 
	{
		return meta::data(
			meta::mem_fun("onFileOpen", &Controller::onFileOpen),
			meta::mem_fun("onViewStatus", &Controller::onViewStatus),
			meta::mem_fun("onViewDiff", &Controller::onViewDiff),
			meta::mem_fun("onViewFile", &Controller::onViewFile),
			meta::mem_fun("on_web", &Controller::on_web),
			meta::mem_fun("onHelp", &Controller::onHelp),
			meta::mem_fun("onExit", &Controller::onExit)
		);
	}
};


int main (int argc, char **argv)
{
    gtk_init (&argc, &argv);

    Controller controller;

    gtk_main();

    return 0;
}
