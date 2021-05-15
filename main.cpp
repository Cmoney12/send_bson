#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <chrono>
#include "chat_message.h"

using boost::asio::ip::tcp;

typedef std::deque<chat_message *> chat_message_queue;

std::chrono::time_point<std::chrono::high_resolution_clock> start;

void time_it() {
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        std::cout << duration.count() << std::endl;
}

class chat_client
{
public:
    chat_client(boost::asio::io_context& io_context,
                const tcp::resolver::results_type& endpoints)
            : io_context_(io_context),
              socket_(io_context)
    {
        do_connect(endpoints);
    }

    void write(chat_message *msg)
    {
        boost::asio::post(io_context_,
                          [this, msg] ()
                          {
                              bool write_in_progress = !write_msgs_.empty();
                              write_msgs_.push_back(msg);
                              if (!write_in_progress)
                              {
                                  do_write();
                              }
                          });
    }

    void close()
    {
        boost::asio::post(io_context_, [this]() { socket_.close(); });
    }

private:
    void do_connect(const tcp::resolver::results_type& endpoints)
    {
        boost::asio::async_connect(socket_, endpoints,
                                   [this](boost::system::error_code ec, tcp::endpoint)
                                   {
                                       if (!ec)
                                       {
                                           do_read_header();
                                       }
                                   });
    }

    void do_read_header()
    {
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_.header, chat_message::HEADER_LENGTH),
                                [this](boost::system::error_code ec, std::size_t /*length*/)
                                {
                                    if (!ec && read_msg_.decode_header()) {
                                        do_read_body();
                                    }
                                    else {
                                        socket_.close();
                                    }
                                });
    }

    void do_read_body()
    {
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
                                [this](boost::system::error_code ec, std::size_t /*length*/)
                                {
                                    if (!ec)
                                    {
                                        time_it();
                                        chat_message::parse_bson(read_msg_.body(), read_msg_.body_length_);
                                        do_read_header();
                                    }
                                    else
                                    {
                                        socket_.close();
                                    }
                                });
    }

    void do_write()
    {
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(write_msgs_.front()->data(),
                                                     write_msgs_.front()->length()),
                                 [this](boost::system::error_code ec, std::size_t /*length*/)
                                 {
                                     if (!ec)
                                     {

                                         write_msgs_.pop_front();
                                         delete write_msgs_.front();
                                         if (!write_msgs_.empty())
                                         {
                                             do_write();
                                         }
                                     }
                                     else
                                     {
                                         socket_.close();
                                     }
                                 });
    }

private:
    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    chat_message read_msg_;
    chat_message_queue write_msgs_;
};

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 3)
        {
            std::cerr << "Usage: chat_client <host> <port>\n";
            return 1;
        }

        boost::asio::io_context io_context;

        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(argv[1], argv[2]);
        chat_client c(io_context, endpoints);

        std::thread t([&io_context](){ io_context.run(); });

        char line[chat_message::MAX_MESSAGE_SIZE + 1];
        while (std::cin.getline(line, chat_message::MAX_MESSAGE_SIZE + 1))
        {
            chat_message *message = new chat_message;
            char user[] = "user";
            char type[] = "text";
            message->create_bson(user, user, type, line);
            message->set_size(message->body_length_);
            std::memcpy(message->body(), message->bson, message->body_length_);
            message->encode_header();

            c.write(message);

        }

        c.close();
        t.join();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}