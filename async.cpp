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


class EchoWebsocket : public std::enable_shared_from_this<EchoWebsocket>
{
    websocket::stream<beast::tcp_stream> ws;
    beast::flat_buffer buffer;

    public:
        EchoWebsocket(tcp::socket&& socket): ws(std::move(socket)){}
        void run(){
            ws.async_accept([self{shared_from_this()}](beast::error_code ec){
                if(ec){ 
                    std::cout<< ec.message() << std::endl; return;
                }
                self -> echo();
            });
        }
        void echo(){
            ws.async_read(buffer, [self{shared_from_this()}](beast::error_code ec, std::size_t bytes_transferred){
                if(ec == websocket::error::closed) return;
                if(ec) { std::cout<<ec.message() << std::endl; return;}
                auto out = beast::buffers_to_string(self->buffer.cdata());
                std::cout<<out<<std::endl;

                // RESPONSE
                // self->ws.async_write(self->buffer.data(), [self](beast::error_code ec, std::size_t bytes_transferred){
                //     if(ec){ std::cout<<ec.message() << std::endl; return; }
                //     self -> buffer.consume(self-> buffer.size());
                //     self->echo();
                // });
            });
        }
};

class Listener : public std::enable_shared_from_this<Listener>
{
    net::io_context& ioc;
    tcp::acceptor acceptor;

    public:
        Listener(net::io_context& ioc,
        unsigned short int port):ioc(ioc),
        acceptor(ioc, { net::ip::make_address("127.0.0.1"), port}){}

    void asyncAccept()
    {
        acceptor.async_accept(ioc, [self{shared_from_this()}](boost::system::error_code ec, tcp::socket socket){
            //pass socket to EchoWebSocket
            std::make_shared<EchoWebsocket>(std::move(socket))-> run();
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