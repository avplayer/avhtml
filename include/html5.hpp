
#pragma once

#include <type_traits>
#include <memory>
#include <functional>

#include <string>
#include <vector>

#include <boost/coroutine/asymmetric_coroutine.hpp>

#ifdef _MSC_VER
#	define noexcept throw()
#endif

namespace html{

	template<typename CharType>
	class basic_dom;

// 	typedef std::shared_ptr<dom> dom_ptr;
// 	typedef std::weak_ptr<dom> dom_weak_ptr;

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

	protected:
		struct condition
		{
			std::basic_string<CharType> matching_tag_name;
			std::basic_string<CharType> matching_id;
			std::basic_string<CharType> matching_class;
			std::basic_string<CharType> matching_name;
			std::basic_string<CharType> matching_index;
			std::basic_string<CharType> matching_attr;
			bool operator()(const basic_dom<CharType>&) const;
		};

		struct selector_matcher{
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

	template<typename CharType>
	class basic_dom
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
		bool append_partial_html(const std::basic_string<CharType>&);

	public:
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

 		std::basic_string<CharType> basic_charset(const std::string& default_charset) const;

	protected:

		void to_html(std::basic_ostream<CharType>*, int deep) const;


		std::map<std::basic_string<CharType>, std::basic_string<CharType>> attributes;
		std::basic_string<CharType> tag_name;

		std::basic_string<CharType> content_text;

		typedef std::shared_ptr<basic_dom<CharType>> basic_dom_ptr;

		std::vector<basic_dom_ptr> children;
		basic_dom<CharType>* m_parent;

		template<class T>
		static void dom_walk(basic_dom_ptr d, T handler);

		friend class basic_selector<CharType>;
	};

	typedef basic_dom<char> dom;
	typedef basic_dom<wchar_t> wdom;

} // namespace html
