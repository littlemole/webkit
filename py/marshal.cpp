#include "marshal.h"
#include "pyglue.h"

bool gvariant_is_number(GVariant* parameter)
{
    const GVariantType * t = g_variant_get_type(parameter);      

    if ( g_variant_type_equal (t,G_VARIANT_TYPE_BYTE)    ) return true;
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_INT16)   ) return true;
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_UINT16)  ) return true;
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_INT32)   ) return true;
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_UINT32)  ) return true;
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_INT64)   ) return true;
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_UINT64)  ) return true;
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_DOUBLE)  ) return true;

    return false;
}

double gvariant_get_number(GVariant* parameter)
{
    const GVariantType * t = g_variant_get_type(parameter);      

    if ( g_variant_type_equal (t,G_VARIANT_TYPE_BYTE)    ) return g_variant_get_byte(parameter);
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_INT16)   ) return g_variant_get_int16(parameter);
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_UINT16)  ) return g_variant_get_uint16(parameter);
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_INT32)   ) return g_variant_get_int32(parameter);
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_UINT32)  ) return g_variant_get_uint32(parameter);
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_INT64)   ) return g_variant_get_int64(parameter);
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_UINT64)  ) return g_variant_get_uint64(parameter);
    if ( g_variant_type_equal (t,G_VARIANT_TYPE_DOUBLE)  ) return g_variant_get_double(parameter);

    return 0;
}


GVariant* make_variant(PyObject* pyObj)
{
//    PyObject_Print(pyObj, stdout,0);
//    printf("\n");

    if (!pyObj)
    {
        return g_variant_new_maybe (G_VARIANT_TYPE_VARIANT,NULL);
    }

    if ( PyUnicode_Check(pyObj) ) 
    {
        const char* c = PyUnicode_AsUTF8(pyObj);
        return g_variant_new("s",c);                       
    }
    else if ( PyByteArray_Check(pyObj) )
    {
        char* c = PyByteArray_AsString(pyObj);
        return g_variant_new("s",c);                       
    }
    else if ( pyObj == Py_None ) 
    {
       return g_variant_new_maybe (G_VARIANT_TYPE_VARIANT,NULL);
    }
    else if ( PyBool_Check(pyObj) ) 
    {
        bool b = pyObj == Py_True;
        return g_variant_new_boolean(b);               
    }
    else if (PyLong_Check(pyObj) ) 
    {
        long l = PyLong_AsLong(pyObj);
        return g_variant_new("x",l);
    }
    else if (PyFloat_Check(pyObj) ) 
    {
        double d = PyFloat_AsDouble(pyObj);
        return g_variant_new_double(d);
    }
    else if (PyTuple_CheckExact(pyObj)) 
    {
        Py_ssize_t len = PySequence_Size(pyObj);

        GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE_TUPLE);
        for(Py_ssize_t i = 0; i < len; i++)
        {
            PyObjectRef item = PySequence_GetItem(pyObj,i);
            g_variant_builder_add(builder, "v", make_variant(item));
        }
        return g_variant_builder_end(builder);
    }
    else if (PyList_CheckExact(pyObj)) 
    {
        Py_ssize_t len = PySequence_Size(pyObj);
        GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE_TUPLE);
        for(Py_ssize_t i = 0; i < len; i++)
        {
            PyObjectRef item = PySequence_GetItem(pyObj,i);
            g_variant_builder_add(builder, "v", make_variant(item));
        }
        return g_variant_builder_end(builder);
    }
    else if (PyDict_CheckExact(pyObj))
    {
        GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);

        PyObjectRef keys = PyDict_Keys(pyObj);
        Py_ssize_t len = PySequence_Size(keys);

        for(Py_ssize_t i = 0; i < len; i++)
        {
            PyObjectRef key = PySequence_GetItem(keys,i);

            const char* k = PyUnicode_AsUTF8(key);
            PyObjectRef value = PyMapping_GetItemString(pyObj, k);

            GVariant* dict = g_variant_new("{sv}", k,make_variant(value));
            g_variant_builder_add_value(builder,dict);
        }  
        return g_variant_builder_end(builder);
    }

    return g_variant_new_maybe (G_VARIANT_TYPE_VARIANT,NULL);
}


PyObject* gvariant_to_py_value(GVariant* parameter)
{
    if (!parameter)
    {
        Py_RETURN_NONE;
    }

    const GVariantType * t = g_variant_get_type(parameter);      

    if ( g_variant_type_equal (t,G_VARIANT_TYPE_STRING)  ) 
    {
        const char* c = g_variant_get_string (parameter,NULL);
        return PyUnicode_FromString(c);
    }
    else if ( g_variant_type_equal (t,G_VARIANT_TYPE_VARIANT) )
    {
        GVariant* v = g_variant_get_variant(parameter);
        return gvariant_to_py_value(v);
    }
    else if ( g_variant_type_is_tuple (t) )
    {
        gsize s = g_variant_n_children (parameter);
        PyObject* ret = PyTuple_New(s);

        for( gsize i = 0; i < s; i++)
        {
            GVariant* v = g_variant_get_child_value (parameter,i);
            PyObject* p = gvariant_to_py_value(v);
            PyTuple_SetItem(ret, i, p);
            g_variant_unref(v);
        }

        return ret;
    }
    else if ( g_variant_type_equal (t,G_VARIANT_TYPE_MAYBE) )
    {
        GVariant* v = g_variant_get_maybe(parameter);
        if(!v)
        {
            Py_RETURN_NONE;
        }
        else
        {
            auto ret = gvariant_to_py_value(v);
            g_variant_unref(v);
            return ret;
        }
    }
    else if ( g_variant_type_equal (t,G_VARIANT_TYPE_BOOLEAN) )
    {
        return PyBool_FromLong((long)g_variant_get_boolean(parameter));
    }    
    else if (gvariant_is_number(parameter) ) 
    {
        return PyFloat_FromDouble(gvariant_get_number(parameter));
    }
    else if (g_variant_type_equal (t,G_VARIANT_TYPE_VARDICT)) 
    {
        PyObject* ret = PyDict_New();

        GVariantIter iter;
        GVariant *value;
        gchar *key;

        g_variant_iter_init (&iter, parameter);
        while (g_variant_iter_next (&iter, "{sv}", &key, &value))
        {
            PyObjectRef val = gvariant_to_py_value(value);
            PyObjectRef k = PyUnicode_FromString(key);

            PyDict_SetItem(ret, k, val );

            /* must free data for ourselves */
            g_variant_unref (value);
            g_free (key);
        }
        return ret;      
    }
    else if ( g_variant_type_equal (t,G_VARIANT_TYPE_ARRAY) )
    {
        gsize s = g_variant_n_children (parameter);
        PyObject* ret = PyList_New(s);

        for( gsize i = 0; i < s; i++)
        {
            GVariant* v = g_variant_get_child_value (parameter,i);
            PyObject* val = gvariant_to_py_value(v);
            PyList_SetItem(ret, i,val );
            g_variant_unref(v);
        }

        return ret;
    }    

    std::cout << "G_VARIANT_TYPE_UNKNOWN ******************" << std::endl;
    Py_RETURN_NONE;
}