
#pragma once

#include <type_traits>
#include <memory>
#include <functional>

#include <string>
#include <vector>
#include <map>

#include <boost/coroutine/asymmetric_coroutine.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/proto/traits.hpp>

#ifdef _MSC_VER
#	define noexcept throw()
#endif

namespace html{

	template<typename CharType>
	class basic_dom;
	namespace detail { template<typename CharType> class basic_dom_node_parser;}

	template<typename CharType>
	class basic_selector
	{
	public:
		basic_selector(const std::basic_string<CharType>&);
		basic_selector(std::basic_string<CharType>&&);

		basic_selector(const CharType* s)
			: basic_selector(std::basic_string<CharType>(s))
		{}

		template<int N>
		basic_selector(const CharType s[N])
			: basic_selector(std::basic_string<CharType>(s))
		{};

		friend class basic_dom<CharType>;
		friend class detail::basic_dom_node_parser<CharType>;

	protected:
		struct condition
		{
			std::basic_string<CharType> matching_tag_name;
			std::basic_string<CharType> matching_id;
			std::basic_string<CharType> matching_class;
			std::basic_string<CharType> matching_name;
			std::basic_string<CharType> matching_index;
			std::basic_string<CharType> matching_attr;
			std::basic_string<CharType> matching_attr_value;
			std::basic_string<CharType> matching_attr_operator;

			// 判断 basic_dom<CharType> 是否与当前的 condition 一致
			bool operator()(const basic_dom<CharType>&, int&) const;
		};

		struct selector_matcher{
			// 轮询 m_conditions ，判断是否存在与该 basic_dom 对象一致的 condition
			bool operator()(const basic_dom<CharType>&) const;

		private:
			bool all_match = false;
			std::vector<condition> m_conditions;

			friend class basic_selector;
		};
		typedef typename std::vector<selector_matcher>::const_iterator selector_matcher_iterator;

		selector_matcher_iterator begin() const {
			return m_matchers.begin();
		}

		selector_matcher_iterator end() const {
			return m_matchers.end();
		}

	private:
		void build_matchers();

		std::vector<selector_matcher> m_matchers;

		std::basic_string<CharType> m_select_string;
	};

	enum tag_stage{
		tag_open,
		tag_close,
	};

	namespace detail {
		template<typename CharType>
		class basic_dom_node_parser
		{
			basic_dom_node_parser& operator = (basic_dom_node_parser&&)  = delete;
			basic_dom_node_parser& operator = (const basic_dom_node_parser&) = delete;

			basic_dom_node_parser(html::basic_dom<CharType>* domer, const std::basic_string<CharType>& str);
			basic_dom_node_parser(basic_dom_node_parser&& other);

		public: // only for signals2
			basic_dom_node_parser(const basic_dom_node_parser&);
			// called from dom
			void operator()(tag_stage, std::shared_ptr<basic_dom<CharType>>);

		public: // interface
			template<typename Handler>
			typename std::enable_if<
				std::is_same<typename std::result_of<Handler(tag_stage, std::shared_ptr<basic_dom<CharType>>)>::type, void>::value
			>::type
			operator |(const Handler& node_reciver)
			{
				set_callback_fuction(decltype(m_callback)(node_reciver));
			}

			template<typename Handler>
			typename std::enable_if<std::is_function<Handler>::value>::type
			operator |(const Handler& node_reciver)
			{
				set_callback_fuction(decltype(m_callback)(node_reciver));
			}

			basic_dom_node_parser& operator |(const basic_selector<CharType>&);

		public:
			~basic_dom_node_parser();

			friend class basic_dom<CharType>;
		private:

			void set_callback_fuction(std::function<void(tag_stage, std::shared_ptr<basic_dom<CharType>>)>&& cb);

			basic_dom<CharType>* m_dom;
			const basic_selector<CharType>* m_selector;

			const std::basic_string<CharType>& m_str;

			std::function<void(tag_stage, std::shared_ptr<basic_dom<CharType>>)> m_callback;
			boost::signals2::scoped_connection m_sig_connection;
		};
	}


	template<typename CharType>
	class basic_dom : public std::enable_shared_from_this<basic_dom<CharType>>
	{
	public:

		// 默认构造.
		basic_dom(basic_dom<CharType>* parent = nullptr) noexcept;

		// 从html构造 DOM.
		explicit basic_dom(const std::basic_string<CharType>& html_page, basic_dom<CharType>* parent = nullptr);

		explicit basic_dom(const basic_dom<CharType>& d);
		basic_dom(basic_dom<CharType>&& d);
		basic_dom<CharType>& operator = (const basic_dom<CharType>& d);
		basic_dom<CharType>& operator = (basic_dom<CharType>&& d);

	public:
		// 喂入一html片段.
		detail::basic_dom_node_parser<CharType> append_partial_html(const std::basic_string<CharType>&);

	public:
		/*
		传入的 select 语法，先是通过 basic_selector 的构造函数，生成一个 basic_selector 对象
		解析完毕后，根据 basic_selector 的 condition 对象进行匹配。
		只有解析成功以后。dom 对象这个容器才会被填充进对应的内容，否则全部都为空
		*/
		basic_dom<CharType>  operator[](const basic_selector<CharType>&) const;

		std::basic_string<CharType> to_html() const;

		std::basic_string<CharType> to_plain_text() const;

		// return charset of the page if page contain meta http-equiv= content="charset="
		template<typename... Dummy, typename U = CharType>
		typename std::enable_if<std::is_same<U, char>::value, std::basic_string<CharType>>::type
		charset(const std::string& default_charset = "UTF-8") const
		{
			static_assert(sizeof...(Dummy)==0, "Do not specify template arguments!");
			return basic_charset(default_charset);
		}

		std::vector<std::shared_ptr<basic_dom<CharType>>> get_children(){
			return children;
		}

		std::basic_string<CharType> get_attr(const std::basic_string<CharType>& attr)
		{
			auto it = attributes.find(attr);

			if (it==attributes.end())
			{
				return std::basic_string<CharType>();
			}

			return it->second;
		}

	private:
		void html_parser(typename boost::coroutines::asymmetric_coroutine<const std::basic_string<CharType>*>::pull_type & html_page_source);
		typename boost::coroutines::asymmetric_coroutine<const std::basic_string<CharType>*>::push_type html_parser_feeder;
		bool html_parser_feeder_inialized = false;

		typedef std::shared_ptr<basic_dom<CharType>> basic_dom_ptr;
		boost::signals2::signal<void(tag_stage, basic_dom_ptr)> m_new_node_signal;

 		std::basic_string<CharType> basic_charset(const std::string& default_charset) const;

	protected:

		void to_html(std::basic_ostream<CharType>*, int deep) const;


		std::map<std::basic_string<CharType>, std::basic_string<CharType>> attributes;
		std::basic_string<CharType> tag_name;

		std::basic_string<CharType> content_text;
		std::vector<basic_dom_ptr> children;
		basic_dom<CharType>* m_parent;

		template<class T>
		static void dom_walk(basic_dom_ptr d, T handler);

		friend class basic_selector<CharType>;
		friend class detail::basic_dom_node_parser<CharType>;
	};

	typedef basic_dom<char> dom;
	typedef basic_dom<wchar_t> wdom;

} // namespace html
