# avhtml - html解析器

avhtml 是一个 c++ 写成的 HTML 页面解析器. 将 HTML 文本解析成 DOM 节点. 然后就可以使用 jquery 选择器来提取 HTML 页面上的信息.

比如提取标题, 可以用 "title" 选择器.

## 部分解析

avhtml 支持边输入 html 页面边解析. 也就是页面不需要完全下载, 下到哪解析到哪.

每次read下载到部分页面后, 可以用 dom::append_partial_html() 喂入这段 html 代码.

如果返回 false 表示页面有错误, 停止解析. true 则页面没有错误, 下次可以继续喂入解析

## 选择器

avhtml 支持 jquery 一样的选择器语法来选择 DOM 里的节点. 极大的方便了大家提取 html 页面里的信息.

要使用选择器, 使用 [] 下标即可.

如要返回页面的标题, 如下面的例子

```cpp
void test()
{
    html::dom page;

    page.append_partial_html("<html><head>");
    page.append_partial_html("<title>hello world</title");
    page.append_partial_html("></head></html>");

    assert(page["title"].to_plain_text() == "hello world");
}
```

使用 page\["title"\] 就可以返回了, 就好像 jquery 的选择符.
返回的结果还是 dom 类型. 可以使用 to_plain_text() 转为纯文本格式.

## 选择器语法支持列表

| Selector           | Example                | Desc                              | Support |
|--------------------|------------------------|-----------------------------------|---------|
| *                  | page["*"]              | all elements                      | √       |
| #id                | page["#id1"]           | id="id1"                          | √       |
| .class             | page[".class1"]        | class="class1"                    | √       |
| element            | page["div"]            | all <div> tags                    | √       |
| :first             | page["p:first"]        | first <p> tag                     |         |
| :last              | page["p:last"]         | last <p> tag                      |         |
| :eq                | page["p:eq(3)"]        | fourth <p> tag(index starts at 0) |         |
| :qt                | page["p:qt(3)"]        | list <p> tag with index > 3       |         |
| :lt                | page["p:lt(3)"]        | list <p> tag with index < 3       |         |
| :input             | page[":input"]         | all input tags                    |         |
| [attribute]        | page["[href]"]         | all tags with href attribute      | √       |
| [attribute=value]  | page["[href='#']"]     | all tags with empty link          |         |
| [attribute!=value] | page["[href!='#']"]    | all tags with not empty link      |         |
| [attribute$=value] | page["[href$='.jpg']"] | all tags with jpg link            |         |
