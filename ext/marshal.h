#ifndef _MOL_DEF_GUARD_DEFINE_GVARIANT_MARSHAL_DEF_GUARD_
#define _MOL_DEF_GUARD_DEFINE_GVARIANT_MARSHAL_DEF_GUARD_

#include <gio/gio.h>
#include <memory>
#include "jsglue.h"


JSValueRef gvariant_to_js_value(JSContextRef context, GVariant* parameter);
std::vector<JSValueRef> gvariant_to_js_values(JSContextRef context, GVariant* parameters);

GVariant* make_variant(jsval& arg);


#endif
