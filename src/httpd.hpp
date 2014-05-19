// CodeDB - public domain - 2010 Daniel Andersson

#ifndef CODEDB_HTTPD_HPP
#define CODEDB_HTTPD_HPP

#include "nsalias.hpp"

#include <boost/asio.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

struct http_request {
  std::string m_method;
  std::string m_resource;
  std::string m_version;
};

class httpd {
 public:
  typedef std::function<std::vector<std::string>(const http_request&)>
      query_handler;

  httpd(bas::io_service& iosvc, const std::string& host,
        const std::string& port, query_handler handler);

 private:
  struct connection {
    connection(bas::io_service& iosvc) : m_socket(iosvc), m_inbuffer(4096) {}

    bas::ip::tcp::socket m_socket;
    bas::streambuf m_inbuffer;
    std::vector<std::string> m_outbuffer;
  };

  typedef std::shared_ptr<connection> connection_ptr;

  void async_accept();
  void on_connect(connection_ptr c);

  void async_read_request(connection_ptr c);
  void on_request(connection_ptr c, const boost::system::error_code& e);

  void async_write_reply(connection_ptr c, std::vector<std::string>&& content);
  void on_write(connection_ptr c, const boost::system::error_code& e);

  bas::io_service& m_iosvc;
  bas::ip::tcp::acceptor m_acceptor;
  query_handler m_handler;
};

#endif
