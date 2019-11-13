
/*
 * 封装mysqlAPI来封装数据库操作系统
 */
#include<cstdio>
#include<cstdlib>
#include<mysql/mysql.h>
#include<memory>
#include<jsoncpp/json/json.h>

namespace blog_system{
  static MYSQL* MySQLInit(){
    //1.初始化一个Mysql句柄建立连接
    MYSQL* connect_fd = mysql_init(NULL);

    //2.和数据库建立连接
    if (mysql_real_connect(connect_fd, "127.0.0.1", "root", "123456",
          "blog_system", 3306, NULL, 0) == NULL) {
      printf("连接失败! %s\n", mysql_error(connect_fd));
      return NULL;
    }
    printf("连接成功\n");
    //3.设置字符编码集
    mysql_set_character_set(connect_fd, "utf8");
    return connect_fd;
  }    

  static void MySQLRelease(MYSQL* mysql) {
    //释放句柄并断开连接
    mysql_close(mysql);

  }

  class BlogTable {
    public:
      BlogTable(MYSQL* mysql) : mysql_(mysql) {
        //通过这个构造函数获取一个数据库的操作句柄
      }

      bool Insert(const Json::Value& blog) {
             // 由于博客内容中可能包含一些特殊字符(\n, '', "" 等), 会导致拼装出的 sql 语句有问题.
             // 应该使用 mysql_real_escape_string 对 content 字段来进行转义
             // 转义只是为了让 SQL 语句拼接正确. 实际上插入成功后数据库的内容已经自动转义回来了.
             const std::string& content = blog["content"].asString();

             // 文档上要求转义的缓冲区长度必须是之前的 2 倍 + 1
             // 使用 unique_ptr 管理内存
             std::unique_ptr<char> content_escape(new char[content.size() * 2 + 1]);
             mysql_real_escape_string(mysql_, content_escape.get(), content.c_str(), content.size());

             // 插入的博客内容可能较长, 需要搞个大点的缓冲区(根据用户请求的长度自适应),
             std::unique_ptr<char> sql(new char[content.size() * 2 + 4096]);

             sprintf(sql.get(), "insert into blog_table values(null, '%s', '%s', %d,'%s')",
             blog["title"].asCString(), content_escape.get(),
             blog["tag_id"].asInt(), blog["create_time"].asCString());

             int ret = mysql_query(mysql_, sql.get());
             if (ret != 0) {
                printf("执行 sql 失败! sql=%s, %s\n", sql.get(), mysql_error(mysql_));
                return false;
             }
            printf("执行插入成功\n");
            return true;
            
      }

      //因为是获取所有博客内容，我们需要一个返回的结果，
      //blogs作为输出型参数,除此之外我们还可以按照标签来筛选
      bool SelectAll(Json::Value* blogs, const std::string& tag_id = "") {
          //查找不需要太长的sql，固定长度就行了
          char sql[1024 * 4] = {0};
            // 可以根据 tag_id 来筛选结果
          if (tag_id == "") {
              //此时不需要按照标签来进行筛选
             sprintf(sql, "select blog_id, title, tag_id, create_time from blog_table");
          }
          else {
              //此时我们就要按标签进行筛选了
            sprintf(sql, "select blog_id, title, tag_id, create_time from blog_table where tag_id = '%s'", tag_id.c_str());
          }

          int ret = mysql_query(mysql_, sql);
          if (ret != 0) {
            printf("执行 查找 失败! %s\n", mysql_error(mysql_));
            return false;

          }

          MYSQL_RES* result = mysql_store_result(mysql_);
          if (result == NULL) {
            printf("获取结果失败! %s\n", mysql_error(mysql_));
            return false;

          }
          int rows = mysql_num_rows(result);
          //把结果集合写到blogs参数中，返回给调用者
          for (int i = 0; i < rows; ++i) {
            MYSQL_ROW row = mysql_fetch_row(result);
            Json::Value blog;
            //row[]中的下标和上面的select语句中写的列顺序是相关的
            blog["blog_id"] = atoi(row[0]);
            blog["title"] = row[1];
            blog["tag_id"] = atoi(row[2]);
            blog["create_time"] = row[3]; 
          // 遍历结果依次加入到 dishes 中
           blogs->append(blog);
          }
          printf("执行查找成功!共找到%d条博客\n",rows);
          //mysql查询的结果集合需要记得释放
          mysql_free_result(result);
          return true;
      }

