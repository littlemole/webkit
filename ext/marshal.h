#ifndef _MOL_DEF_GUARD_DEFINE_GVARIANT_MARSHAL_DEF_GUARD_
#define _MOL_DEF_GUARD_DEFINE_GVARIANT_MARSHAL_DEF_GUARD_

#include <gio/gio.h>
#include <memory>
#include "jsglue.h"

bool gvariant_is_number(GVariant* parameter);
double gvariant_get_number(GVariant* parameter);

JSValueRef gvariant_to_js_value(JSContextRef context, GVariant* parameter);
std::vector<JSValueRef> gvariant_to_js_values(JSContextRef context, GVariant* parameters);

GVariant* make_variant(JSContextRef context, JSValueRef jsvalue);
GVariant* js_object_to_gvariant(JSContextRef context, JSValueRef arg);
GVariant* js_array_to_gvariant(JSContextRef context, JSValueRef arg);


#endif
