//Windows build
// $ mkdir build
// $ cd build
// $ conan install .. -s build_type=Debug -s compiler="Visual Studio" -s compiler.runtime=MDd
// $ cmake .. -G "Visual Studio 16 2019"
// $ cmake --build . --config Debug

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
  int transactionID[2];
  int protocolID[2];
  int dataLength[2];
  int unitID;
  int functionCode;
  int startAdress[2];
  int quantity[2];

  transactionID[0] = (int)input[0];
  transactionID[1] = (int)input[1];
  protocolID[0] = (int)input[2];
  protocolID[1] = (int)input[3];
  dataLength[0] = (int)input[4];
  dataLength[1] = (int)input[5];
  unitID = (int)input[6];
  functionCode = (int)input[7];
  startAdress[0] = (int)input[8];
  startAdress[1] = (int)input[9];
  quantity[0] = (int)input[10];
  quantity[1] = (int)input[11];

  std::cout << length << std::endl;
  std::cout << "\n\n";
  for (int i =0; i<12; i++)
  {
    std::cout << (int)input[i] << " ";
  }

  std::cout << "=================================================================================================" << std::endl;

  return "resposta";

  for (int i =0; i<15; i++)
  {
    std::cout << i <<"data:" << (int)input[i] << std::endl;
  }
  return "a";
}