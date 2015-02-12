
#include "html5.hpp"

#include <boost/regex.hpp>

html::selector::selector(const std::string& s)
	: m_select_string(s)
{
	build_matchers();
}

html::selector::selector(std::string&&s)
	: m_select_string(s)
{
	build_matchers();
}

static bool strcmp_ignore_case(const std::string& a, const std::string& b)
{
	if ( a.size() == b.size())
		return  strncasecmp(a.c_str(), b.c_str(), a.size()) == 0;
	return false;
}

template<class GetChar, class GetEscape, class StateEscaper>
static std::string _get_string(GetChar&& getc, GetEscape&& get_escape, char quote_char, StateEscaper&& state_escaper)
{
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
			state_escaper();
		}
		return ret;
}

/*
 * matcher 是分代的
 * matcher 会先执行一次匹配, 然后再从上一次执行的结果里执行匹配
 *
 * 比如对于 "div p" 这样的选择器, 是先匹配 div , 然后在结果中匹配 p
 *
 * 又比如 "div.p p" 先匹配 class=p 的 div 然后在结果中匹配 p
 *
 * 也就是说, 空格表示从上一次的结果里匹配.
 * 也就是说匹配的代用空格隔开.
 *
 * 同代的匹配条件里, 是属于 and 关系, 要同时满足
 * 比如 div.p 是同代的 2 个条件. 需要同时满足
 */
void html::selector::build_matchers()
{
	selector_matcher matcher;

	if (m_select_string == "*")
	{
		matcher.all_match = true;
		// 所有的类型
		m_matchers.push_back(matcher);
		return;
	}
	// 从 选择字符构建匹配链
	auto str_iterator = m_select_string.begin();

	int state = 0;

	auto getc = [this, &str_iterator]() -> char {
		if (str_iterator != m_select_string.end())
			return *str_iterator++;
		return 0;
	};

	auto get_escape = [&getc]() -> char
	{
		return getc();
	};

	auto get_string = [&getc, &get_escape, &state](char quote_char){
		return _get_string(getc, get_escape, quote_char, [&state](){
			state = 0;
		});
	};

	std::string matcher_str;


	char c;

	do
	{
		c = getc();
#	define METACHAR  0 : case '.' : case '#'
		switch(state)
		{
			case 0:
			case '#':
			case '.':
				switch(c)
				{
					case '\\':
						matcher_str += get_escape();
						break;
					case METACHAR: case ' ':
						{
							if (!matcher_str.empty())
							{
								condition match_condition;

								switch(state)
								{
									case 0:
										match_condition.matching_tag_name = std::move(matcher_str);
										break;
									case '#':
										match_condition.matching_id = std::move(matcher_str);
										break;
									case '.':
										match_condition.matching_class = std::move(matcher_str);
										break;
								}
								matcher.m_conditions.push_back(match_condition);
							}
							state = c;

							if ( c == ' ')
								state = 0;

							if (state == 0)
							{
								m_matchers.push_back(std::move(matcher));
							}
						}
						break;
					case '[':
						state = '[';
						break;
					default:
						matcher_str += c;
				}
				break;
			case ' ':
			{
				m_matchers.push_back(std::move(matcher));
				state = 0;
			}
			break;
			case ':':
			{
				// 冒号暂时不实现
				switch(c)
				{
					case METACHAR:
						state = c;
						break;
				}

			}break;
			case '[':// 暂时不实现
			{
				switch(c)
				{
					case ']': // 匹配结束
						state = 0;
					case ' ':
					{
						condition match_condition;

						match_condition.matching_attr = std::move(matcher_str);
						matcher.m_conditions.push_back(match_condition);
						break;
					}
					case METACHAR:
					break;
					default:
						matcher_str += c;
				}
			}
			break;
		}
#	undef METACHAR

	}while(c);
}

html::dom::dom(dom* parent) noexcept
	: m_parent(parent)
{
}

html::dom::dom(const std::string& html_page, dom* parent)
	: dom(parent)
{
	append_partial_html(html_page);
}

html::dom::dom(html::dom&& d)
	: attributes(std::move(d.attributes))
	, tag_name(std::move(d.tag_name))
	, content_text(std::move(d.content_text))
	, m_parent(std::move(d.m_parent))
	, children(std::move(d.children))
	, html_parser_feeder(std::move(d.html_parser_feeder))
	, html_parser_feeder_inialized(std::move(d.html_parser_feeder_inialized))
{
}

html::dom::dom(const html::dom& d)
	: attributes(d.attributes)
	, tag_name(d.tag_name)
	, content_text(d.content_text)
	, m_parent(d.m_parent)
	, children(d.children)
//	, html_parser_feeder(nullptr)
	, html_parser_feeder_inialized(false)
{
}

