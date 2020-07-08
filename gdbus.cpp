#include <gio/gio.h>
#include <stdlib.h>
#include <iostream>

static GDBusConnection* dbuscon = 0;
static GMainLoop *loop = 0;
 
static void done()
{
    g_main_loop_quit(loop);
}

static void send_signal( std::string s, int i, GDBusConnection *connection)
{
  GVariantBuilder *builder;
  GVariantBuilder *builder2;
  GVariantBuilder *builder3;
  GVariantBuilder *invalidated_builder;

//  builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);

  builder = g_variant_builder_new (G_VARIANT_TYPE_TUPLE);
  builder2 = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
  builder3 = g_variant_builder_new (G_VARIANT_TYPE_TUPLE);

  //invalidated_builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));


    std::cout << "builder 1" << std::endl;
 /* g_variant_builder_add (builder,
                           "s",
                           s.c_str());

*/
    GVariant* dict = g_variant_new("{sv}", "keyy",g_variant_new("s","theKey"));

    g_variant_builder_add_value(builder2,dict);

    dict = g_variant_new("{sv}", "key2",g_variant_new("s","theKey2"));

    g_variant_builder_add_value(builder2,dict);

    dict = g_variant_new("{sv}", "key3",g_variant_new("x",4711));

    g_variant_builder_add_value(builder2,dict);

    //g_variant_new_dict_entry(g_variant_new("s","key"), g_variant_new("s","theKey"));

    g_variant_builder_add(builder3,"s","one");
    g_variant_builder_add(builder3,"x",4711);

    dict = g_variant_new("{sv}", "key4",g_variant_builder_end(builder3));

    g_variant_builder_add_value(builder2,dict);

    g_variant_builder_add_value(builder,g_variant_builder_end (builder2));

    std::cout << "builder 2" << std::endl;
  g_variant_builder_add (builder,
                           "x",
                           i );

    std::cout << "emit" << std::endl;

    GVariant* params = g_variant_builder_end (builder);

        GString* gs = g_variant_print_string (params, NULL,TRUE);
        std::cout << g_variant_get_type_string (params) << " " << gs->str << std::endl;
        g_string_free (gs,TRUE);

  g_dbus_connection_emit_signal (connection,
                                 NULL,
                                 "/com/example/TestService/object",
                                 "com.example.TestService",
                                 "HelloSignal",
                                 params,
                                 NULL);
                                 

}

void signal_handler(GDBusConnection *connection,
                        const gchar *sender_name,
                        const gchar *object_path,
                        const gchar *interface_name,
                        const gchar *signal_name,
                        GVariant *parameters,
                        gpointer user_data)
{
    std::cout << "got signal " << signal_name << " from " << sender_name << " for " 
        << interface_name << " at " << object_path << " with values: " << g_variant_get_type_string (parameters) << std::endl;


    gsize s = g_variant_n_children (parameters);
    std::cout << "size " << s << std::endl;
    for( gsize i = 0; i < s; i++) 
    {
        GVariant* gv = g_variant_get_child_value (parameters,i);
//        g_variant_print (gv,TRUE);

        GString* gs = g_variant_print_string (gv, NULL,TRUE);
        std::cout << g_variant_get_type_string (gv) << " " << gs->str << std::endl;
        g_string_free (gs,TRUE);
        g_variant_unref(gv);
    }

    std::cout << std::endl;

}

void got_dbus (GObject *source_object,
                        GAsyncResult *res,
                        gpointer user_data)
{
    std::cout << "get_dbus!" << std::endl;
    dbuscon =  g_bus_get_finish (res, NULL);
    std::cout << "got dbus!" << std::endl;

    guint sid = g_dbus_connection_signal_subscribe (
        dbuscon, /*sender*/ NULL, 
        "com.example.TestService",
        /*const gchar *member*/ NULL,
        "/com/example/TestService/object",
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        &signal_handler,
        NULL,
        NULL);

    std::cout << "sid" << sid << std::endl;

    std::cout << "send signal!" << std::endl;
    send_signal("hello",43,dbuscon);
    std::cout << "done!" << std::endl;
}

int main (int argc, char *argv[])
{

//  g_type_init ();

  g_bus_get (G_BUS_TYPE_SESSION,
           NULL,
           &got_dbus,
           NULL);


  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);

  return 0;
}