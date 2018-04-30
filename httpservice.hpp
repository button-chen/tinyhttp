#pragma once
#include <winsock2.h>
#include <microhttpd.h>
#include <map>
#include <string>
#include <sstream>
#include <type_traits>
#include <memory>
#include <functional>

class Response;
class Request;

class Response
{
public:
	Response(struct MHD_Connection* connection){
		m_connection = connection;
	}
	~Response() {};

public:
	void set_header(std::string k, std::string v){
		m_headers[k] = v;
	}

	void write(std::string res, int status=MHD_HTTP_OK){
		struct MHD_Response *response;
		response = MHD_create_response_from_buffer(res.size(), (void*)res.c_str(), MHD_RESPMEM_MUST_COPY);
		auto itr = m_headers.begin();
		for (itr; itr != m_headers.end(); itr++){
			MHD_add_response_header(response, itr->first.c_str(), itr->second.c_str());
		}
		MHD_queue_response (m_connection, status, response);
		MHD_destroy_response (response);
	}

private:
	std::map<std::string, std::string> m_headers;
	struct MHD_Connection* m_connection;
};

class Request
{
public:
	Request(struct MHD_Connection* connection){
		m_connection = connection;
	};
	~Request() {};

public:
	template<typename T>
	typename std::enable_if<!std::is_same<T, std::string>::value, T >::type 
		query(std::string key, T defval=T()){
		const char* p = MHD_lookup_connection_value(m_connection, MHD_GET_ARGUMENT_KIND, key.c_str());
		if (p == NULL){
			return defval;
		}
		std::istringstream iss(p);
		T r;
		iss >> r;
		return r;
	}

	template<typename T>
	typename std::enable_if<std::is_same<T, std::string>::value, std::string >::type
		query(std::string key, T defval=T()){
		const char* p = MHD_lookup_connection_value(m_connection, MHD_GET_ARGUMENT_KIND, key.c_str());
		if (p == NULL){
			return defval;
		}
		return std::string(p);
	}

	std::string body(){
		const char* p = MHD_lookup_connection_value(m_connection, MHD_POSTDATA_KIND, NULL);
		if (p == NULL){
			return std::string();
		}
		return std::string(p);
	}

private:
	struct MHD_Connection* m_connection;
};

class DefHttpRouter
{
public:
	typedef std::function<void(std::shared_ptr<Response>, std::shared_ptr<Request>)> HandleFunc;

	void GET(std::string path, HandleFunc handler){
		m_resource[path]["GET"] = handler;
	}

	void POST(std::string path, HandleFunc handler){
		m_resource[path]["POST"] = handler;
	}

	HandleFunc get_handler(std::string method, std::string path){
		HandleFunc func;
		if (m_resource.find(path) != m_resource.end()){
			if (m_resource[path].find(method) != m_resource[path].end()){
				func = m_resource[path][method];
			}
		}
		return func;
	}

private:
	//  path | method func
	std::map<std::string, std::map<std::string, HandleFunc > > m_resource;
};

template<typename Router>
class HttpService : public Router
{
public:
	HttpService(unsigned short port);
	~HttpService();

public:
	int run();

protected:
	// MHD »Øµ÷
	static int answer_to_connection (
		void *cls, 
		struct MHD_Connection *connection, 
		const char *url, 
		const char *method, 
		const char *version, 
		const char *upload_data, 
		size_t *upload_data_size, 
		void **con_cls );

private:
	struct MHD_Daemon* m_daemon;
	unsigned short m_port;
};

template<typename Router>
int HttpService<Router>::answer_to_connection( 
	void *cls, 
	struct MHD_Connection *connection, 
	const char *url, 
	const char *method, 
	const char *version, 
	const char *upload_data, 
	size_t *upload_data_size, 
	void **con_cls )
{
	DWORD begin = GetTickCount();
	std::cout << "[" << method << "] " << url << std::endl; 
	
	HttpService* host = reinterpret_cast<HttpService*>(cls);
	auto res = std::make_shared<Response>(connection);
	auto req = std::make_shared<Request>(connection);
	auto f = host->get_handler(method, url);
	if (f){
		f(res, req);
	}else{
		res->write("Not Found", MHD_HTTP_NOT_FOUND);
	}
	std::cout << "[" << method << "] " << url <<  " || cast time: " << GetTickCount() - begin << " ms" << std::endl;
	return MHD_YES;
}

template<typename Router>
HttpService<Router>::HttpService( unsigned short port )
{
	m_port = port;
}

template<typename Router>
HttpService<Router>::~HttpService()
{
	MHD_stop_daemon (m_daemon);
}

template<typename Router>
int HttpService<Router>::run()
{
	m_daemon = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD, m_port, NULL, NULL,
		&answer_to_connection, this, MHD_OPTION_END);
	if (NULL == m_daemon)
		return 1;
	return 0;
}

typedef HttpService<DefHttpRouter> TinyHTTP;
