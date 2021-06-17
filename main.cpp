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

class Modbus
{
  std::map<int,int> inputs;

  public:
  void process(char* input, char* response,  int& responseLength);
  void querryHoldingRegisters(char* input, char* response, int& responseLength);
};

Modbus * s_modbus;

typedef boost::shared_ptr<tcp::socket> socket_ptr;

void session(socket_ptr sock)
{
  try
  {
    while (true)
    {
      char data[max_length];

      char response[max_length];
      int responseLength;

      boost::system::error_code error;
      size_t length = sock->read_some(boost::asio::buffer(data), error);

      if (error == boost::asio::error::eof)
        break;
      else if (error)
        throw boost::system::system_error(error);

      // Sync transactionId
      response[0] = data[0];
      response[1] = data[1];

      // Protocol identifier (MODBUS)
      response[2] = 0;
      response[3] = 0;

      s_modbus->process(data, response, responseLength);

      std::cout << "\n=================================================================================================\n";
      
      std::cout << "Received: ";
      for (int i =0; i<length; i++)
      {
        std::cout << std::setfill ('0') << std::setw (2) << std::hex << (int)data[i] << " ";
      }

      std::cout << "\nResponse: ";
      for (int i =0; i<responseLength; i++)
      {
        std::cout << std::setfill ('0') << std::setw (2) << std::hex << (int)response[i] << " ";
      }
      
      std::cout << "\n=================================================================================================\n";
      
      boost::asio::write(*sock, boost::asio::buffer(response, responseLength));
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

void Modbus::process(char* input, char* response, int& responseLength)
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
      return this->querryHoldingRegisters(input, response, responseLength);
    default:
      break;
  }

}

void Modbus::querryHoldingRegisters(char* input, char* response, int& responseLength)
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

// Baseline
// Request 00 03 00 00 00 06 00 03 00 01 00 06 
// Response 00 01 00 00 00 0F 00 03 0C 00 00 00 00 00 00 00 00 00 00 00 00