#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

using boost::asio::ip::tcp;
class Modbus
{
  std::vector<int> inputs;

  public:
  std::string process(char* input, size_t length);
};

const int max_length = 1024;

Modbus * s_modbus;

typedef boost::shared_ptr<tcp::socket> socket_ptr;

void session(socket_ptr sock)
{
  try
  {
    for (;;)
    {
      //TODO get length from protocol, then no need to initialize
      char data[max_length];
      for (int i =0; i<50; i++)
      {
        data[i] = 0;
      }

      boost::system::error_code error;
      size_t length = sock->read_some(boost::asio::buffer(data), error);

      s_modbus->process(data, length);
      
      if (error == boost::asio::error::eof)
        break; // Connection closed cleanly by peer.
      else if (error)
        throw boost::system::system_error(error); // Some other error.

      boost::asio::write(*sock, boost::asio::buffer(data, length));
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception in thread: " << e.what() << "\n";
  }
}

void server(boost::asio::io_service& io_service, short port)
{
  tcp::acceptor a(io_service, tcp::endpoint(tcp::v4(), port));
  for (;;)
  {
    socket_ptr sock(new tcp::socket(io_service));
    a.accept(*sock);
    boost::thread t(boost::bind(session, sock));
  }
}

int main(int argc, char* argv[])
{
  Modbus modbus;
  s_modbus = &modbus;

  try
  {
    boost::asio::io_service io_service;

    server(io_service, std::atoi("9001"));
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

std::string Modbus::process(char* input, size_t length)
{
  for (int i =0; i<15; i++)
  {
    std::cout << i <<"data:" << (int)input[i] << std::endl;
  }
  return "a";
}