html::dom& html::dom::operator=(const html::dom& d)
{
	attributes = d.attributes;
	tag_name = d.tag_name;
	content_text = d.content_text;
	m_parent = d.m_parent;
	children = d.children;
	html_parser_feeder_inialized = false;
	return *this;
}

html::dom& html::dom::operator=(html::dom&& d)
{
	attributes = std::move(d.attributes);
	tag_name = std::move(d.tag_name);
	content_text = std::move(d.content_text);
	m_parent = std::move(d.m_parent);
	children = std::move(d.children);
	html_parser_feeder_inialized = false;
	return *this;
}

bool html::dom::append_partial_html(const std::string& str)
{
	if (!html_parser_feeder_inialized)
	{
		html_parser_feeder = boost::coroutines::asymmetric_coroutine<char>::push_type(std::bind(&dom::html_parser, this, std::placeholders::_1));
		html_parser_feeder_inialized = true;
	}

	for(auto c: str)
		html_parser_feeder(c);
	return true;
}

template<class Handler>
void html::dom::dom_walk(html::dom_ptr d, Handler handler)
{
	if(handler(d))
		for (auto & c : d->children)
		{
			if (c->tag_name != "<!--")
				dom_walk(c, handler);
		}
}

bool html::selector::condition::operator()(const html::dom& d) const
{
	if (!matching_tag_name.empty())
	{
		return strcmp_ignore_case(d.tag_name, matching_tag_name);
	}
	if (!matching_id.empty())
	{
		auto it = d.attributes.find("id");
		if ( it != d.attributes.end())
		{
			return it->second == matching_id;
		}
	}
	if (!matching_class.empty())
	{
		auto it = d.attributes.find("class");
		if ( it != d.attributes.end())
		{
			return it->second == matching_class;
		}
	}

	if(!matching_attr.empty())
	{
		auto it = d.attributes.find(matching_attr);
		return it != d.attributes.end();
	}

	return false;
}

bool html::selector::selector_matcher::operator()(const html::dom& d) const
{
	if (this->all_match)
		return true;

	bool all_match = false;

	for (auto& c : m_conditions)
	{
		if(c(d))
		{
			continue;
		}
		return false;
	}

	return true;
}

html::dom html::dom::operator[](const selector& selector_) const
{
	html::dom selectee_dom;
	html::dom matched_dom(*this);

	for (auto & matcher : selector_)
	{
		selectee_dom = std::move(matched_dom);

		for( auto & c : selectee_dom.children)
		{
			dom_walk(c, [this, &matcher, &matched_dom, selector_](html::dom_ptr i)
			{
				bool no_match = true;

				if (matcher(*i))
				{
					no_match = false;
				}else
					no_match = true;

				if (!no_match)
					matched_dom.children.push_back(i);
				return no_match;
			});
		}
	}

	return matched_dom;
}

static inline std::string get_char_set( std::string type,  const std::string & header )
{
	boost::cmatch what;
	// 首先是 text/html; charset=XXX
	boost::regex ex( "charset=([a-zA-Z0-9\\-_]+)" );
	boost::regex ex2( "<meta charset=[\"\']?([a-zA-Z0-9\\-_]+)[\"\']?" );

	if( boost::regex_search( type.c_str(), what, ex ) )
	{
		return what[1];
	}
	else if( boost::regex_search( type.c_str(), what, ex2 ) )
	{
		return what[1];
	}
	else if( boost::regex_search( header.c_str(), what, ex ) )
	{
		return what[1];
	}
	else if( boost::regex_search( header.c_str(), what, ex2 ) )
	{
		return what[1];
	}

	return "utf8";
}

std::string html::dom::charset() const
{
	std::string cset;
	auto charset_dom = (*this)["meta [http-equiv][content]"];

	for (auto & c : charset_dom.children)
	{
		dom_walk(c, [this, &cset](html::dom_ptr i)
		{
			if (strcmp_ignore_case(i->attributes["http-equiv"], "content-type"))
			{
				auto content= i->attributes["content"];

				cset = get_char_set(content, "charset=utf8");
				return false;
			}
			return true;
		});
	}
	return cset;
}

std::string html::dom::to_plain_text() const
{
	std::string ret;

	if (!strcmp_ignore_case(tag_name, "script") && tag_name != "<!--")
	{
		ret += content_text;

		for ( auto & c : children)
		{
			ret += c->to_plain_text();
		}
	}

	return ret;
}

