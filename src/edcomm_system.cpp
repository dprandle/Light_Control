#include <edtimer.h>
#include <unistd.h>
#include <edmctrl.h>
#include <edcomm_system.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <edutility.h>
#include <string.h>
#include <errno.h>
#include <edsocket.h>
#include <edlogger.h>

Command::Command()
{
    zero_buf(data,COMMAND_BYTE_SIZE);
}

edcomm_system::edcomm_system():
    m_server_fd(0),
	m_port(0),
    m_cur_cmd(),
    m_cur_index(0)
{}

edcomm_system::~edcomm_system()
{}
	
void edcomm_system::init()
{
	sockaddr_in server;

    m_server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);//
	if (m_server_fd < 0)
    {
        int err = errno;
        elog("Could not create server! Errno: {}",err);
    }

    int optval = 1;
    setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

    server.sin_addr.s_addr = INADDR_ANY;
	server.sin_family = AF_INET;
    server.sin_port = htons(m_port);

    if (bind(m_server_fd, (struct sockaddr *) &server, sizeof(server)) < 0)
    {
        int err = errno;
        elog("Could not bind server! Errno: {}",err);
    }

	listen(m_server_fd, 5);
    ilog("Listening on port {}",m_port);
}

uint16_t edcomm_system::port()
{
	return m_port;
}

void edcomm_system::set_port(uint16_t port_)
{
	m_port = port_;
}

void edcomm_system::release()
{
    while (m_clients.begin() != m_clients.end())
    {
        delete m_clients.back();
        m_clients.pop_back();
    }
    close(m_server_fd);
}

uint32_t edcomm_system::recvFromClients(uint8_t * data, uint32_t max_size)
{
    uint32_t total = 0;
    for (uint32_t i = 0; i < m_clients.size(); ++i)
        total += m_clients[i]->read(data+total, max_size-total);
    return total;
}

void edcomm_system::sendToClients(uint8_t * data, uint32_t size)
{
    for (uint32_t i = 0; i < m_clients.size(); ++i)
        m_clients[i]->write(data, size);
}

void edcomm_system::update()
{
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int32_t sockfd = accept(m_server_fd, (struct sockaddr *) &client_addr, &client_len);
    if (sockfd != -1)
    {
        edsocket * new_client = new edsocket(sockfd);
        if (!new_client->start())
        {
            wlog("Received connection but could not start socket - should see deletion next");
        }
        m_clients.push_back(new_client);
        ilog("Server recieved connection from {}:{}",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
    }

    // Check for closed connections and remove them if there are any
    _clean_closed_connections();

    static uint8_t buf[256];
    int32_t cnt = recvFromClients(buf, 256);
    for (int32_t i = 0; i < cnt; ++i)
        _handle_byte(buf[i]);
}

void edcomm_system::_clean_closed_connections()
{
    ClientArray::iterator iter = m_clients.begin();
    while (iter != m_clients.end())
    {
        if (!(*iter)->running())
        {
            sockaddr_in cl_addr;
            socklen_t cl_len = sizeof(cl_addr);
            getpeername((*iter)->fd(), (sockaddr *)&cl_addr, &cl_len);
            std::string client_ip = std::string(inet_ntoa(cl_addr.sin_addr)) + ":" + std::to_string(ntohs(cl_addr.sin_port));

            edthreaded_fd::Error er = (*iter)->error();
            std::string errno_message = strerror(er._errno);

            switch(er.err_val)
            {
              case(edthreaded_fd::ConnectionClosed):
                  ilog("Connection closed with {}",client_ip);
                  break;
              case (edthreaded_fd::DataOverwrite):
                  elog("Socket internal buffer overwritten with new data before previous data was sent {} - Errno message: {}",client_ip,errno_message);
                  break;
              case (edthreaded_fd::InvalidRead):
                  elog("Socket invalid read from {} - Errno message {}",client_ip, errno_message);
                  break;
              case (edthreaded_fd::InvalidWrite):
                  elog("Socket invalid write to {} - Errno message: {}",client_ip, errno_message);
                  break;
              case (edthreaded_fd::ThreadCreation):
                  elog("Error in thread creation for connection with {}",client_ip);
                  break;
              default:
                  elog("No internal error but socket thread not running with {}",client_ip);
                  break;
            }
            delete (*iter);
            iter = m_clients.erase(iter);
        }
        else
            ++iter;
    }
}

void edcomm_system::_handle_byte(uint8_t byte)
{
    m_cur_cmd.data[m_cur_index] = byte;
    ++m_cur_index;
    if (m_cur_index == COMMAND_BYTE_SIZE)
    {
        _do_command();
        m_cur_index = 0;
        zero_buf(m_cur_cmd.data, 8);
    }
}

void edcomm_system::_do_command()
{
    // uint32_t rphash = hash_id(rplidar_request::Type());
    // uint32_t nav_sys_command = hash_id(nav_system_request::Type());

    // if (m_cur_cmd.hash_id == rphash)
    // {
    //     rplidar_request::req_type rt = static_cast<rplidar_request::req_type>(m_cur_cmd.cmd_data);
    //     log_message("Sending rplidar request type: " + std::to_string(rt));
    //     rplidar_request * req = edm.message_dispatch()->push<rplidar_request>();
    //     if (req != NULL)
    //         req->r_type = rt;
    // }
    // else if (m_cur_cmd.hash_id == nav_sys_command)
    // {
    //     nav_system_request * rmsg = edm.message_dispatch()->push<nav_system_request>();
    //     if (rmsg != NULL)
    //     {
    //         rmsg->pid.set(m_cur_cmd.cmd_data_d,m_cur_cmd.cmd_data_d2, m_cur_cmd.cmd_data_d3);
    //         rmsg->ramp_limit = m_cur_cmd.cmd_data_d4;
    //         rmsg->complex_der = ((m_cur_cmd.cmd_data & 0x01) == 0x01);
    //         rmsg->anti_reset_winding = ((m_cur_cmd.cmd_data & 0x10) == 0x10);
    //         rmsg->threshold_dropout = ((m_cur_cmd.cmd_data & 0x0100) == 0x0100);
    //         rmsg->bias_vec.set(m_cur_cmd.cmd_data_d6, m_cur_cmd.cmd_data_d5);
    //         rmsg->g_factor = m_cur_cmd.cmd_data_d7;
    //         rmsg->bias_threshold_dist = m_cur_cmd.cmd_data_d8;
    //     }
    // }
}
