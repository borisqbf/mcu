#include <ESPmDNS.h>
#include <Chronos.h>
#include "WebController.h"

// Static members
WebController *WebController::theInstance = NULL;
WebController::Route WebController::routes[MAX_ROUTES];
Session_Config WebController::config;
SMTPSession WebController::smtp;
int WebController::nextRouteIndex = 1; // one for root
WebServer *WebController::server = NULL;

WebController *WebController::GetInstance()
{
    if (theInstance == NULL)
    {
        theInstance = new WebController();

        // returning the instance pointer
        return theInstance;
    }
    else
    {
        return theInstance;
    }
}

void WebController::HandleRoot()
{
    String message = "Hello resolved by mDNS!\n\n";
    message += "\nAvailable routes are:\n";
    for (int i = 0; i < MAX_ROUTES; i++)
    {
        if (routes[i].url == nullptr)
            break;
        message += routes[i].url;
        message += "\n";
    }

    server->send(200, "text/plain", message);
}

void WebController::HandleNotFound()
{
    String message = "Rote Not Found\n\n";
    message += "URI: ";
    message += server->uri();
    message += "\nMethod: ";
    message += (server->method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server->args();
    message += "\n";
    for (uint8_t i = 0; i < server->args(); i++)
    {
        message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
    }
    message += "\nAvailable routes are:\n";
    for (int i = 0; i < MAX_ROUTES; i++)
    {
        if (routes[i].url == nullptr)
            break;
        message += routes[i].url;
        message += "\n";
    }

    Chronos::DateTime n = Chronos::DateTime::now();
    message += "Current time is ";
    message += n.day();
    message += "/";
    message += n.month();
    message += "/";
    message += n.year();
    message += " ";
    message += n.hour();
    message += ":";
    if (n.minute() < 10)
        message += "0";
    message += n.minute();
    message += "\n";

    server->send(404, "text/plain", message);
}

void WebController::AddAction(const char *url, WebServer::THandlerFunction action)
{
    if (nextRouteIndex < MAX_ROUTES)
    {
        routes[nextRouteIndex++] = {url, action};
        server->on(url, action);
    }
    else
        Serial.println("No more route slots available");
}

WebController::UrlQueryParameter *WebController::GetUrlQueryParams()
{
    if (server->args() == 0)
        return NULL;
    else
    {
        WebController::UrlQueryParameter *retVal = new WebController::UrlQueryParameter[server->args() + 1];
        uint8_t i = 0;
        for (i = 0; i < server->args(); i++)
        {
            retVal[i] = {server->argName(i), server->arg(i)};
        }
        retVal[i] = {"", ""};
        return retVal;
    }
}

WebController::WebController()
{
    server = new WebServer(80);
    routes[0] = {"/", &WebController::HandleRoot};
    for (int i = 1; i < MAX_ROUTES; i++)
    {
        routes[i].url = nullptr;
        routes[i].handler = nullptr;
    }
}

void WebController::Setup()
{
    if (MDNS.begin("irrigation-controller"))
    { // Start mDNS

        Serial.println("MDNS started");
    }

    server->onNotFound(HandleNotFound);

    server->begin(); // Start server
    Serial.println("HTTP server started");
    InitMailClient();
}

void WebController::ProcessMainLoop()
{
    server->handleClient();
    delay(2); // allow the cpu to switch to other tasks
}

void WebController::SendHttpResponseOK(const char *message)
{
    server->send(200, "text/plain", message);
}

void WebController::SendHttpResponseBadRequest(const char *message)
{
    server->send(200, "text/plain", message);
}

void WebController::InitMailClient()
{
    MailClient.networkReconnect(true);
    smtp.debug(1);

    /* Set the callback function to get the sending results */
    smtp.callback(SmtpCallback);

    /* Set the session config */
    config.server.host_name = SMTP_HOST;
    config.server.port = SMTP_PORT;
    config.login.email = AUTHOR_EMAIL;
    config.login.password = AUTHOR_PASSWORD;
    config.login.user_domain = F("127.0.0.1");

    config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
    config.time.gmt_offset = 10;
    config.time.day_light_offset = 0;
}

void WebController::SmtpCallback(SMTP_Status status)
{
    /* Print the sending result */
    if (status.success())
    {
        smtp.sendingResult.clear();
    }
    else
    {
        Serial.println(status.info());
    }
}

void WebController::Alert(const char *alert)
{
    /* Declare the message class */
    SMTP_Message message;

    /* Set the message headers */
    message.sender.name = F("Irrigation Controller");
    message.sender.email = AUTHOR_EMAIL;
    message.subject = F("Alert");
    message.addRecipient(F("Admin"), RECIPIENT_EMAIL);

    message.text.content = F(alert);

    /** The message priority
     * esp_mail_smtp_priority_high or 1
     * esp_mail_smtp_priority_normal or 3
     * esp_mail_smtp_priority_low or 5
     * The default value is esp_mail_smtp_priority_low
     */
    message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_normal;

    /* Connect to the server */
    if (!smtp.connect(&config))
    {
        MailClient.printf("Connection error, Status Code: %d, Error Code: %d, Reason: %s\n", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
        return;
    }

    if (!smtp.isLoggedIn())
    {
        Serial.println("Error, Not yet logged in.");
    }
    /* Start sending Email and close the session */
    if (!MailClient.sendMail(&smtp, &message))
        MailClient.printf("Error, Status Code: %d, Error Code: %d, Reason: %s\n", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
}
