#ifndef _MOL_DEF_GUARD_DEFINE_MTK_COMMONS_H_DEF_GUARD_
#define _MOL_DEF_GUARD_DEFINE_MTK_COMMONS_H_DEF_GUARD_

#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <regex>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <dlfcn.h>
#include <libgen.h>

#include <glib-object.h>
#include <gio/gio.h>
#include <json/json.h>
#include "gprop.h"

class JsonParseEx : public std::exception
{
public:

    JsonParseEx() {};

    JsonParseEx(const std::string& msg) : msg_(msg) {};

    virtual const char* what() const noexcept 
    {
        return msg_.c_str();
    }

    std::string msg_;
};

namespace JSON {

inline Json::Value parse(const std::string& txt)
{
        Json::Value json;
        
        Json::CharReaderBuilder rbuilder;
        std::string errs;
        std::istringstream iss(txt);
        bool ok = Json::parseFromStream(rbuilder, iss, &json, &errs);
        if(!ok)
        {
                throw JsonParseEx(errs);
        }
    return json;
}

inline const std::string stringify(Json::Value value)
{
        Json::StreamWriterBuilder wbuilder;
        return Json::writeString(wbuilder, value);

}

inline const std::string flatten(Json::Value value)
{
        Json::StreamWriterBuilder wbuilder;
        wbuilder["commentStyle"] = "None";
        wbuilder["indentation"] = ""; 
        return Json::writeString(wbuilder, value);
}

} // end namespace

inline std::vector<std::string> split(const std::string& txt, const char seperator)
{
    std::vector<std::string> result;
    size_t startpos = 0;
    size_t pos = txt.find(seperator,startpos);
    while(pos != std::string::npos)
    {
        result.push_back( txt.substr(startpos,pos-startpos) );
        startpos = pos + 1;
        pos = txt.find(seperator,startpos);
    }
    return result;
}

inline std::string escape_shell( const std::string& cmd )
{
    std::ostringstream oss;
    const gchar* p= cmd.c_str();
    while(*p)
    {
        if( *p == '\'')
        {
            oss << "\\'";
        }
        else
        {
            oss << *p;
        }
        p++;
    }        
    return oss.str();
}

inline std::vector<std::string> listdir(const std::string& dirname)
{
    std::vector<std::string> result;

    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir (dirname.c_str())) != NULL) 
    {
        while ((ent = readdir (dir)) != NULL) 
        {
            int l = strlen(ent->d_name);
            if ( l == 1 && ent->d_name[0] == '.')
            {
                continue;
            }
            if ( l == 2 && ent->d_name[0] == '.' && ent->d_name[1] == '.')
            {
                continue;
            }

            result.push_back( ent->d_name );
        }
        closedir (dir);
    } 
    return result;
}

#endif
