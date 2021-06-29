#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::address;
using boost::asio::ip::tcp;

union u_Int
{
  char _char[2];
  unsigned int _int;
  u_Int() {this->_int = 0;}

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

class Tcp_client_interface
{
  public:
    virtual auto querry() ->std::string = 0;
    virtual auto read_response(const std::string& query, const std::string& response, const size_t response_length) ->void = 0;
};

class Modbus_client : public Tcp_client_interface
{
  public:
    virtual auto querry() ->std::string override;
    virtual auto read_response(const std::string& query, const std::string& response, const size_t response_length) ->void override;
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

    // using namespace boost::asio;
    // io_service service;
    // ip::tcp::endpoint ep( ip::address::from_string("192.168.0.11"), 9001);
    // ip::tcp::socket sock(service);
    // sock.connect(ep);

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

      // boost::array<char, 128> buf;
      // std::copy(query.begin(),query.end(),buf.begin());
      socket.write_some(boost::asio::buffer(query, query_length), ec);
      // sock.write_some(buffer("abcde"));
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


// Inplement interface (cout + cin) to generate the query
auto Modbus_client::querry() ->std::string
{
    std::string query = "";
    return query;
}

// Inplement valuer reading
auto Modbus_client::read_response(const std::string& query, const std::string& response, const size_t response_length) ->void
{
  std::cout << "\n=================================================================================================\n";
  
  std::cout << "Sent: ";
  for (int i =0; i<query.length(); i++)
  {
    std::cout  << static_cast<int>(query[i]) << " ";
  }

  std::cout << "\nResponse: ";
  for (int i =0; i<response_length; i++)
  {
    std::cout << static_cast<int>(response[i]) << " ";
  }
  
  std::cout << "\n=================================================================================================" << std::endl;
}