      //查看指定博客，
      //blog同样是输出型参数，根据当前的blog_id在数据库中找到具体的内容
      //博客内容通过blog参数返回给调用者
      bool SelectOne(int32_t blog_id, Json::Value* blog) {
          char sql[1024 * 4] = {0};
          sprintf(sql, "select * from blog_table where blog_id = %d", blog_id);
          int ret = mysql_query(mysql_, sql);
          if (ret != 0) {
            printf("执行 sql 失败! %s\n", mysql_error(mysql_));
            return false;
          }

          MYSQL_RES* result = mysql_store_result(mysql_);
          if (result == NULL) {
            printf("获取结果失败! %s\n", mysql_error(mysql_));
            return false;

          }

          int rows = mysql_num_rows(result);
          if (rows != 1) {
            printf("查找结果不为 1 条. rows = %d!\n", rows);
            return false;
          }

          MYSQL_ROW row = mysql_fetch_row(result);
          (*blog)["blog_id"] = atoi(row[0]);
          (*blog)["title"] = row[1];
          (*blog)["content"] = row[2];
          (*blog)["tag_id"] = atoi(row[3]);
          (*blog)["create_time"] = row[4];
          return true;
      }
      bool Update(const Json::Value& blog) {
          //此处还需要转义，
          const std::string& content = blog["content"].asString();

          std::unique_ptr<char> content_escape(new char[content.size() * 2 + 1]);
          mysql_real_escape_string(mysql_, content_escape.get(), content.c_str(), content.size());
          
          //插入的博客内容会有点长，我们可以尽量把缓冲区变大点
          std::unique_ptr<char> sql(new char[content.size() * 2 + 4096]);

          sprintf(sql.get(), "update blog_table set title='%s', content='%s',tag_id=%d where blog_id=%d",
              blog["title"].asCString(),
              content_escape.get(),
              blog["tag_id"].asInt(),
              blog["blog_id"].asInt());

          int ret = mysql_query(mysql_, sql.get());
          if (ret != 0) {
            printf("执行更新博客 失败! sql=%s, %s\n", sql.get(), mysql_error(mysql_));
            return false;

          }
        printf("执行博客成功\n");
        return true;
      }
      bool Delete(int blog_id) {
         char sql[1024 * 4] = {0};
          sprintf(sql, "delete from blog_table where blog_id=%d", blog_id);
            int ret = mysql_query(mysql_, sql);
            if (ret != 0) {
                printf("执行删除博客失败! sql=%s, %s\n", sql, mysql_error(mysql_));
                return false;
                        
            }
          printf("删除博客成功\n");
          return true;
      }
    private:
      MYSQL* mysql_;

  };
  class TagTable {
    public:
    TagTable(MYSQL* mysql) : mysql_(mysql) {  }

    bool SelectAll( Json::Value* tags) {

          char sql[1024 * 4] = {0};
          sprintf(sql, "select * from tag_table");
          int ret = mysql_query(mysql_, sql);
          if (ret != 0) {
            printf("查找失败! %s\n", mysql_error(mysql_));
            return false;

          }
          MYSQL_RES* result = mysql_store_result(mysql_);
          if (result == NULL) {
            printf("获取结果失败! %s\n", mysql_error(mysql_));
            return false;

          }
          int rows = mysql_num_rows(result);
          for (int i = 0; i < rows; ++i) {
            MYSQL_ROW row = mysql_fetch_row(result);
            Json::Value tag;

            tag["tag_id"] = atoi(row[0]);
            tag["tag_name"] = row[1];
            tags->append(tag);
          }
          printf("查找标签成功!共找到 %d 个\n",rows);
          return true;
      }
    bool Insert(const Json::Value& tag) {

        char sql[1024 * 4] = {0};
        // 此处 dish_ids 需要先转成字符串(本来是一个对象,
        // 形如 [1, 2, 3]. 如果不转, 是无法 asCString)
        sprintf(sql,"insert into tag_table values(null, '%s')",tag["tag_name"].asCString());
        int ret = mysql_query(mysql_, sql);
        if (ret != 0) {
           printf("插入标签失败! sql=%s, %s\n", sql, mysql_error(mysql_));
          return false;
         }
        printf("插入标签成功\n");
        return true;
      }

  
    bool Delete(int32_t  tag_id) {
      char sql[1024 * 4] = {0};    
      sprintf(sql, "delete from tag_table where tag_id = %d", tag_id);    
      int ret = mysql_query(mysql_, sql);    
      if (ret != 0) {    
           printf("插入标签失败! sql=%s, %s\n", sql, mysql_error(mysql_));    
          return false;    
      }    
       printf("插入标签成功\n");    
       return true;  
    }

  private:
    MYSQL* mysql_;

};
}//end blog_system
