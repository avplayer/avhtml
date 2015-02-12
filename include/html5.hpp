
#pragma once

#include <memory>
#include <functional>

#include <string>
#include <vector>

#include <boost/coroutine/asymmetric_coroutine.hpp>

#ifdef _MSC_VER
#	define noexcept throw()
#endif

namespace html{

	class dom;
	typedef std::shared_ptr<dom> dom_ptr;
	typedef std::weak_ptr<dom> dom_weak_ptr;

	class selector
	{
	public:
		selector(const std::string&);
		selector(std::string&&);

		selector(const char* s)
			: selector(std::string(s))
		{}

		template<int N>
		selector(const char s[N])
			: selector(std::string(s))
		{};

		friend class dom;

	protected:
		struct selector_matcher{
			bool operator()(const dom&) const;

		private:
			std::string matching_tag_name;
			std::string matching_id;
			std::string matching_class;
			std::string matching_name;
			std::string matching_index;

			friend class selector;
		};
		typedef std::vector<selector_matcher>::const_iterator selector_matcher_iterator;

		selector_matcher_iterator begin() const {
			return m_matchers.begin();
		}

		selector_matcher_iterator end() const {
			return m_matchers.end();
		}

	private:
		void build_matchers();

		std::vector<selector_matcher> m_matchers;

		std::string m_select_string;
	};

	class dom
	{
	public:

		// 默认构造.
		dom(dom* parent = nullptr) noexcept;

		// 从html构造 DOM.
		explicit dom(const std::string& html_page, dom* parent = nullptr);

		explicit dom(const dom& d);
		dom(dom&& d);
		dom& operator = (const dom& d);

	public:
		// 喂入一html片段.
		bool append_partial_html(const std::string &);

	public:
		dom operator[](const selector&);

		std::string to_html();

		std::string to_plain_text();

	private:

		void html_parser(boost::coroutines::asymmetric_coroutine<char>::pull_type & html_page_source);
		boost::coroutines::asymmetric_coroutine<char>::push_type html_parser_feeder;
		bool html_parser_feeder_inialized = false;

	protected:
		dom operator()(selector::selector_matcher_iterator first, selector::selector_matcher_iterator last);

	protected:
		std::map<std::string, std::string> attributes;
		std::string tag_name;

		std::vector<std::string> contents;

		std::vector<dom_ptr> children;
		dom* m_parent;

		template<class T>
		static void dom_walk(html::dom_ptr d, T handler);

		friend class selector;
	};
};
