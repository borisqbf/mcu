#ifndef WEBCONTROLLER_H
#define WEBCONTROLLER_H

#include <WebServer.h>
#include <Arduino.h>
#include <WiFi.h>
#include <ESP_Mail_Client.h>

#define MAX_ROUTES 10
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT esp_mail_smtp_port_587

/* The log in credentials */
#define AUTHOR_EMAIL "borisqbf@gmail.com"
#define AUTHOR_PASSWORD "wxumeztmydfrakhd"
#define RECIPIENT_EMAIL "boris_qbf@hotmail.com"

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
    void Alert(const char *message);
    static WebController *GetInstance();
    static void AddAction(const char *url, WebServer::THandlerFunction action);
    static UrlQueryParameter *GetUrlQueryParams();
    void SendHttpResponse(const char *message);

private:
    WebController();
    static WebServer *server;

    static void HandleRoot();
    static void HandleNotFound();
    static void InitMailClient();
    static WebController *theInstance;

    /* Callback function to get the Email sending status */
    static void SmtpCallback(SMTP_Status status);
    static Route routes[MAX_ROUTES];
    static int nextRouteIndex; // one for root
    static Session_Config config;
    static SMTPSession smtp;
};

#endif