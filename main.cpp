#include "httpservice.hpp"
#include <iostream>

using namespace std;


void handle_test1(std::shared_ptr<Response> res, std::shared_ptr<Request> req){
	res->write(req->body());
}

class CLASSA {
public:
	CLASSA(){};

	void handle_test2(std::shared_ptr<Response> res, std::shared_ptr<Request> req){
		//cout << "收到参数: name" << req->query<std::string>("name") << ", age:" << req->query<int>("age") << endl; 
		res->set_header("Content-Type", "application/json");
		res->write("{\"name\":\"button\", \"age\":100}");
	}
};

int main()
{
	TinyHTTP http(8888);
	// 可以用自由函数注册
	http.POST("/chen/test1", handle_test1);
	// 可以用成员函数注册
	CLASSA testclass;
	http.GET("/chen/test2", std::bind(&CLASSA::handle_test2, &testclass, std::placeholders::_1, std::placeholders::_2));
	// 可以用lambda注册
	http.GET("/chen/test3", [](std::shared_ptr<Response> res, std::shared_ptr<Request> req){
		res->write("handle test3 url request");
	});

	http.run();

	(void) getchar ();
	return 0;
}

