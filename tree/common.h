#ifndef _MOL_DEF_GUARD_DEFINE_MTK_COMMONS_H_DEF_GUARD_
#define _MOL_DEF_GUARD_DEFINE_MTK_COMMONS_H_DEF_GUARD_

#include <vector>
#include <string>
#include <sstream>

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
