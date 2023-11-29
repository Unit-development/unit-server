#include <iostream>
#include "Server.h"
#include "HttpResponse.h"
#include "types.h"

int main()
{
    unit::server::handler::Server server("../config/config.toml");
    server.handle(unit::server::request::GET, R"(/)",
                  [&](unit::server::data::HttpRequest&request, unit::server::data::HttpResponse&response) {
                      response.writeFile("../sample_website/sample1.html");
                      response.addHeader((char *) ":status", (char *) "200");
                      response.addHeader((char *) "Content-Type", (char *) "text/html; charset=utf-8");
                  });
    server.start();
    return 0;
}
