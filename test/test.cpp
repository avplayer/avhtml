
#include <html5.hpp>
#include <fstream>
#include <iostream>

const char msg_usage[] = "\nusage : %s <html file name> <selector>\n\n";

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		if (argc > 0)
			printf(msg_usage, argv[0]);
		else
			printf(msg_usage, "html5test");
	}
	else {
		html::dom cu_page;
		std::ifstream ifs(argv[1]);

		std::string test_page((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
		cu_page.append_partial_html(test_page);
		std::cout << cu_page[argv[2]].to_plain_text() << std::endl;
	}
}
