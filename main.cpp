#include <crow.h>
#include <asio.hpp>

int main()
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([] {
        return "Hello WizzMania 🚀";
    });

    app.port(18080).multithreaded().run();
}
