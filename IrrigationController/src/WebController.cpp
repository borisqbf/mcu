#include <ESPmDNS.h>
#include "WebController.h"

static WebController theInstance;

void WebController::HandleRoot()
{
    String message = "Hello resolved by mDNS!\n\n";
    message += "\nAvailable routes are:\n";
    for (int i = 0; i < MAX_ROUTES; i++)
    {
        if (routes[i].url == NULL)
            break;
        message += routes[i].url;
        message += "\n";
    }

    theInstance.server->send(200, "text/plain", message);
}

void WebController::HandleNotFound()
{
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += theInstance.server->uri();
    message += "\nMethod: ";
    message += (theInstance.server->method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += theInstance.server->args();
    message += "\n";
    for (uint8_t i = 0; i < theInstance.server->args(); i++)
    {
        message += " " + theInstance.server->argName(i) + ": " + theInstance.server->arg(i) + "\n";
    }
    message += "\nAvailable routes are:\n";
    for (int i = 0; i < MAX_ROUTES; i++)
    {
        if (routes[i].url == NULL)
            break;
        message += routes[i].url;
        message += "\n";
    }
    theInstance.server->send(404, "text/plain", message);
}

WebController *WebController::GetInstance()
{
    return &theInstance;
}

void WebController::AddAction(const char *url, WebServer::THandlerFunction action)
{
    if (nextRouteIndex < MAX_ROUTES)
        WebController::routes[nextRouteIndex++] = {url, action};
    else
        Serial.println("No more route slots available");
}

WebController::WebController()
{
    server = new WebServer(80);
    nextRouteIndex = 1;
}

void WebController::Setup()
{
    if (MDNS.begin("irrigation-controller"))
    { // Start mDNS

        Serial.println("MDNS started");
    }
    routes[0] = {"/", &WebController::HandleRoot};
    for (int i = 0; i < MAX_ROUTES; i++)
    {
        if (routes[i].url == NULL)
            break;
        server->on(routes[i].url, routes[i].handler);
    }

    server->onNotFound(HandleNotFound);

    server->begin(); // Start server
    Serial.println("HTTP server started");
}

void WebController::ProcessMainLoop()
{
    server->handleClient();
}

void WebController::SendHttpResponse(const char *message)
{
    // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
    // and a content-type so the client knows what's coming, then a blank line:

    server->send(200, "text/plain", message);
}

void WebController::Alert(const char *message)
{
}
