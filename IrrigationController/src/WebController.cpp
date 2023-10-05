#include <ESPmDNS.h>
#include <Chronos.h>
#include "WebController.h"

// Static members
WebController* WebController::theInstance = NULL;
WebController::Route WebController::routes[MAX_ROUTES];
Session_Config WebController::config;
SMTPSession WebController::smtp;
int WebController::nextRouteIndex = 1; // one for root
WebServer *WebController::server = NULL;

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
        if (routes[i].url == NULL)
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

void WebController::AddAction(const char *url, WebServer::THandlerFunction action)
{
    if (nextRouteIndex < MAX_ROUTES)
        routes[nextRouteIndex++] = {url, action};
    else
        Serial.println("No more route slots available");
}

WebController::WebController()
{
    server = new WebServer(80);
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
    InitMailClient();
}

void WebController::ProcessMainLoop()
{
    server->handleClient();
    delay(2); // allow the cpu to switch to other tasks
}

void WebController::SendHttpResponse(const char *message)
{
    // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
    // and a content-type so the client knows what's coming, then a blank line:

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

    /*
    Set the NTP config time
    For times east of the Prime Meridian use 0-12
    For times west of the Prime Meridian add 12 to the offset.
    Ex. American/Denver GMT would be -6. 6 + 12 = 18
    See https://en.wikipedia.org/wiki/Time_zone for a list of the GMT/UTC timezone offsets
    */
    config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
    config.time.gmt_offset = 10;
    config.time.day_light_offset = 0;
}

void WebController::SmtpCallback(SMTP_Status status)
{
    /* Print the current status */
    Serial.println(status.info());

    /* Print the sending result */
    if (status.success())
    {
        // MailClient.printf used in the examples is for format printing via debug Serial port
        // that works for all supported Arduino platform SDKs e.g. SAMD, ESP32 and ESP8266.
        // In ESP8266 and ESP32, you can use Serial.printf directly.

        Serial.println("----------------");
        MailClient.printf("Message sent success: %d\n", status.completedCount());
        MailClient.printf("Message sent failed: %d\n", status.failedCount());
        Serial.println("----------------\n");

        for (size_t i = 0; i < smtp.sendingResult.size(); i++)
        {
            /* Get the result item */
            SMTP_Result result = smtp.sendingResult.getItem(i);

            // In case, ESP32, ESP8266 and SAMD device, the timestamp get from result.timestamp should be valid if
            // your device time was synched with NTP server.
            // Other devices may show invalid timestamp as the device time was not set i.e. it will show Jan 1, 1970.
            // You can call smtp.setSystemTime(xxx) to set device time manually. Where xxx is timestamp (seconds since Jan 1, 1970)

            MailClient.printf("Message No: %d\n", i + 1);
            MailClient.printf("Status: %s\n", result.completed ? "success" : "failed");
            MailClient.printf("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
            MailClient.printf("Recipient: %s\n", result.recipients.c_str());
            MailClient.printf("Subject: %s\n", result.subject.c_str());
        }
        Serial.println("----------------\n");

        // You need to clear sending result as the memory usage will grow up.
        smtp.sendingResult.clear();
    }
}

void WebController::Alert(const char *alert)
{
   /* Declare the message class */
    SMTP_Message message;

    /* Set the message headers */
    message.sender.name = F("ESP Mail");
    message.sender.email = AUTHOR_EMAIL;
    message.subject = F("Irrigation Controller Alert");
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
    else
    {
        if (smtp.isAuthenticated())
            Serial.println("Successfully logged in.");
        else
            Serial.println("Connected with no Auth.");
    }

    /* Start sending Email and close the session */
    if (!MailClient.sendMail(&smtp, &message))
        MailClient.printf("Error, Status Code: %d, Error Code: %d, Reason: %s\n", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
}
