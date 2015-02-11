
#include "html5.hpp"

html::selector::selector(const std::string& s)
	: m_select_string(s)
{
}

html::selector::selector(std::string&&s)
	: m_select_string(s)
{
}

html::dom::dom(dom* parent) noexcept
    : html_parser_feeder(std::bind(&dom::html_parser, this, std::placeholders::_1))
	, m_parent(parent)
{
}

html::dom::dom(const std::string& html_page, dom* parent)
	: dom(parent)
{
	append_partial_html(html_page);
}

bool html::dom::append_partial_html(const std::string& str)
{
	for(auto c: str)
		html_parser_feeder(c);
}

html::dom html::dom::operator[](const selector& selector_)
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
	int pre_state = 0, state = 0;

	auto getc = [&html_page_source](){
		auto c = html_page_source.get();
		html_page_source();
		return c;
	};

	auto get_escape = [&getc]() -> char
	{
		return getc();
	};

	auto get_string = [&getc, &get_escape, &pre_state, &state](char quote_char){

		std::string ret;

		auto c = getc();

		while ( (c != quote_char) && (c!= '\n'))
		{
			if ( c == '\'' )
			{
				c += get_escape();
			}else
			{
				ret += c;
			}
			c = getc();
		}

		if (c == '\n')
		{
			pre_state = state;
			state = 0;
		}

		return ret;
	};


	std::string tag; //当前处理的 tag
	std::string content; // 当前 tag 下的内容
	std::string k,v;

	dom * current_ptr = this;

	char c;

	bool ignore_blank = true;

	while(html_page_source) // EOF 检测
	{
		// 获取一个字符
		c = getc();

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
							if (!content.empty())
								current_ptr->contents.push_back(std::move(content));
							ignore_blank = (tag != "script");
						}
					}
					break;
					CASE_BLANK :
					if(ignore_blank)
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

							dom_ptr new_dom = std::make_shared<dom>(current_ptr);
							new_dom->tag_name = std::move(tag);

							current_ptr->children.push_back(new_dom);

							current_ptr = new_dom.get();
						}
					}
					break;
					case '>': // tag 解析完毕, 正式进入 下一个 tag
					{
						pre_state = state;
						state = 0;

						dom_ptr new_dom = std::make_shared<dom>(current_ptr);
						new_dom->tag_name = std::move(tag);
						current_ptr->children.push_back(new_dom);
						current_ptr = new_dom.get();
					}
					break;
					case '/':
						pre_state = state;
						state = 5;
					break;
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
					case '>': // 马上就关闭 tag 了啊
					{
						// tag 解析完毕, 正式进入 下一个 tag
						pre_state = state;
						state = 0;
						if ( current_ptr->tag_name[0] == '!')
						{
							current_ptr = current_ptr->m_parent;
						}
					}break;
					case '/':
					{
						// 直接关闭本 tag 了
						// 下一个必须是 '>'
						c = getc();

						if (c!= '>')
						{
							// TODO 报告错误
						}

						pre_state = state;
						state = 0;

						if (current_ptr->m_parent)
							current_ptr = current_ptr->m_parent;
						else
							current_ptr = this;
					}break;
					case '\"':
					case '\'':
					{
						pre_state = state;
						state = 3;
						k = get_string(c);
					}break;
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
					}break;
					case '>':
					{
						pre_state = state;
						state = 0;
						current_ptr->attributes[k] = "";
						k.clear();
						v.clear();
					}
					break;
					default:
						k += c;
				}
			}break;
			case 4: // 进入 attribute 解析 value
			{
				switch (c)
				{
					case '\"':
					case '\'':
					{
						v = get_string(c);
					}
					CASE_BLANK :
					{
						state = 2;
						current_ptr->attributes[k] = std::move(v);
						k.clear();
					}
					break;
					default:
						v += c;
				}
			}
			break;
			case 5:
			{
				switch(c)
				{
					case '>':
					{

						if(!tag.empty())
						{
							// 来, 关闭 tag 了
							// 注意, HTML 里, tag 可以越级关闭

							// 因此需要进行回朔查找

							auto _current_ptr = current_ptr;

							while (_current_ptr && _current_ptr->tag_name != tag)
							{
								_current_ptr = _current_ptr->m_parent;
							}

							tag.clear();
							if (!_current_ptr)
							{
								// 找不到对应的 tag 要咋关闭... 忽略之
								break;
							}

							current_ptr = _current_ptr;

							// 找到了要关闭的 tag

							// 那就退出本 dom 节点
							if (current_ptr->m_parent)
								current_ptr = current_ptr->m_parent;
							else
								current_ptr = this;
							state = 0;
						}
					}break;
					CASE_BLANK:
					break;
					default:
					{
						// 这个时候需要吃到  >
						tag += c;
					}
				}
				break;
			}
		}
	}
}

#undef CASE_BLANK


