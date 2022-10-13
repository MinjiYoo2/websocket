#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <memory>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;

using tcp = boost::asio::ip::tcp;

// shared_ptr 형태 => error
// class session : public std::enable_shared_from_this<session>
// {
//     tcp::resolver resolver_;
//     websocket::stream<tcp::socket> toMaster;
//     boost::beast::multi_buffer buffer_;


//     std::string host_;
//     std::string text_;

// public:
//     // Resolver and socket require an io_context
//     explicit session (boost::asio::io_context & ioc) : resolver_(ioc), ws_(ioc)
//     {
//     }

//     // Start the asynchronous operation
//     void run(char const* host, char const* port, char const* text)
//     {
//         // Save these for later
//         host_ = host;
//         text_ = text;

//         // resolve 
//         resolver_.async_resolve(host, port, [self{shared_from_this()}](beast::error_code ec, tcp::resolver::results_type results){ 
//             if(ec == websocket::error::closed) return; 
//             if(ec) { std::cout<< ec.message() << std::endl; return;}
//             self -> on_connect();
//         });
//     }
//     void on_connect(){
//         tcp::resolver::results_type results; 

//         boost::asio::async_connect(toMaster.next_layer(), results.begin(), results.end(), [self{shared_from_this()}](beast::error_code ec){
//             if(ec == websocket::error::closed) return; 
//             if(ec) { std::cout<< ec.message() << std::endl; return;}
//             self -> on_handshake();
//         });
//     }
//     void on_handshake(){
//          toMaster.async_handshake(host_, "/", [self{shared_form_this()}](beast::error_code ec){
//             if(ec == websocket::error::closed) return; 
//             if(ec) { std::cout<< ec.message() << std::endl; return;}
//             self -> on_write();
//         });
//     }
//     void on_write(){
//         toMaster.async_write( boost::asio::buffer(text_),, [self](beast::error_code ec, std::size_t bytes_transferred){
//                     if(ec == websocket::error::closed) return; 
//                     if(ec){ std::cout<<ec.message() << std::endl; return; }
//                     self -> on_read();
//                 });
//     }
//     void on_read(){
//         wsServer.async_read(buffer_, [self{shared_from_this()}](beast::error_code ec, std::size_t bytes_transferred){
//                 if(ec == websocket::error::closed) return;
//                 if(ec) { std::cout<<ec.message() << std::endl; return;}
//                 std::cout << boost::beast::make_printable(buffer_.data()) << std::endl;
//                 //auto out = beast::buffers_to_string(self->buffer.cdata());
//                 //std::cout<<out<<std::endl;
//     }
// }

class session : public std::enable_shared_from_this<session>
{
    tcp::resolver resolver_;
    websocket::stream<tcp::socket> ws_;
    boost::beast::multi_buffer buffer_;

    std::string host_;
    std::string text_;

public:
    explicit session (boost::asio::io_context & ioc) : resolver_(ioc), ws_(ioc)
    {
    }
    
    websocket::stream<tcp::socket> getSocket (){
        return std::move(ws_);
    }

    void run(char const* host, char const* port, char const* text)
    {
        host_ = host;
        text_ = text;

        resolver_.async_resolve(
            host, 
            port, 
            std::bind( &session::on_resolve, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }

    void on_resolve( boost::system::error_code ec, tcp::resolver::results_type results)
    {
        if(ec)
            return fail(ec, "resolve");
        
        boost::asio::async_connect(
            ws_.next_layer(),
            results.begin(),
            results.end(),
            std::bind(
                &session::on_connect,
                shared_from_this(),
                std::placeholders::_1));
    }

    void on_connect(boost::system::error_code ec)
    {
        if(ec)
            return fail(ec, "connect");
        
        ws_.async_handshake(host_ ,"/",
            std::bind(
                &session::on_handshake,
                shared_from_this(),
                std::placeholders::_1));
    } 

    void on_handshake(boost::system::error_code ec)
    {
        if(ec)
            return fail(ec, "handshake");
        
        ws_.async_write(
            boost::asio::buffer(text_),
            std::bind(
                &session::on_write,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    }

    void on_write(boost::system::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");
        
        ws_.async_read(
            buffer_,
            std::bind(
                &session::on_read,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    }
    
    void on_read(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);

            if(ec)
                return fail(ec, "read");

            ws_.async_close(websocket::close_code::normal, 
            std::bind(
                &session::on_close,
                shared_from_this(),
                std::placeholders::_1));
        }

    void on_close(boost::system::error_code ec)
    {
        if(ec)
            return fail(ec, "close");
        
        std::cout << boost::beast::make_printable(buffer_.data()) << std::endl;
    }
};


class ServerSocket : public std::enable_shared_from_this<ServerSocket>
{
    websocket::stream<beast::tcp_stream> wsServer; // 
    beast::flat_buffer buffer;
    net::io_context ioc;

    public:
        ServerSocket(net::io_context ioc, tcp::socket&& socket): wsServer(std::move(ioc) std::move(socket)){
            self->ioc = std::move(ioc);
        }
        void run(){
            wsServer.async_accept([self{shared_from_this()}](beast::error_code ec){
                if(ec){ 
                    std::cout<< ec.message() << std::endl; return;
                }
                self -> read();
            });
        }
        void read(){
            wsServer.async_read(buffer, [self{shared_from_this()}](beast::error_code ec, std::size_t bytes_transferred){
                if(ec == websocket::error::closed) return;
                if(ec) { std::cout<<ec.message() << std::endl; return;}
                auto out = beast::buffers_to_string(self->buffer.cdata());
                std::cout<<out<<std::endl;

                std::make_shared<session>(ioc)->run("127.0.0.1", "8084", "toMaster");

            });
        }
};

class Listener : public std::enable_shared_from_this<Listener>
{
    net::io_context& ioc;
    tcp::acceptor acceptor;

public:
        Listener(net::io_context& ioc, unsigned short int port):ioc(ioc), acceptor(ioc, { net::ip::make_address("127.0.0.1"), port}){}

    void asyncAccept()
    {
        acceptor.async_accept(ioc, [self{shared_from_this()}](boost::system::error_code ec, tcp::socket socket){
            //pass socket to EchoWebSocket
            std::make_shared<ServerSocket>(std::move(ioc), std::move(socket))-> run();

            std::cout<<"connection accepted"<<std::endl;
            self->asyncAccept();
        });
    }
};
int main(int argc, char* argv[]){
    auto const port = 8083;
    net::io_context ioc{};
    std::make_shared<Listener>(ioc, port)->asyncAccept();

    ioc.run();

    return 0;
}