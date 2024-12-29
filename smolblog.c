// smolblog.c
// A simple blog generator written in C.
// This program watches a directory for new text files, processes them into HTML files, and updates the index page.
// It also generates an RSS feed based on the latest posts.
// Designed by Krzysztof Krystian Jankowski

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define MAX_PATH 1024
#define MAX_LINE 4096
#define MAX_POSTS 1000
#define RSS_ITEMS_LIMIT 20
#define WEBLOG_URL "https://smol.p1x.in/weblog/"  // Add base URL constant

void read_template(const char *filename, char *buffer) {
    FILE *fp = fopen(filename, "r");
    if (!fp) exit(1);
    size_t bytes = fread(buffer, 1, MAX_LINE, fp);
    buffer[bytes] = '\0';
    fclose(fp);
}

void copy_file(const char *src_path, const char *dst_path) {
    FILE *src = fopen(src_path, "rb");  // Changed to binary read
    FILE *dst = fopen(dst_path, "wb");  // Changed to binary write
    if (!src || !dst) return;

    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes, dst);
    }
    fclose(src);
    fclose(dst);
}

void format_date(char *date_str, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(date_str, size, "%Y-%m-%d %H:%M", t);
}

void process_file(const char *src_path, const char *dst_path, const char *category) {
    char html_path[MAX_PATH], txt_path[MAX_PATH];
    strcpy(html_path, dst_path);
    strcpy(txt_path, dst_path);
    txt_path[strlen(txt_path) - 5] = '\0';
    strcat(txt_path, ".txt");
    
    copy_file(src_path, txt_path);
    
    char line[MAX_LINE], title[MAX_LINE] = "", header[MAX_LINE] = "";
    char footer[MAX_LINE] = "", content[MAX_LINE * 10] = "";
    
    read_template("templates/post.header.template.html", header);
    read_template("templates/post.footer.template.html", footer);  // Changed to use post-specific footer
    
    FILE *src = fopen(src_path, "r");
    if (!src) return;
    
    if (fgets(line, sizeof(line), src)) {
        line[strcspn(line, "\n")] = 0;
        strcpy(title, line);
    }
    
    while (fgets(line, sizeof(line), src)) {
        line[strcspn(line, "\n")] = 0;
        strcat(content, line);
        strcat(content, "<br>\n");
    }
    fclose(src);
    
    FILE *dst = fopen(dst_path, "w");
    if (!dst) return;
    
    char date_str[64];
    format_date(date_str, sizeof(date_str));
    
    char nav[MAX_LINE] = "";
    read_template("templates/nav.template.html", nav);
    
    fprintf(dst, "%s", header);
    fprintf(dst, "<h1>%s</h1>\n", title);
    fprintf(dst, "<div class=\"date\">Published: %s</div>\n", date_str);
    if (category[0] != '\0') {
        fprintf(dst, "<p>Category: %s</p>\n", category);
    }
    fprintf(dst, "<div class=\"post\">\n%s</div>\n", content);
    fprintf(dst, "%s", nav);  // Add navigation before footer
    fprintf(dst, "%s", footer);
    fclose(dst);
    
    char date_file[MAX_PATH];
    strcpy(date_file, dst_path);
    date_file[strlen(date_file) - 5] = '\0';
    strcat(date_file, ".date");
    FILE *df = fopen(date_file, "w");
    if (df) {
        fprintf(df, "%s", date_str);
        fclose(df);
    }

    printf("Processed: %s\n", src_path);
}

void extract_title_from_txt(const char *filepath, char *title, size_t title_size) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) return;
    
    if (fgets(title, title_size, fp)) {
        title[strcspn(title, "\n")] = 0;
    }
    fclose(fp);
}

void copy_static_assets() {
    copy_file("templates/styles.css", "public_html/styles.css");
    copy_file("templates/favicon.png", "public_html/favicon.png");
}

