#include"httplib.h"
#include"db.hpp"
#include<signal.h>



MYSQL* mysql = NULL;

int main()
{
  using namespace httplib;
  using namespace blog_system;
  //1. 先和数据库建立好链接
  mysql = blog_system::MySQLInit();
  signal(SIGINT,[](int){blog_system::MySQLRelease(mysql);exit(0);});
  //2.创建相关数据库处理对象
  BlogTable blog_table(mysql);
  TagTable tag_table(mysql);
  //3,创建服务器器
  Server server;


  //4.设置"路由"
    // 新增博客
   server.Post("/blog", [&blog_table](const Request& req, Response& resp) {
        printf("新增博客!\n");
         //创建一些我们需要用到的对象
        Json::FastWriter writer;  //写对象
        Json::Reader reader;  //读取对象
        Json::Value req_json; //请求对象
        Json::Value resp_json;  //响应对象
        //1.获取到请求中的body并解析成json
        //parse函数中第一个参数是需要解析的字符串
        //第二个参数是把解析后的字符串放到哪个对象中
        //第三个参数是注释，默认是true，所以我就不管了
        bool ret = reader.parse(req.body,req_json);
        if(!ret){
            //解析出错，给用户提示
            printf("解析请求失败! %s\n",req.body.c_str());
            //构造一个相应对象，告诉客户端出错了
            resp_json["ok"] = false;
            resp_json["reason"] = "提交的数据格式有误!\n";
            resp.status = 400;
            //返回给客户端，第二个参数设置反回的数据类型
            resp.set_content(writer.write(resp_json), "application/json");
            return;
        }

        //2. 进行参数校验
        //这些字段如果缺任何一个，都代表有错
        if (req_json["title"].empty() || req_json["content"].empty()
            || req_json["tag_id"].empty() || req_json["create_time"].empty()) {
          resp_json["ok"] = false;
          resp_json["reason"] = "博客的格式出错!\n";
          resp.status = 400;
          resp.set_content(writer.write(resp_json), "application/json");
          return; 
        }

        //3.调用数据库接口进行操作
        ret = blog_table.Insert(req_json);
        if (!ret) {
          printf("插入博客失败!\n");
          resp_json["ok"] = false;
          resp_json["reason"] = "插入失败！\n";
          resp.status = 500;
          resp.set_content(writer.write(resp_json), "application/json");
          return;
        }

        //4.封装正确的返回结果
         resp_json["ok"] = true;
         resp.set_content(writer.write(resp_json), "application/json");
         return;

   });
   // 查看所有博客(可以按标签筛选)
   server.Get("/blog", [&blog_table](const Request& req, Response& resp) {
      printf("查看所有博客!\n");
      Json::Reader reader;
      Json::FastWriter writer;
      Json::Value resp_json;
      //如果没传tag_id,返回的是空字符串 
      const std::string& tag_id = req.get_param_value("tag_id");
      // 对于查看博客来说 API 没有请求参数, 不需要解析参数和校验了, 直接构造结果即可
      //  1. 调用数据库接口查询数据
      Json::Value blogs;
      bool ret = blog_table.SelectAll(&blogs, tag_id);
      if (!ret) {
        resp_json["ok"] = false;
        resp_json["reason"] = "查看博客失败\n";
        resp.status = 500;
        resp.set_content(writer.write(resp_json), "application/json");
        return;
      }
      
      // 2. 构造响应结果
      resp.set_content(writer.write(blogs), "application/json");
      return;
   });

   //查看一篇博客
   server.Get(R"(/blog/(\d+))", [&blog_table](const Request& req, Response& resp) {
       //1.解析获取到blog_id
      int32_t blog_id = std::stoi(req.matches[1].str());
      printf("查看id为  %d  的博客!\n",blog_id);
      Json::Value resp_json;
      Json::FastWriter writer;
      //2.直接调用数据库操作
      bool ret = blog_table.SelectOne(blog_id, &resp_json);
      if (!ret) {
        resp_json["ok"] = false;
        resp_json["reason"] = "查看指定博客失败!\n";
        resp.status = 404;
        resp.set_content(writer.write(resp_json), "application/json");
        return;
      }
      
      //3.包装正确的响应
      resp_json["ok"] = true;
      resp.set_content(writer.write(resp_json), "application/json");
      return;
   });

   //删除博客
   server.Delete(R"(/blog/(\d+))", [&blog_table](const Request& req, Response& resp) {
        Json::Value resp_json;
        Json::FastWriter writer;
        // 1. 解析获取 blog_id
        //使用 matches[1] 就能获取到 blog_id
        int32_t blog_id = std::stoi(req.matches[1].str());
        printf("删除 id 为%d 的博客!\n",blog_id);

        // 2. 调用数据库接口删除博客
        bool ret = blog_table.Delete(blog_id);
        if (!ret) {
          resp_json["ok"] = false;
          resp_json["reason"] = "删除博客失败!\n";
          resp.status = 500;
          resp.set_content(writer.write(resp_json), "application/json");
          return;
        }
      
        //3.包装正确的响应
        resp_json["ok"] = true;
        resp.set_content(writer.write(resp_json), "application/json");
        return;
   });
   // 修改博客
   server.Put(R"(/blog/(\d+))", [&blog_table](const Request& req, Response& resp) {
        Json::Reader reader;
        Json::FastWriter writer;
        Json::Value req_json;
        Json::Value resp_json;
        // 1. 获取到博客 id
        int32_t blog_id = std::stoi(req.matches[1].str());
        printf("修改 id为 %d的博客!\n",blog_id);
        
        // 2. 解析博客信息
        bool ret = reader.parse(req.body, req_json);
        if (!ret) {
            resp_json["ok"] = false;
            resp_json["reason"] = "解析博客失败!\n";
            resp.status = 400;
            resp.set_content(writer.write(resp_json), "application/json");
            return ;
        }

        //一定要记得补充上 blog_id 
        req_json["blog_id"] = blog_id;  //从path中得到的id设置到json对象中
        // 3. 校验博客信息
        if (req_json["title"].empty() || req_json["content"].empty()
            || req_json["tag_id"].empty()) {
          // 请求解析出错, 返回一个400响应
           resp_json["ok"] = false;
           resp_json["reason"] = "更新博客格式错误\n";
           resp.status = 400;
           resp.set_content(writer.write(resp_json), "application/json");
           return;
        }

        // 4. 调用数据库接口进行修改
        ret = blog_table.Update(req_json);
        if (!ret) {
          resp_json["ok"] = false;
          resp_json["reason"] = "更新失败!\n";
          resp.status = 500;
          resp.set_content(writer.write(resp_json), "application/json");
          return;
        }

        // 5. 封装正确的数据
        resp_json["ok"] = true;
        resp.set_content(writer.write(resp_json), "application/json");
        return;
   });
   // 新增标签
   server.Post("/tag", [&tag_table](const Request& req, Response& resp) {
        Json::Reader reader;
        Json::FastWriter writer;
        Json::Value req_json;
        Json::Value resp_json;
        printf("新增标签!\n");
        // 1. 请求解析成 Json 格式
        bool ret = reader.parse(req.body, req_json);
        if (!ret) {
          resp_json["ok"] = false;
          resp_json["reason"] = "解析失败\n";
          resp.status = 400;
           resp.set_content(writer.write(resp_json), "application/json");
          return ;
        }
        // 2. 校验标签格式
        if (req_json["tag_name"].empty()) {
         resp_json["ok"] = false;
         resp_json["reason"] = "标签格式有误!\n";
         resp.status = 400;
         resp.set_content(writer.write(resp_json), "application/json");
         return;
        }
        
        // 3. 调用数据库接口, 插入标签
        ret = tag_table.Insert(req_json);
        if (!ret) {
          resp_json["ok"] = false;
          resp_json["reason"] = "插入标签失败！\n";
          resp.status = 500;
          resp.set_content(writer.write(resp_json), "application/json");
          return;
        }
        // 4. 返回正确的结果
        resp_json["ok"] = true;
        resp.set_content(writer.write(resp_json), "application/json");
   });
   //删除标签
   server.Delete(R"(/tag/(\d+))", [&tag_table](const Request& req, Response& resp) {
          Json::Value resp_json;
          Json::FastWriter writer;
          // 1. 解析出 tag_id
          int tag_id = std::stoi(req.matches[1].str());
          printf("要删除的标签id 为 %d\n",tag_id);
          // 2. 执行数据库操作删除标签
          bool ret = tag_table.Delete(tag_id);
          if (!ret) {
            resp_json["ok"] = false;
            resp_json["reason"] = "删除所有标签失败\n";
            resp.status = 500;
            resp.set_content(writer.write(resp_json), "application/json");
            return;
          }
          // 3. 包装正确的结果
          resp_json["ok"] = true;
          resp.set_content(writer.write(resp_json), "application/json");
          return;
       });
   // 获取所有标签
    server.Get("/tag", [&tag_table](const Request& req, Response& resp) {
        printf("获取所有标签\n");
        Json::Reader reader;
        Json::FastWriter writer;
        Json::Value resp_json;
        // 1. 调用数据库接口查询数据
        Json::Value tags;
        bool ret = tag_table.SelectAll(&tags);
        if (!ret) {
          resp_json["ok"] = false;
          resp_json["reason"] = "查找所有标签失败\n";
          resp.status = 500;
          resp.set_content(writer.write(resp_json), "application/json");
          return;
        }
        // 2. 构造响应结果
        resp.set_content(writer.write(tags), "application/json");
        return;
    });

 //设置静态文件目录 
  server.set_base_dir("./wwwroot");
  server.listen("0.0.0.0",9093);
  return 0; 
}
