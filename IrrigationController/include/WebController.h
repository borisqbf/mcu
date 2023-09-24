#ifndef WEBCONTROLLER_H
#define WEBCONTROLLER_H

#include <WebServer.h>

#define MAX_ROUTES 10

class WebController
{
    public:
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
};

#endif