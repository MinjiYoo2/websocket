#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <string>
#include <thread>


using tcp = boost::asio::ip::tcp;

int main() {
    
    auto const address = boost::asio::ip::make_address("127.0.0.1");
    auto const port = static_cast<unsigned short>(std::atoi("8083"));

    boost::asio::io_context ioc{1};

    boost::asio::ip::tcp::acceptor acceptor{ioc, {address, port}};



    while(1){
        tcp::socket socket{ioc};
        acceptor.accept(socket);
        std::cout<< "socket accepted"<<std::endl;   
        std::thread{[q = std::move(socket)]() mutable {
    
        boost::beast::websocket::stream<tcp::socket> ws {std::move(const_cast<tcp::socket&>(q))};

        ws.set_option(boost::beast::websocket::stream_base::decorator(
            [](boost::beast::websocket::response_type& res){
                res.set(boost::beast::http::field::server, "connect websocket");
            }
        ));

        ws.accept();
            while(1){
                try{   
                    boost::beast::flat_buffer buffer;
                    ws.read(buffer);
                    auto out = boost::beast::buffers_to_string(buffer.cdata());
                    std::cout<< out<< std::endl;
                    ws.write(buffer.data());

                }catch(boost::beast::system_error const& se){
                    if(se.code() != boost::beast::websocket::error::closed)
                    {
                        std::cout << se.code().message() << std::endl;
                       //std::cerr << se.what() <<'\n';
                       break;
                    }
                }
            }

        }}.detach();
    }
    return 0;
}

