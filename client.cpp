#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::address;
using boost::asio::ip::tcp;

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
    Tcp_client(std::shared_ptr<Tcp_client_interface>& client, const std::string& ipAddress, const int port);
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

Tcp_client::Tcp_client(std::shared_ptr<Tcp_client_interface>& client, const std::string& ipAddress, const int port)
{
    boost::system::error_code ec; // Boost asio tends not to use c++ exceptions

    tcp::socket socket(m_io_service);

    address ip_address = address::from_string(ipAddress, ec);
    if (ec)
    {
        throw "Could not create address";
    }

    tcp::endpoint ep(ip_address, port);

    while ( true )
    {
      std::string query;

      std::string response;
      size_t response_length;
      
      query = this->m_client->querry();
      socket.write_some(boost::asio::buffer(query, query.length()), ec);

      response_length = socket.read_some(boost::asio::buffer(response), ec);

      // Handle errors and disconections============
      if (ec == boost::asio::error::eof)
        break;
      else if (ec)
        throw boost::system::system_error(ec);
      //============================================

      this->m_client->read_response(query, response, response_length);
    }
}

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
