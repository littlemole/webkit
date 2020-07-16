#include "marshal.h"
#include "pyglue.h"

GVariant* make_variant(pyobj_ref& obj)
{
//    PyObject_Print(pyObj, stdout,0);
//    printf("\n");

    pyobj pyObj(obj);

    if (!pyObj.isValid())
    {
        return gnull();
    }

    if( pyObj.isString() || pyObj.isByteArray() )
    {
        return g_variant_new_string(pyObj.str());                       
    }
    else if ( pyObj.isNone() ) 
    {
       return gnull();
    }
    else if ( pyObj.isBoolean() )
    {
        bool b = pyObj.boolean();
        return g_variant_new_boolean(b);               
    }
    else if ( pyObj.isLong() ) 
    {
        long l = pyObj.integer();
        return g_variant_new_int64 (l);
    }
    else if ( pyObj.isFloat() )
    {
        double d = pyObj.number();
        return g_variant_new_double(d);
    }
    else if ( pyObj.isTuple() || pyObj.isList() )
    {
        gvar_builder builder = gtuple();

        pyObj.for_each( [&builder] (int index, pyobj_ref& item)
        {
            builder.add(make_variant(item));
        });

        return builder.build(); 
    }
    else if ( pyObj.isDict() )
    {
        gvar_builder builder = garray();

        pyObj.for_each( [&builder] ( const char* key, pyobj_ref& value)
        {
            builder.add(key,make_variant(value));
        });

        return builder.build(); 
    }

    return g_variant_new_maybe (G_VARIANT_TYPE_VARIANT,NULL);
}


PyObject* gvariant_to_py_value(GVariant* parameter)
{
    if (!parameter)
    {
        Py_RETURN_NONE;
    }

    gvar param(parameter);

    if ( param.isString() )
    {
        return PyUnicode_FromString(param.str());
    }
    else if ( param.isVariant())
    {
        GVariant* v = param.variant();
        return gvariant_to_py_value(v);
    }
    else if (param.isTuple())
    {
        int len = param.length();
        PyObject* ret = PyTuple_New(len);

        param.for_each( [&ret](int index, GVariant* item)
        {
            PyObject* p = gvariant_to_py_value(item);
            PyTuple_SetItem(ret, index, p); // steals ref to p!
        });

        return ret;
    }
    else if ( param.isMaybe() )
    {
        GVariant* v = param.maybe();
        if(!v)
        {
            Py_RETURN_NONE;
        }
        else
        {
            PyObject* ret = gvariant_to_py_value(v);
            g_variant_unref(v);
            return ret;
        }
    }
    else if (param.isBoolean())
    {
        return PyBool_FromLong( (long) param.boolean() );
    }    
    else if ( param.isNumber() )
    {
        return PyFloat_FromDouble(param.number());
    }
    else if ( param.isDict())
    {
        PyObject* ret = PyDict_New();

        param.for_each( [&ret](const char* key, GVariant* value)
        {
            pyobj_ref val = gvariant_to_py_value(value);
            PyDict_SetItemString(ret, key, val ); // does NOT steal val
        });

        return ret;      
    }
    else if ( param.isArray() )
    {
        int len = param.length();

        PyObject* ret = PyList_New(len);

        param.for_each( [&ret]( int index, GVariant* item)
        {
            PyObject* value = gvariant_to_py_value(item);
            PyList_SetItem(ret, index, value ); // steals ref to value
        });

        return ret;
    }    

    std::cout << "G_VARIANT_TYPE_UNKNOWN ******************" << std::endl;
    Py_RETURN_NONE;
}