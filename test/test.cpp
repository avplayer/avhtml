
#include <html5.hpp>
#include <fstream>
#include <iostream>
#include <boost/locale.hpp>

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

		ifs >> std::noskipws;

		std::string test_page((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
		cu_page.append_partial_html(test_page);

		auto charset = cu_page.charset();

		auto dom_text = cu_page[argv[2]].to_plain_text();

		std::cout << boost::locale::conv::between(dom_text, "UTF-8", charset) << std::endl;
	}
}

void test()
{
	html::dom page;

	page.append_partial_html("<html><head>");
	page.append_partial_html("<title>hello world</title");
	page.append_partial_html("></head></html>");

	assert(page["title"].to_plain_text() == "hello world" );
}
