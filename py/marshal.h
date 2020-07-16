#ifndef _MOL_DEFGUARD_DEFINE_PMARSHAL_DEF_GUARD_
#define _MOL_DEFGUARD_DEFINE_PMARSHAL_DEF_GUARD_

#include <Python.h>
#include <structmember.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <gio/gio.h>
#include <glib.h>
#include "gvglue.h"
#include "pyglue.h"

GVariant* make_variant(pyobj_ref& pyObj);

PyObject* gvariant_to_py_value(GVariant* parameters);

#endif
