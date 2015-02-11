
#include "html5.hpp"

html::dom::dom() noexcept
    : html_parser_feeder(std::bind(&dom::html_parser, this, std::placeholders::_1))
{
}

html::dom::dom(const std::string& html_page)
	: dom()
{
	append_partial_html(html_page);
}

bool html::dom::append_partial_html(const std::string& str)
{
	for(auto c: str)
		html_parser_feeder(c);
}

html::dom html::dom::operator[](const std::string& selector)
{
	html::dom ret_dom;


	

	return ret_dom;
}

std::string html::dom::to_plain_text()
{
	std::string ret;
	return ret;
}


#define CASE_BLANK case ' ': case '\r': case '\n': case '\t'

void html::dom::html_parser(boost::coroutines::asymmetric_coroutine<char>::pull_type& html_page_source)
{
	auto getc = [&html_page_source](){
		html_page_source();
		return html_page_source.get();
	};

	int pre_state=0, state = 0;

	std::string tag; //当前处理的 tag
	std::string content; // 当前 tag 下的内容
	std::string k,v;
	std::map<std::string,std::string> attribute;

	dom * current_ptr = this;

	std::vector<dom*> tag_stack = {this};

	while(html_page_source) // EOF 检测
	{
		// 获取一个字符
		auto c = getc();

		switch(state)
		{
			case 0: // 起始状态. content 状态
			{
				switch(c)
				{
					case '<':
					{
						if (tag.empty())
						{
							// 进入 < tag 解析状态
							pre_state = state;
							state = 1;
						}
					}
					break;
					CASE_BLANK :
						break;
					default:
						content += c;
				}
			}
			break;
			case 1: // tag名字解析
			{
				switch (c)
				{
					CASE_BLANK :
					{
						if (tag.empty())
						{
							// empty tag name
							// 重新进入上一个 state
							// 也就是忽略 tag
							state = pre_state;
						}else
						{
							pre_state = state;
							state = 2;
						}
					}
					break;
					case '>':
					{
						// tag 解析完毕, 正式进入 下一个 tag
						pre_state = state;
						state = 0;

						dom_ptr new_dom = std::make_shared<dom>();

						current_ptr->tag_name = tag;
						current_ptr->children.push_back(new_dom);

						tag_stack.push_back(current_ptr);

						current_ptr = new_dom.get();

					}break;
					case '/':
					{
						pre_state = state;
						state = 7;

					}break;
					// 为 tag 赋值.
					default:
						tag += c;
				}
			}
			break;
			case 2: // tag 名字解析完毕, 进入 attribute 解析, skiping white
			{
				switch (c)
				{
					CASE_BLANK :
					break;
					default:
						pre_state = state;
						state = 3;
						k += c;
				}
			}break;
			case 3: // tag 名字解析完毕, 进入 attribute 解析 key
			{
				switch (c)
				{
					CASE_BLANK :
					{
						// empty k=v
						state = 2;
						current_ptr->attributes[k] = "";
						k.clear();
						v.clear();
					}
					break;
					case '=':
					{
						pre_state = state;
						state = 4;
					}
					default:
						k += c;
				}
			}break;
			case 4: // 进入 attribute 解析 value
			{
				switch (c)
				{
					case '\"':
						state = 5;
						break;
					case '\'':
						state = 6;
						break;
					CASE_BLANK :
					{
						state = 2;
						current_ptr->attributes[k] = v;
						k.clear();
						v.clear();
					}
					break;
					default:
						v += c;
				}
			}
			break;
			case 5:
			case 6:
			{
				switch(c)
				{
					case '\n':
					{
						// FIXME 跳到 tag 结束
						state = 0;
						current_ptr->attributes[k] = v;
						k.clear();
						v.clear();
					}break;
					case '\'':
					case '\"':
					if ( (state == 5 && c == '\"') || (state == 6 && c == '\''))
					{
						state = 2;
						current_ptr->attributes[k] = v;
						k.clear();
						v.clear();
						break;
					}
					default:
						v += c;
				}
			}break;
			case 7:
			{
				switch(c)
				{
					case '>':
					{
						// 因为上一个是  /
						// 所以如果这个是 >
						// 意味着马上关闭当前 tag

					}break;
					default:
					{
						// 这个时候需要吃到  >

					}
				}
				if (c != '>')
				{
					// 居然在  / 后面不是 >
					// 执行到这里, 一般是  <div/>
					// 这种自动关闭的, 上一个字符是 /
					// 所以切换到这个状态, 期待的下一个字符立即是 >
					state = pre_state;
				}
				break;
			}
			case 8:
			{
				switch(c)
				{
					CASE_BLANK:
					break;
					case '/':
					{

					}break;
					default:
					{

					}
				}
			}
		}
	}
}

#undef CASE_BLANK


