# avhtml - html解析器

avhtml 是一个 c++ 写成的 HTML 页面解析器. 将 HTML 文本解析成 DOM 节点. 然后就可以使用 jquery 选择器来提取 HTML 页面上的信息.

比如提取标题, 可以用 "title" 选择器.

## 部分解析

avhtml 支持边输入 html 页面边解析. 也就是页面不需要完全下载, 下到哪解析到哪.

每次read下载到部分页面后, 可以用 dom::append_partial_html() 喂入这段 html 代码.

如果返回 false 表示页面有错误, 停止解析. true 则页面没有错误, 下次可以继续喂入解析

## 选择器

avhtml 支持 jquery 一样的选择器来选择 DOM 里的节点. 极大的方便了大家提取 html 页面里的信息.

要使用选择器, 使用 [] 下标即可.
