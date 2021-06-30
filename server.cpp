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
#include <boost/asio.hpp>

#ifdef _WIN32
  #include <windows.h>
#elif __linux__
  #include <fstream>
#else
  Not supported platform
#endif

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
    input[position]   = this->_char[1];
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

const int max_length = 1024;

#ifdef _WIN32
  //Função para retornar o uso da cpu
  //https://stackoverflow.com/questions/23143693/retrieving-cpu-load-percent-total-in-windows-with-c
  static float CalculateCPULoad(unsigned long long idleTicks, unsigned long long totalTicks)
  {
    static unsigned long long _previousTotalTicks = 0;
    static unsigned long long _previousIdleTicks = 0;

    unsigned long long totalTicksSinceLastTime = totalTicks-_previousTotalTicks;
    unsigned long long idleTicksSinceLastTime  = idleTicks-_previousIdleTicks;

    float ret = 1.0f-((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime)/totalTicksSinceLastTime : 0);

    _previousTotalTicks = totalTicks;
    _previousIdleTicks  = idleTicks;
    return ret;
  }

  static unsigned long long FileTimeToInt64(const FILETIME & ft) {return (((unsigned long long)(ft.dwHighDateTime))<<32)|((unsigned long long)ft.dwLowDateTime);}

  u_Float GetCPULoad()
  {
    FILETIME idleTime, kernelTime, userTime;
    float aux = GetSystemTimes(&idleTime, &kernelTime, &userTime) ? CalculateCPULoad(FileTimeToInt64(idleTime), FileTimeToInt64(kernelTime)+FileTimeToInt64(userTime)) : -1.0f;
    u_Float res;
    res._float = aux*100;
    return res;
  }

#elif __linux__
  auto getWorkingData() ->std::tuple<int, int>
  {
    int working_jiffies = 0;
    int total_jiffies = 0;
    std::ifstream file ("/proc/stat"); //Open processor stats file descriptor (on linux "Everything is a file)
    if (file.is_open()) {
        int buf;
        file.seekg(4);
        for (size_t i = 0; i <= 3; ++i)
        {
          file >> buf;
          working_jiffies += buf;
          total_jiffies += buf;
        }
        for (size_t i = 0; i <= 4; ++i)
        {
          file >> buf;
          total_jiffies += buf;
        }
        file.close();
    }

    return {working_jiffies, total_jiffies};
  }

  auto GetCPULoad() ->u_Float
  {
    auto [working_jiffies, total_jiffies] = getWorkingData();
    //TODO Getting cpu usage since bootup, in order to get current cpu usage, should be making 2 calls and use only de difference

    u_Float cpu_usage;
    cpu_usage._float = working_jiffies * 100;
    cpu_usage._float /= total_jiffies;
    return cpu_usage;
  }
#endif

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
  std::map<int,float> m_inputs;

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
  /* * * * * * * * * * * * * * * * 
   * char transactionID[2];
   * char protocolID[2];
   * char dataLength[2];
   * char unitID;
   * char functionCode;
   * char startAdress[2];
   * char quantity[2];
   *
   * transactionID[0]  = input[0];
   * transactionID[1]  = input[1];
   * protocolID[0]     = input[2];
   * protocolID[1]     = input[3];
   * dataLength[0]     = input[4];
   * dataLength[1]     = input[5];
   * unitID            = input[6];
   * functionCode      = input[7];
   * startAdress[0]    = input[8];
   * startAdress[1]    = input[9];
   * quantity[0]       = input[10];
   * quantity[1]       = input[11];
   * * * * * * * * * * * * * * * * */

  // The 8`th bit select the function
  switch (input[7])
  {
    case 3:
      this->querryHoldingRegisters(input, response, responseLength);
    default:
      //Can`t handle any other function
      break;
  }

  // Sync transactionId
  response[0] = input[0];
  response[1] = input[1];

  // Protocol identifier (MODBUS)
  response[2] = 0;
  response[3] = 0;

std::cout << "\n=================================================================================================\n";
  
  std::cout << "Received: " << inputLength << std::endl;
  for (int i =0; i<inputLength; i++)
  {
    std::cout  << static_cast<int>(input[i]) << " ";
  }

  std::cout << "\nResponse: "<< responseLength << std::endl;
  for (int i =0; i<responseLength; i++)
  {
    std::cout << static_cast<int>(response[i]) << " ";
  }
  
  std::cout << "\n=================================================================================================" << std::endl;
}


// TODO use data members values // or refactor to use work class interface instead of static function
auto Modbus_service::querryHoldingRegisters(char* input, char* response, size_t& responseLength) ->void
{
  u_Int startAddres;
  startAddres.read(input, 8); //Read byte 10 and 11 to get que quantity of queries;

  u_Int numberOfQueries;
  numberOfQueries.read(input, 10); //Read byte 10 and 11 to get que quantity of queries;

  // Data length from this point forward
  u_Int dataLength;
  dataLength._int = (numberOfQueries._int * 2) + 3;
  dataLength.write(response, 4);

  // Unit Identifier
  response[6] = 0;

  // Function Code
  response[7] = 3; //This member function treats only code 3

  // Data length from this point forward
  dataLength._int = dataLength._int - 3;
  response[8] = dataLength._char[0];
  
  //Resposta da cpuLoad
  u_Float CPULoad = GetCPULoad();
  std::cout << "CPU LOAD: " << CPULoad._float  << std::endl;
  CPULoad.write(response, 9);

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

  char data[max_length];
  size_t length;

  char response[max_length];
  size_t responseLength;

  boost::system::error_code error;
  
  //Catch exception inside the session thread
  try
  {
    // Handle messages until the session is closed
    while (true)
    {

      // Read a message
      length = sock->read_some(boost::asio::buffer(data), error);

      // Handle errors and disconections============
      if (error == boost::asio::error::eof)
        break;
      else if (error)
        throw boost::system::system_error(error);
      //============================================

      // Call the modbus service
      m_service->process(data, length, response, responseLength);
      
      // Send the response back
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
// http://rapidscada.net/modbus/ModbusParser.aspx