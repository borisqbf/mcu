#include "WebController.h"

static WebController theInstance;

const char *absLevelURL = "/api/level-abs";

void HandleRoot()
{
    String message = "Hello resolved by mDNS!\n\n";
    message += "Available routes are ";
    message += absLevelURL;
    message += " and ";
    message += absLevelURL;

    server->send(200, "text/plain", message);
}

void HandleNotFound()
{
    String message = "File Not Found\n\n";
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
    message += "\nAvailable routes are ";
    message += absLevelURL;
    message += " and ";
    message += absLevelURL;

    server->send(404, "text/plain", message);
}

void GetLevelPercentage()
{
    server->send(200, "text/plain", String(waterLevelPercentage, 2).c_str());
}

WebController *WebController::GetInstance()
{
    return &theInstance;
}

void WebController::SetOnAction(IrrigationController *controllerInstance, ValveActionFn action)
{
    actionController = controllerInstance;
    onAction = action;
}

void WebController::SetOffAction(IrrigationController *controllerInstance, ValveActionFn action)
{
    actionController = controllerInstance;
    offAction = action;
}

void WebController::SetResetAction(IrrigationController *controllerInstance, ValveActionFn action)
{
    actionController = controllerInstance;
    resetAction = action;
}

WebController::WebController()
{
    server = new WebServer(80);
}

void WebController::Setup()
{
    if (MDNS.begin("rain-tank-level"))
    { // Start mDNS

        Serial.println("MDNS started");
    }

    server->on("/", HandleRoot);                // Associate handler function to path
    server->on(absLevelURL, GetLevelAbsolute);  // Associate handler function to path
    server->onNotFound(HandleNotFound);

    server->begin(); // Start server
    Serial.println("HTTP server started");
}

void WebController::ProcessMainLoop()
{
    server->handleClient();
    MDNS.update();
}

void WebController::SendHttpResponse(WiFiClient client)
{
    // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
    // and a content-type so the client knows what's coming, then a blank line:
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println();

    // the content of the HTTP response follows the header:
    client.print("Current time is ");
    char message[50];
    Chronos::DateTime n = Chronos::DateTime::now();
    sprintf(message, "%02u/%02u/%u %02u:%02u. ", n.day(), n.month(), n.year(), n.hour(), n.minute());
    client.println(message);
    client.print("Current state is ");
    client.println(actionController->GetCurrentState());
    client.print("<br>Current flow is ");
    client.println(actionController->GetWaterFlow());

    // The HTTP response ends with another blank line:

    client.println();
}
