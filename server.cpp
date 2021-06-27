//Windows build
// $ mkdir build
// $ cd build
// $ conan install .. -s build_type=Debug -s compiler="Visual Studio" -s compiler.runtime=MDd
// $ cmake .. -G "Visual Studio 16 2019"
// $ cmake --build . --config Debug
// $ cd bin
// $ server.exe

//Linux build
// $ mkdir build
// $ cd build
// $ conan install .. --build=missing
// $ cmake ..
// $ cmake --build . --config Debug
// $ cd bin
// $ ./server

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

using boost::asio::ip::tcp;

union u_Int
{
  char _char[2];
  int _int;

  void read(char* data, int position) 
  {
    this->_char[1] = data[position]; 
    this->_char[0] = data[position+1];
  }

  void write(char* input, int position) 
  {
    input[position] = this->_char[0];
    input[position+1] = this->_char[1];
  }
};

const int max_length = 1024;

/*
 * All services implemented througth this TCP server should inherith from this interface.
 * Each time the tcp server receive a message, it calls it's service method "process" on a new thread.
 */ 
class Tcp_service_interface
{
  public:
    virtual auto process(char* input, const size_t inputLenght, char* response, size_t& responseLength) -> void = 0;
};

// TODO services must be threadsafe
class Modbus_service : public Tcp_service_interface
{
  std::map<int,int> m_inputs;

  auto querryHoldingRegisters(char* input, char* response, size_t& responseLength) ->void;

  public:
    virtual auto process(char* input, const size_t inputLength, char* response, size_t& responseLength) -> void ;
};

class Tcp_server
{
  boost::asio::io_service m_io_service; //Contain platform specific code to handle i/o context (socket readines, file descriptors...)
  std::shared_ptr<Tcp_service_interface> m_service;

  auto handle_session(std::shared_ptr<tcp::socket> sock) noexcept ->void;

  public:
    Tcp_server(std::shared_ptr<Tcp_service_interface> service, int port);
};

int main(int argc, char* argv[])
{
  const int port = 9001;
  try
  {
    std::shared_ptr<Tcp_service_interface> modbus_server = std::make_shared<Modbus_service>();

    std::cout << "Server starting on port " << port << std::endl;
    Tcp_server tcp_server(modbus_server, port);
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
    return 1;
  }

  return 0;
}

auto Modbus_service::process(char* input, const size_t inputLength, char* response, size_t& responseLength) ->void
{
  // char transactionID[2];
  // char protocolID[2];
  // char dataLength[2];
  // char unitID;
  // char functionCode;
  // char startAdress[2];
  // char quantity[2];

  // transactionID[0]  = input[0];
  // transactionID[1]  = input[1];
  // protocolID[0]     = input[2];
  // protocolID[1]     = input[3];
  // dataLength[0]     = input[4];
  // dataLength[1]     = input[5];
  // unitID            = input[6];
  // functionCode      = input[7];
  // startAdress[0]    = input[8];
  // startAdress[1]    = input[9];
  // quantity[0]       = input[10];
  // quantity[1]       = input[11];

  switch (input[7])
  {
    case 3:
      this->querryHoldingRegisters(input, response, responseLength);
    default:
      break;
  }

  // Sync transactionId
  response[0] = input[0];
  response[1] = input[1];

  // Protocol identifier (MODBUS)
  response[2] = 0;
  response[3] = 0;

  std::cout << "\n=================================================================================================\n";
  
  std::cout << "Received: ";
  for (int i =0; i<inputLength; i++)
  {
    std::cout  << (int)input[i] << " ";
  }

  std::cout << "\nResponse: ";
  for (int i =0; i<responseLength; i++)
  {
    std::cout << (int)response[i] << " ";
  }
  
  std::cout << "\n=================================================================================================\n";
}

auto Modbus_service::querryHoldingRegisters(char* input, char* response, size_t& responseLength) ->void
{
  u_Int numberOfQueries;
  numberOfQueries.read(input, 10); //Read byte 10 and 11 to get que quantity of queries;

  u_Int dataLength;
  dataLength._int = (numberOfQueries._int * 2) + 2;

  dataLength.write(response, 5);

  // Unit Identifier
  response[6] = 0;

  // Function Code
  response[7] = 3; //This member function treats only code 3

  dataLength._int = dataLength._int - 2;
  response[8] = dataLength._char[0];

  for (int i =0; i<=dataLength._int; i+=2)
  {
    // TODO Implement a value for each query
    // Using allways 256
    response[9+i] = 1;
    response[10+i] = 0;
  }

  responseLength = 9 + dataLength._int;
}

Tcp_server::Tcp_server(std::shared_ptr<Tcp_service_interface> service, int port)
  :m_service (service)
{
  tcp::acceptor acceptor(m_io_service, tcp::endpoint(tcp::v4(), port)); //When a message arives in the socket, the acceptor object check if the message follow TCP protocol, then it generate a session.

  while( true )
  {
    std::shared_ptr<tcp::socket> sock(new tcp::socket(m_io_service)); //Handle socket
    acceptor.accept(*sock); //Wait for a correct socket message

    std::thread thread([this, sock](){this->handle_session(sock);}); //Spawn a thread to handle the tcp session
    thread.detach();
  }
}

auto Tcp_server::handle_session(std::shared_ptr<tcp::socket> sock) noexcept ->void 
{
  std::cout << "Session created" << std::endl;
  try //Catch exception inside the session
  {
    while (true)
    {
      char data[max_length];
      size_t length;

      char response[max_length];
      size_t responseLength;

      boost::system::error_code error;
      length = sock->read_some(boost::asio::buffer(data), error);

      if (error == boost::asio::error::eof)
        break;
      else if (error)
        throw boost::system::system_error(error);

      m_service->process(data, length, response, responseLength);
      
      boost::asio::write(*sock, boost::asio::buffer(response, responseLength));
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception in thread: " << e.what() << "\n";
  }
}

// Baseline
// Request 00 03 00 00 00 06 00 03 00 01 00 06 
// Response 00 01 00 00 00 0F 00 03 0C 00 00 00 00 00 00 00 00 00 00 00 00
// https://rapidscada.net/modbus/ModbusParser.aspx/