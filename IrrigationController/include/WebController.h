#ifndef WEBCONTROLLER_H
#define WEBCONTROLLER_H

#include <WebServer.h>

#define MAX_ROUTES 20

class WebController
{
    struct Route
    {
        const char *url;
        WebServer::THandlerFunction handler;
    };

public:
    WebController();
    void Setup();
    void ProcessMainLoop();
    void Alert(const char *message);
    static WebController *GetInstance();
    static void AddAction(const char* url, WebServer::THandlerFunction action);
    void SendHttpResponse(const char *message);

private:
    WebServer *server;
    static void HandleRoot();
    static void HandleNotFound();
    static Route routes[MAX_ROUTES];
    static int nextRouteIndex; // one for root
};

#endif