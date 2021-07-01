#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::address;
using boost::asio::ip::tcp;

union u_Int
{
  unsigned int _int = 0;
  char _char[2];

  void read(char* data, int position) 
  {
    this->_char[1] = data[position]; 
    this->_char[0] = data[position+1];
  }

  void write(char* input, int position) 
  {
    input[position] = this->_char[1];
    input[position+1] = this->_char[0];
  }
};

union u_Float
{
  float _float = 0;
  char _char[4];

  void write(char* input, int position) 
  {
    input[position]   = this->_char[1];
    input[position+1] = this->_char[0];
    input[position+2] = this->_char[3];
    input[position+3] = this->_char[2];
  }

  void read(char* input, int position)
  {
    this->_char[0] = input[position+1];
    this->_char[1] = input[position];
    this->_char[2] = input[position+3];
    this->_char[3] = input[position+2];
  }

  void print()
  {
    std::cout << "Float values " 
    << (int)this->_char[0] << " " << (int)this->_char[1] << " " 
    << (int)this->_char[2] << " " << (int)this->_char[3] << std::endl;
  }
};

class Tcp_client_interface
{
  public:
    virtual auto query(char* query, size_t& query_length) ->void = 0;
    virtual auto read_response(char* query, const size_t query_length, char* response, const size_t response_length) ->void = 0;
};

class Modbus_client : public Tcp_client_interface
{
  u_Int m_message_number = 0;

  public:
    virtual auto query(char* query, size_t& query_length) ->void override;
    virtual auto read_response(char* query, const size_t query_length, char* response, const size_t response_length) ->void override;
};

class Tcp_client
{
  boost::asio::io_service m_io_service; //Contain platform specific code to handle i/o context (socket readines, file descriptors...)
  std::shared_ptr<Tcp_client_interface> m_client;

  public:
    Tcp_client(std::shared_ptr<Tcp_client_interface> client, const std::string& ipAddress, const int port);
};

int main(int argc, char* argv[])
{
  try
  {
    std::shared_ptr<Tcp_client_interface> modbus_client = std::make_shared<Modbus_client>();

    Tcp_client(modbus_client, "192.168.0.11", 9001);
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}

Tcp_client::Tcp_client(std::shared_ptr<Tcp_client_interface> client, const std::string& ipAddress, const int port)
  :m_client(client)
{
  try{
    boost::system::error_code ec; // Boost asio tends not to use c++ exceptions

    tcp::socket socket(m_io_service);

    address ip_address = address::from_string(ipAddress, ec);
    if (ec)
    {
        throw "Could not create address";
    }

    tcp::endpoint ep(ip_address, port);

    socket.connect(ep);

    while ( true )
    {
      char query[200];
      size_t query_length;

      char response[200];
      size_t response_length;
      
      this->m_client->query(query, query_length);

      socket.write_some(boost::asio::buffer(query, query_length), ec);
      if (ec)
        throw boost::system::system_error(ec);

      response_length = socket.read_some(boost::asio::buffer(response), ec);

      // Handle errors and disconections============
      if (ec == boost::asio::error::eof)
        break;
      else if (ec)
        throw boost::system::system_error(ec);
      //============================================

      this->m_client->read_response(query, query_length, response, response_length);
    }
  } 
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }
}

// Baseline
// Request 00 03 00 00 00 06 00 03 00 01 00 06 
// Response 00 01 00 00 00 0F 00 03 0C 00 00 00 00 00 00 00 00 00 00 00 00
// https://rapidscada.net/modbus/ModbusParser.aspx/


auto Modbus_client::query(char* query, size_t& query_length) ->void
{
  u_Int register_start;
  u_Int number_of_registers;

  std::cout << "\n=================================================================================================\n";

  do {
    std::cout << "Which register to start the read?" <<std::endl;
    std::cin >> register_start._int;
  } while (0 > register_start._int || register_start._int > 999);

  do {
    std::cout << "How many floats to read?" << std::endl;
    std::cin >> number_of_registers._int;
  } while (0 > number_of_registers._int || number_of_registers._int > 10);
  number_of_registers._int *= 2;

  // transactionID[0]  = input[0];
  // transactionID[1]  = input[1];
  // protocolID[0]     = input[2];
  // protocolID[1]     = input[3];
  // dataLength[0]     = input[4];
  // dataLength[1]     = input[5];
  // unitID[0]         = input[6];
  // functionCode[0]   = input[7];
  // startAdress[0]    = input[8];
  // startAdress[1]    = input[9];
  // quantity[0]       = input[10];
  // quantity[1]       = input[11];

  m_message_number.write(query, 0);

  // protocolID
  query[2] = 0;
  query[3] = 0;

  // dataLength
  query[4] = 0;
  query[5] = 6;

  // unitId
  query[6] = 0;

  // functionCode
  query[7] = 3;

  // register addres
  register_start.write(query, 8);

  // number of registers
  number_of_registers.write(query, 10);

  query_length = 12;
}

auto Modbus_client::read_response(char* query, const size_t query_length, char* response, const size_t response_length) ->void
{
  std::cout << "=================================================================================================\n";
  
  std::cout << "Sent: ";
  for (int i =0; i < query_length; i++)
  {
    std::cout  << static_cast<int>(query[i]) << " ";
  }

  std::cout << "\nResponse: ";
  for (int i =0; i<response_length; i++)
  {
    std::cout << static_cast<int>(response[i]) << " ";
  }
  std::cout << "\n=================================================================================================\n\n" << std::endl;

  // Check if message received is the response of the query
  if (query[0]!=response[0] || query[1]!=response[1])
  {
    std::cerr << "Messages arriving out of order. \n Expected " << (int)query[0] << " " << (int)query[1];
    std::cerr << "\n Got " << (int)response[0] << " " << (int)response[1] << std::endl;

    return;
  }

  u_Int register_start;
  register_start.read(query, 8); //The position of the start addres was writen in the query position 8
  std::cout << "Registers start addres " << (int)register_start._int << std::endl;

  u_Int number_of_registers;
  number_of_registers.read(query, 10); //The number of registers read was writen in the query position 10

  //We are expecting floats so
  if ((number_of_registers._int % 2) == 1) 
  {
    std::cerr << "Expecting float, should have been an even number of registers in the response. Got " << number_of_registers._int << std::endl;
    return; //TODO Just read the int?
  }
  number_of_registers._int /= 2;

  for (size_t i=0; i < number_of_registers._int; i+=4) //The first register value is writen in position 10 of the response.
  {
    u_Float register_value;
    register_value.read(response, 9);

    std::cout << "Register " << register_start._int + i << " has value: " << (float)register_value._float << std::endl;
  }
}