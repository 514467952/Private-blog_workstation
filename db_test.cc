#include"db.hpp"

/*测试db.hpp
 */

void TestBlogTable(){
  
  //方便我们进行格式化结果，打印出来的结果更好看一些
  Json::StyledWriter writer;

  MYSQL* mysql = blog_system::MySQLInit();
  blog_system::BlogTable blog_table(mysql);
  bool ret = false;

  Json::Value blog;
  /*
  blog["title"] = "第一篇博客";
  blog["content"] = "我要好好学习";
  blog["tag_id"] = 1;
  blog["create_time"] = "2019/-/-";
  bool ret = blog_table.Insert(blog);
  printf("Insert:%d\n",ret);
  */

  //测试查找
  /*Json::Value blogs;
  ret = blog_table.SelectAll(&blogs); 
  printf("select ALl %d\n",ret);
  printf("%s\n",writer.write(blogs).c_str());
  */

  //测试查找单个博客
  /*ret = blog_table.SelectOne(1,&blog);
  printf("select one %d\n",ret);
  printf("blog:%s\n",writer.write(blog).c_str());//序列化，方便我们看输出
  */

  //测试修改博客
  /* blog["blog_id"] = 1;
  blog["title"] = "第一篇项目博客";
  blog["content"] = "1.博客表\n 博客'就是总结与学习',帮助我们记事。";
  ret = blog_table.Update(blog);
  printf("Update %d\n",ret);
  */

  //测试删除博客
  ret = blog_table.Delete(1);
  printf("Delete %d\n",ret);
  blog_system::MySQLRelease(mysql);
}


void TestTagTable(){
   MYSQL* mysql = blog_system::MySQLInit(); 
  blog_system::TagTable tag_table(mysql);
  Json::Value tags;
  Json::StyledWriter writer;
  
  /*tag["tag_name"] = "项目";
  bool ret = tag_table.Insert(tag);
  printf("insert %d\n",ret);
  */
  
  //测试查找
 /* bool ret = tag_table.SelectAll(&tags);
  printf("select all %d\n",ret);
  printf("tags:%s\n",writer.write(tags).c_str());
  */

  //测试删除
  bool ret = tag_table.Delete(1);
  printf("Delete %d\n",ret);


  blog_system::MySQLRelease(mysql);
} 

int main()
{
  //TestBlogTable();
  TestTagTable();

  return 0;
}
