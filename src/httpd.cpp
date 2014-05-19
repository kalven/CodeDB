// CodeDB - public domain - 2010 Daniel Andersson

#include "httpd.hpp"

#include <iostream>

httpd::httpd(bas::io_service& iosvc, const std::string& host,
             const std::string& port, query_handler handler)
    : m_iosvc(iosvc), m_acceptor(iosvc), m_handler(handler) {
  bas::ip::tcp::resolver resolver(iosvc);
  bas::ip::tcp::resolver::query query(host, port);
  bas::ip::tcp::endpoint endpoint = *resolver.resolve(query);

  m_acceptor.open(endpoint.protocol());
  m_acceptor.set_option(bas::ip::tcp::acceptor::reuse_address(true));
  m_acceptor.bind(endpoint);
  m_acceptor.listen();

  async_accept();
}

void httpd::async_accept() {
  connection_ptr c(new connection(m_iosvc));
  m_acceptor.async_accept(c->m_socket, std::bind(&httpd::on_connect, this, c));
}

void httpd::on_connect(connection_ptr c) {
  async_read_request(c);
  async_accept();
}

void httpd::async_read_request(connection_ptr c) {
  bas::async_read_until(
      c->m_socket, c->m_inbuffer, "\x0d\x0a\x0d\x0a",
      std::bind(&httpd::on_request, this, c, std::placeholders::_1));
}

void httpd::on_request(connection_ptr c, const boost::system::error_code& e) {
  if (e) {
    std::cerr << "on_request error: " << e << std::endl;
    c->m_socket.close();
  } else {
    std::istream is(&c->m_inbuffer);

    http_request r;
    is >> r.m_method >> r.m_resource >> r.m_version;

    async_write_reply(c, m_handler(r));
  }
}

void httpd::async_write_reply(connection_ptr c,
                              std::vector<std::string>&& content) {
  c->m_outbuffer = std::move(content);

  std::vector<bas::const_buffer> buffers;
  buffers.reserve(c->m_outbuffer.size());

  for (auto i = c->m_outbuffer.begin(); i != c->m_outbuffer.end(); ++i)
    buffers.push_back(bas::buffer(*i));

  bas::async_write(c->m_socket, buffers,
                   std::bind(&httpd::on_write, this, c, std::placeholders::_1));
}

void httpd::on_write(connection_ptr c, const boost::system::error_code& e) {
  if (e) {
    std::cerr << "on_write error: " << e << std::endl;
    c->m_socket.close();
  }
}