void generate_rss_feed(const char *posts_list) {
    char rss_path[MAX_PATH] = "public_html/rss.xml";
    FILE *rss = fopen(rss_path, "w");
    if (!rss) return;

    char header_template[MAX_LINE] = "", item_template[MAX_LINE] = "", footer_template[MAX_LINE] = "";
    read_template("templates/rss.header.template.xml", header_template);
    read_template("templates/rss.item.template.xml", item_template);
    read_template("templates/rss.footer.template.xml", footer_template);

    time_t now = time(NULL);
    char date_str[64];
    strftime(date_str, sizeof(date_str), "%a, %d %b %Y %H:%M:%S %z", localtime(&now));

    fprintf(rss, header_template, date_str);

    char *post = strdup(posts_list);
    char *line = strtok(post, "\n");
    int items = 0;

    while (line && items < RSS_ITEMS_LIMIT) {
        char title[MAX_LINE] = "", link[MAX_LINE] = "", date[64] = "";
        char content[MAX_LINE * 10] = "";  // Added for post content
        
        // Extract title between first '>' and '</a>' without the HTML tags
        char *title_start = strchr(line, '>');
        char *title_end = strstr(line, "</a>");
        if (title_start && title_end) {
            // Skip any nested HTML tags in the title
            char *clean_title_start = title_start + 1;
            char *nested_tag = strstr(clean_title_start, "<");
            if (nested_tag && nested_tag < title_end) {
                clean_title_start = strchr(nested_tag, '>') + 1;
            }
            strncpy(title, clean_title_start, title_end - clean_title_start);
            title[title_end - clean_title_start] = '\0';
        }

        // Extract link and make it absolute
        char *link_start = strstr(line, "href=\"");
        char *link_end = NULL;
        if (link_start) {
            link_start += 6;
            link_end = strchr(link_start, '"');
            if (link_end) {
                char full_url[MAX_PATH];
                snprintf(full_url, sizeof(full_url), "%s%.*s", 
                        WEBLOG_URL, (int)(link_end - link_start), link_start);
                strncpy(link, full_url, sizeof(link) - 1);
                link[sizeof(link) - 1] = '\0';
            }
        }

        // Extract and read content from txt file
        char txt_path[MAX_PATH];
        if (link_start && link_end) {
            strncpy(txt_path, "public_html/", MAX_PATH);
            strncat(txt_path, link_start, link_end - link_start);
            txt_path[strlen(txt_path) - 5] = '\0';  // Remove .html
            strcat(txt_path, ".txt");
            
            FILE *txt_file = fopen(txt_path, "r");
            if (txt_file) {
                char line_buf[MAX_LINE];
                int first_line = 1;
                while (fgets(line_buf, sizeof(line_buf), txt_file)) {
                    if (first_line) {  // Skip the title (first line)
                        first_line = 0;
                        continue;
                    }
                    strcat(content, line_buf);
                }
                fclose(txt_file);
            }
        }

        // Extract and format date
        char *date_start = strstr(line, "class=\"date\">");
        if (date_start) {
            date_start += 13;
            char *date_end = strchr(date_start, '<');
            if (date_end) {
                strncpy(date, date_start, date_end - date_start);
                date[date_end - date_start] = '\0';
                
                // Convert date to RFC822 format
                struct tm tm = {0};
                char rfc822_date[64];
                sscanf(date, "%d-%d-%d %d:%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min);
                tm.tm_year -= 1900;
                tm.tm_mon -= 1;
                strftime(rfc822_date, sizeof(rfc822_date), "%a, %d %b %Y %H:%M:%S %z", &tm);
                
                if (strlen(title) > 0 && strlen(link) > 0) {
                    fprintf(rss, item_template, title, link, link, rfc822_date, content);
                    items++;
                }
            }
        }

        line = strtok(NULL, "\n");
    }

    free(post);
    fprintf(rss, "%s", footer_template);
    fclose(rss);
    printf("Generated RSS feed\n");
}

void update_index() {
    printf("Updating index file...\n");
    copy_static_assets();
    printf("Static assets copied\n");
    
    char header[MAX_LINE] = "", footer[MAX_LINE] = "";
    char index_template[MAX_LINE] = "", posts_list[MAX_LINE * 10] = "";
    
    read_template("templates/header.template.html", header);
    read_template("templates/footer.template.html", footer);
    read_template("templates/index.template.html", index_template);
    
    DIR *dir = opendir("public_html/posts");
    if (!dir) return;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char category_path[MAX_PATH];
            snprintf(category_path, sizeof(category_path), "public_html/posts/%s", entry->d_name);
            
            DIR *cat_dir = opendir(category_path);
            if (cat_dir) {
                struct dirent *post;
                while ((post = readdir(cat_dir)) != NULL) {
                    if (strstr(post->d_name, ".html")) {
                        char title[MAX_LINE] = "";
                        char post_path[MAX_PATH];
                        char txt_path[MAX_PATH];
                        
                        snprintf(post_path, sizeof(post_path), "%s/%s", category_path, post->d_name);
                        strncpy(txt_path, post_path, sizeof(txt_path));
                        txt_path[strlen(txt_path) - 5] = '\0';
                        strcat(txt_path, ".txt");
                        
                        extract_title_from_txt(txt_path, title, sizeof(title));
                        
                        char date_str[64] = "";
                        char date_file[MAX_PATH];
                        strncpy(date_file, post_path, sizeof(date_file));
                        date_file[strlen(date_file) - 5] = '\0';
                        strcat(date_file, ".date");
                        
                        FILE *df = fopen(date_file, "r");
                        if (df) {
                            fgets(date_str, sizeof(date_str), df);
                            fclose(df);
                        }
                        
                        char base_name[MAX_PATH];
                        strncpy(base_name, post->d_name, sizeof(base_name));
                        base_name[strlen(base_name) - 5] = '\0';
                        
                        char link[MAX_LINE];
                        snprintf(link, sizeof(link), 
                                "<p><a href=\"posts/%s/%s\">%s</a> [%s] "
                                "<a href=\"posts/%s/%s.txt\">[txt]</a> "
                                "<span class=\"date\">%s</span></p>\n",
                            entry->d_name, post->d_name, title, entry->d_name,
                            entry->d_name, base_name,
                            date_str);
                        strcat(posts_list, link);
                    }
                }
                closedir(cat_dir);
            }
        }
    }
    closedir(dir);
    
    FILE *index = fopen("public_html/index.html", "w");
    if (!index) {
        printf("Error: Could not create index.html\n");
        return;
    }
    
    fprintf(index, "%s", header);
    char *pos = strstr(index_template, "<!-- POSTS_LIST -->");
    if (pos) {
        *pos = '\0';
        fprintf(index, "%s%s%s", index_template, posts_list, pos + strlen("<!-- POSTS_LIST -->"));
    } else {
        fprintf(index, "%s", index_template);
    }
    fprintf(index, "%s", footer);
    fclose(index);
    
    printf("Index file updated\n");
    generate_rss_feed(posts_list);
}

