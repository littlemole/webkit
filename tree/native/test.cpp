 #include <gtk/gtk.h>
 #include <metacpp/meta.h>
#include "pywebkit.h"
#include "connector.h"
#include "mtkwebkit.h"

class Controller
{
public:
    GtkWidget* window;
    GtkWidget* label;
    GtkWidget* scrolled;
    GtkWidget* web;
    GtkWidget* titleBar;
    GtkWidget* button;

    Controller()
    {

        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

        button = gtk_button_new_from_icon_name("gtk-action",GTK_ICON_SIZE_SMALL_TOOLBAR);

        titleBar = gtk_header_bar_new();
        gtk_header_bar_pack_start ( (GtkHeaderBar*)titleBar, button);
        gtk_header_bar_set_show_close_button((GtkHeaderBar*)titleBar, TRUE);
        gtk_window_set_titlebar( (GtkWindow*)window,titleBar);

        web = (GtkWidget*)pywebkit_webview_new();
        pywebkit_webview_load_local_uri( (PywebkitWebview*)web,"index.html");

//        label = gtk_label_new ("Hello GNOME Controller!");
        scrolled = gtk_scrolled_window_new(NULL,NULL);
        gtk_scrolled_window_add_with_viewport ((GtkScrolledWindow*)scrolled,web);

        gtk_container_add (GTK_CONTAINER (window), scrolled);
        gtk_window_set_title (GTK_WINDOW (window), "Welcome to GNOME");

        gtk_window_set_default_size (GTK_WINDOW (window), 600, 400);
        gtk_widget_show_all (window);

        g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

        connect( button, "clicked", &Controller::on_click, this);
        g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

        webkit_bind( (WebKitWebView*)web,this);
    }

    gboolean on_click(GtkWidget* button)
    {
        g_print("CONTROLLER CLICK\n");

        //send_request( (WebKitWebView*)web, std::string("recvResponse"), "CONTROLLER CLICK");

        std::function<void(std::string)> cb = [](std::string s)
        {
                g_print("CONTROLLER CLICKED %s\n", s.c_str());
        };

        Json::Value json(Json::arrayValue);
        json.append(Json::Value("CONTROLLER CLICK"));
        json.append(Json::Value("CONTROLLER CLICK2"));

        send_request( 
            (WebKitWebView*)web, 
            cb, 
            std::string("recvResponse"), 
            json
        );
        return FALSE;
    }    

    int on_web(std::string msg)
    {
        g_print("web: %s\n", msg.c_str());
        return 42;
    }

    constexpr static auto meta() 
	{
		return meta::data(
			meta::mem_fun("on_click", &Controller::on_click),
			meta::mem_fun("on_web", &Controller::on_web)
		);
	}
};


int main (int argc, char **argv)
{
    gtk_init (&argc, &argv);

    int status = 0;

    Controller controller;


/*
    GtkBuilder *builder = gtk_builder_new();
    GError* error = 0;

    if ( gtk_builder_add_from_file(builder,"test.ui.xml", &error) == 0)
    {
        g_printerr ("Error loading file: %s\n", error->message);
        g_clear_error (&error);
        return 1;
    }

    auto window = gtk_builder_get_object (builder, "mainWindow");
    g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

    auto label = gtk_builder_get_object (builder, "label");
    gtk_widget_set_has_window((GtkWidget*)label,TRUE);
    gtk_widget_set_events((GtkWidget*)label,GDK_BUTTON_PRESS_MASK);
    //  int id = g_signal_connect (label, "button-press-event", G_CALLBACK (on_click), NULL);
    //  g_print("on_click: %i\n",id);

    gtk_window_set_default_size((GtkWindow*)window,300,300);

    gtk_builder_connect_signals_full(builder, connector, &controller);
*/
   // gtk_widget_show_all((GtkWidget*)window);

    gtk_main();

    return status;
}