void html::dom::to_html(std::ostream* out, int deep) const
{
	if (!tag_name.empty())
	{
		for (auto i = 0; i < deep; i++)
			*out << ' ';

		if (tag_name!="<!--")
			*out << "<" << tag_name;
		else{
			*out << tag_name;
		}

		if (!attributes.empty())
		{
			for (auto a : attributes)
			{
				*out << ' ';
				*out << a.first << "=" << a.second;
			}
		}
		if (tag_name!="<!--")
			*out << ">\n";
		else{
			*out << content_text;
			*out << "-->\n";
		}
	}else
	{
		for (auto i = 0; i < deep +1; i++)
			*out << ' ';
		*out << content_text << "\n";
	}

	for ( auto & c : children)
	{
		c->to_html(out, deep + 1);
	}

	if (!tag_name.empty())
	{
		if (tag_name!="<!--")
		{
			for (auto i = 0; i < deep; i++)
				*out << ' ';
			*out << "</" << tag_name << ">\n";
		}
	}
}

std::string html::dom::to_html() const
{
	std::stringstream ret;
	to_html(&ret, -1);
	return ret.str();
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
		return _get_string(getc, get_escape, quote_char, [&pre_state, &state](){
			pre_state = state;
			state = 0;
		});
	};

	std::string tag; //当前处理的 tag
	std::string content; // 当前 tag 下的内容
	std::string k,v;

	dom * current_ptr = this;

	char c;

	bool ignore_blank = true;
	std::vector<int> comment_stack;

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
							{
								auto content_node = std::make_shared<dom>(current_ptr);
								content_node->content_text = std::move(content);
								current_ptr->children.push_back(std::move(content_node));
							}
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
						if(new_dom->tag_name[0] != '!')
							current_ptr = new_dom.get();
					}
					break;
					case '/':
						pre_state = state;
						state = 5;
					break;
					case '!':
						pre_state = state;
						state = 10;
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
						if ( current_ptr->tag_name[0] == '!')
						{
							current_ptr = current_ptr->m_parent;
						}
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
					case '>':
					{
						pre_state = state;
						state = 0;
						current_ptr->attributes[k] = "";
						k.clear();
						v.clear();
						if ( current_ptr->tag_name[0] == '!')
						{
							current_ptr = current_ptr->m_parent;
						}
					}break;
					default:
						v += c;
				}
			}
			break;
			case 5: // 解析 </xxx>
			{
				switch(c)
				{
					case '>':
					{
						if(!tag.empty())
						{
							state = 0;
							// 来, 关闭 tag 了
							// 注意, HTML 里, tag 可以越级关闭

							// 因此需要进行回朔查找

							auto _current_ptr = current_ptr;

							while (_current_ptr && !strcmp_ignore_case(_current_ptr->tag_name, tag))
							{
								_current_ptr = _current_ptr->m_parent;
							}

							if (!_current_ptr)
							{
								// 找不到对应的 tag 要咋关闭... 忽略之
								tag.clear();

								break;
							}

							tag.clear();

							current_ptr = _current_ptr;

							// 找到了要关闭的 tag

							// 那就退出本 dom 节点
							if (current_ptr->m_parent)
								current_ptr = current_ptr->m_parent;
							else
								current_ptr = this;
						}else
						{
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

			case 10:
			{
				// 遇到了 <! 这种,
				switch(c)
				{
					case '-':
						tag += c;
						state = 11;
						break;
					default:
						tag += c;
						state = pre_state;
				}
			}break;
			case 11:
			{
				// 遇到了 <!- 这种,
				switch(c)
				{
					case '-':
						state = 12;
						comment_stack.push_back(pre_state);
						tag.clear();
						break;
					default:
						tag += c;
						state = pre_state;
				}

			}break;
			case 12:
			{
				// 遇到了 <!-- 这种,
				// 要做的就是一直忽略直到  -->
				switch(c)
				{
					case '<':
					{
						c = getc();
						if ( c == '!')
						{
							pre_state = state;
							state = 10;
						}else{
							content += '<';
							content += c;
						}
					}break;
					case '-':
					{
						pre_state = state;
						state = 13;
					}
					default:
						content += c;
				}
			}break;
			case 13: // 遇到 -->
			{
				switch(c)
				{
					case '-':
					{
						state = 14;
					}break;
					default:
						content += c;
						state = pre_state;
				}
			}break;
			case 14: // 遇到 -->
			{
				switch(c)
				{
					case '>':
					{
						if (pre_state == 12)
							content.pop_back();
						comment_stack.pop_back();
						state = comment_stack.empty()? 0 : 12;
						dom_ptr comment_node = std::make_shared<dom>(current_ptr);
						comment_node->tag_name = "<!--";
						comment_node->content_text = std::move(content);
						current_ptr->children.push_back(comment_node);
					}break;
					default:
						content += c;
						state = pre_state;
				}
			}break;
		}
	}
}

#undef CASE_BLANK


