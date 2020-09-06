#!/bin/bash

echo -e "#include <string>\n#include <map>\n\n"

echo

for i in resources/*.* ; do

	i=$(basename $i)
	
	if [ "$i" = "Makefile" ] ; then continue; fi
	if [ "$i" = "index" ] ; then continue; fi

	x=$(echo -n $i | sed 's/\./_/g' )
	
	echo 'extern char _binary_'$x'_start; '
	echo 'extern char _binary_'$x'_end; '		
done

echo
echo -e "std::map<std::string,std::string> resources = {"
echo

for i in resources/*.* ; do

	i=$(basename $i)
	if [ "$i" = "Makefile" ] ; then continue; fi
	if [ "$i" = "index" ] ; then continue; fi

	x=$(echo -n $i | sed 's/\./_/g' )
	
	echo -n '    { "'$i'", std::string( (const char*) '
	echo -n '&_binary_'$x'_start, '
	echo -n '(int)(&_binary_'$x'_end  - '
	echo    '&_binary_'$x'_start)) },'
done

echo '    { "", std::string("") }' 
echo '};'

