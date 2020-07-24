#include <stdio.h>
#include <sys/auxv.h>
#include <string>
#include <unistd.h>
#include <limits.h>
#include <iostream>

int main() 
{

    printf("%s\n", (char *)getauxval(AT_EXECFN));

    char result[ PATH_MAX ];
    ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
    std::string appPath = std::string( result, (count > 0) ? count : 0 );

    std::size_t found = appPath.find_last_of("/\\");
    std::string p = appPath.substr(0,found);
    std::cout << p << std::endl;
    return 0;
}

