i want a simple blogging system with static html pages and main index page. write it in one C file.

the way to manage it is that it will have special folders:

/drafts/ for draft blog posts
/ready/ for ready to be published blog posts
/posts/ for published blog posts
When i put anythin anythin /ready/ it should automatically move to the /posts/ and add it to the index page. Respect subflders in /posts/ and /ready/ as categories.

Input files will be in .txt format. Change each new line to <br>. First line of the file is always a title.

Use template files:

header.template.html
footer.template.html
index.template.html
