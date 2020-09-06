#include <string>
#include <map>



extern char _binary_index_html_start; 
extern char _binary_index_html_end; 
extern char _binary_test_ui_xml_start; 
extern char _binary_test_ui_xml_end; 

std::map<std::string,std::string> resources = {

    { "index.html", std::string( (const char*) &_binary_index_html_start, (int)(&_binary_index_html_end  - &_binary_index_html_start)) },
    { "test.ui.xml", std::string( (const char*) &_binary_test_ui_xml_start, (int)(&_binary_test_ui_xml_end  - &_binary_test_ui_xml_start)) },
    { "", std::string("") }
};
