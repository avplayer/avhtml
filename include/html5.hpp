
#pragma once

#include <memory>
#include <functional>

#include <string>
#include <vector>

#include <boost/coroutine/asymmetric_coroutine.hpp>

namespace html{

	class dom;
	typedef std::shared_ptr<dom> dom_ptr;
	typedef std::weak_ptr<dom> dom_weak_ptr;

	class dom
	{

	public:

		// 默认构造.
		dom() noexcept;

		// 从html构造 DOM.
		explicit dom(const std::string& html_page);

	public:
		// 喂入一html片段.
		bool append_partial_html(const std::string &);

	public:
		dom operator[](const std::string& selector);

		std::string to_html();

		std::string to_plain_text();

	private:

		void html_parser(boost::coroutines::asymmetric_coroutine<char>::pull_type & html_page_source);
		boost::coroutines::asymmetric_coroutine<char>::push_type html_parser_feeder;

	protected:
		std::map<std::string, std::string> attributes;
		std::string tag_name;

		std::vector<dom_ptr> children;
		dom_weak_ptr parent;
	};
};