void process_ready_folder() {
    DIR *dir = opendir("generate");
    if (!dir) return;

    char default_posts_path[MAX_PATH];
    snprintf(default_posts_path, sizeof(default_posts_path), "public_html/posts/general");
    mkdir(default_posts_path, 0755);
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && strstr(entry->d_name, ".txt")) {
            char src_path[MAX_PATH], dst_path[MAX_PATH], html_name[MAX_PATH];
            
            snprintf(src_path, sizeof(src_path), "generate/%s", entry->d_name);
            strcpy(html_name, entry->d_name);
            html_name[strlen(html_name) - 4] = '\0';
            strcat(html_name, ".html");
            snprintf(dst_path, sizeof(dst_path), "public_html/posts/general/%s", html_name);
            
            process_file(src_path, dst_path, "general");
            if (access(dst_path, F_OK) == 0) remove(src_path);
        }
        else if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char ready_path[MAX_PATH], posts_path[MAX_PATH];
            
            snprintf(ready_path, sizeof(ready_path), "generate/%s", entry->d_name);
            snprintf(posts_path, sizeof(posts_path), "public_html/posts/%s", entry->d_name);
            mkdir(posts_path, 0755);
            
            DIR *cat_dir = opendir(ready_path);
            if (!cat_dir) continue;
            
            struct dirent *post;
            while ((post = readdir(cat_dir)) != NULL) {
                if (strstr(post->d_name, ".txt")) {
                    char src_path[MAX_PATH], dst_path[MAX_PATH], html_name[MAX_PATH];
                    
                    strcpy(html_name, post->d_name);
                    html_name[strlen(html_name) - 4] = '\0';
                    strcat(html_name, ".html");
                    
                    snprintf(src_path, sizeof(src_path), "%s/%s", ready_path, post->d_name);
                    snprintf(dst_path, sizeof(dst_path), "%s/%s", posts_path, html_name);
                    
                    process_file(src_path, dst_path, entry->d_name);
                    if (access(dst_path, F_OK) == 0) remove(src_path);
                }
            }
            closedir(cat_dir);
        }
    }
    closedir(dir);
    update_index();
}

int main() {
    printf("Starting blog generator...\n");
    
    mkdir("working_dir", 0755);
    mkdir("generate", 0755);
    mkdir("public_html", 0755);
    mkdir("public_html/posts", 0755);
    
    // Copy static assets on startup too
    copy_static_assets();
    
    while (1) {
        process_ready_folder();
        printf(".");
        fflush(stdout);
        sleep(5);
    }
    
    return 0;
}
