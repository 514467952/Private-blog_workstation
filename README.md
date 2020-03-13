＃注意！！注意！！ 9090端口出现问题，现在地址为http://106.54.49.241:9095/
＃私人博客工作站
支持单人的简单博客平台
## 项目概述

       我们的客户端是在浏览器上实现的，要在浏览器上进行对博客的操作，编辑，修改，删除，增加等。

       服务器器端在Linux上实现，针对客户端的按钮都要有对应的事件处理。

       存储用MySQL数据库，调用c/c++API进行对博客的管理
##数据库

## 建表
一共建了两张表
* 博客表：
  * 博客的id，非空自增主键
  * 标题，varchar
  * 正文，用text字段
  * 标签，int
  * 创建时间，varchar
* 标签表
  * 标签id，非空自增主键
  * 便签名字
### 建表时需要注意的地方
 ##### 建表时
*	自增键必须放在主键的后面进行定义，否则建表时报错
		text类型是可变长度的字符串，最多65535个字符； 可以把字段类型改成MEDIUMTEXT（最多存放16777215个字符）或者LONGTEXT（最多存放4294967295个字符）
#### 封装数据库需要注意的操作
* mysql_init(NULL)  获取结果集合
* mysql_real_connect(connect_fd, "127.0.0.1", "root", "123456",    
              "blog_system", 3306, NULL, 0)  链接，等于NULL报错
* mysql_set_character_set(connect_fd, "utf8") 设置字符编码
* mysql_close(mysql) 关闭数据库操作句柄
* mysql_real_escape_string(mysql_, content_escape.get(), content.c_str(), content.size())，博客中会有一些特殊字符，需要对其进行转义
* mysql_query(mysql_, sql.get()) 执行sql语句
* mysql_error(mysql_))  数据库报错
* mysql_store_result(mysql_) 获取结果集合
* mysql_num_rows(result)获取结果集合中一共有多少行
* mysql_fetch_row(result) 获取每一行的内容
  ###JSON的介绍

<https://www.cnblogs.com/DswCnblog/p/6678708.html>

#### JSONCPP库的使用

* **Json::Value** ：可以表示所有支持的类型，如：int , double ,string , object, array等。其包含节点的类型判断(isNull,isBool,isInt,isArray,isMember,isValidIndex等),类型获取(type),类型转换(asInt,asString等),节点获取(get,[]),节点比较(重载<,<=,>,>=,==,!=),节点操作(compare,swap,removeMember,removeindex,append等)等函数。
* **Json::Reader**：将文件流或字符串创解析到Json::Value中，主要使用parse函数。Json::Reader的构造函数还允许用户使用特性Features来自定义Json的严格等级。
* **Json::Writer** ：与JsonReader相反，将Json::Value转换成字符串流等，Writer类是一个纯虚类，并不能直接使用。在此我们使用 Json::Writer 的子类：Json::FastWriter(将数据写入一行,没有格式),**Json::StyledWriter** (按json格式化输出,易于阅读)
  * Json::StyledWriter writer//定义对象
  * printf("tags:%s\n",writer.write(tags).c_str());  write对象中调用write方法写(JSON::Value的数据格式的对象).转化为char*类型打印。

##### JSON库使用时需要注意的地方

* 对不存在的键获取值会返回此类型的默认值。
* 通过key获取value时,要先判断value的类型,使用错误的类型获取value会导致程序中断。


* .append函数功能是将Json::Value添加到数组末尾

#### 封装服务器应该注意的地方

我们用一个`http`服务器作为底层，但是c++中并没有先成的http服务器，所以我在GitHub上找到一个牛人写的`http`服务器，拿来直接用，节省本项目开发的时间

<https://github.com/yhirose/cpp-httplib.git>

1. 首先我们先要让我们的服务器和数据库连通，所以我们先要进行数据库客户端的初始化和释放
2. 第二步我们就要设置一些自定义的路由(此处的路由并不是IP里的路由，而是指的是http中对应的方法+path对应到相应的处理函数上)，要实现新增博客，查看博客等在数据库里所对应的相应的功能
3. 第三步，我们要设置静态文件目录，把我们将来写好的网页放到这个目录里

####  和数据库建立链接

在这一步中，连接很简单，调用我们封装好的数据库API就行，但是断开连接就比较麻烦，因为整个服务器一但启动，他会进入到监听状态，所以必须等服务器关闭时，我们才能断开连接，而我们一般在Linux下关闭服务器都是使用ctrl+c来进行关闭。ctrl+c是一个信号，所以我们可以当触发这个信号时，就进行断开连接操作。

```c++
  using namespace httplib;      
  using namespace blog_system;      
  //1. 先和数据库建立好链接      
  mysql = blog_system::MySQLInit();      
  signal(SIGINT,[](int){blog_system::MySQLRelease(mysql);exit(0);});
  //2.创建相关数据库处理对象                           
  BlogTable blog_table(mysql);                         
  TagTable tag_table(mysql);

```

我们在进行信号处理时，第二个参数，是一个回调函数，当第一个参数触发时，执行后面回调函数里的内容，但是要写一个函数太麻烦，这里使用c++11中提供的lambda表达式。这样可以使代码更简洁，更容易进行维护。

#### cpp-httplib库的使用

* 创建服务器对象，Server srv
* 因为httplib底层是http服务器，他把相应的请求封装成对应的函数，我们只要用srv调用相应的函数就行了，
* 比如Get请求，第一参数是一个标签，第二参数是一个回调函数，表示你要对于这个请求做什么事情，在这里用lambda表达式，捕捉到博客表这个对象，传进去JSON格式的对象，把http服务器的格式转换为JSON格式进行检验，如果出错，判断后返回，如果没错，直接调用数据保存相应的数据，然后封装正确的结果响应，返回个200.即可

### 服务器主要流程

1. 首先我们肯定要接受http的请求消息，这里就是一个Post请求
2. 我们要对这个消息进行校验，但是http的格式并不好让我们进行校验，所以我们要先把http的格式转化为json格式，方便我们进行校验(***JSON中Reader中有一个parse接口可以把接收到的对象数据变成JSON::Value的数据格式**)
3. 校验完后，我们就可以调用我们封装好的mysql的API对数据进行操作
4. 最后我们还要封装正确的返回结果，给用户友好的提示
5. 这里我们并不使用异常处理，我们使用错误判定，这样更好，虽然代码比较乱，但是可以很容易理解。而且c++中异常处理比较差，功能比较有限
6. **不光是这个接口，我们每个博客接口都必须捕捉到blog_table这个对象**

```c++
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
```

#### 查看/删除博客需要注意的地方

查看blog_id，如果这里只写blog_id，这个httplib的库并不能识别，我又仔细看了
这个库的文档，他用了正则表达式，所以我又学习了一些正则表达式的内容
我们可以用\d+ 表示匹配一个数字，但是这里又可能会引发c++中的转义字符，所以我们需要c++11中提供的R()使转义字符不生效 

```c++
server.Get(R"(/blog/(\d+))", &blog_table {
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

```

#### 获取静态页面

 server.set_base_dir("./wwwroot");
