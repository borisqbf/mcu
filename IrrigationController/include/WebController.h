#ifndef WEBCONTROLLER_H
#define WEBCONTROLLER_H

#include <WebServer.h>
#include <Arduino.h>
#include <WiFi.h>

#define MAX_ROUTES 10

class WebController
{
public:
    struct Route
    {
        const char *url;
        WebServer::THandlerFunction handler;
    };

    struct UrlQueryParameter
    {
        String p;
        String v;
    };

public:
    void Setup();
    void ProcessMainLoop();
    static WebController *GetInstance();
    static void AddAction(const char *url, WebServer::THandlerFunction action);
    static UrlQueryParameter *GetUrlQueryParams();
    void SendHttpResponseOK(const char *message);
    void SendHttpResponseBadRequest(const char *message);

private:
    WebController();
    static WebServer *server;

    static void HandleRoot();
    static void HandleNotFound();
    static WebController *theInstance;
    static Route routes[MAX_ROUTES];
    static int nextRouteIndex; // one for root
};

#endif