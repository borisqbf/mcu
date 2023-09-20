#include "WebController.h"

static WebController theInstance;
RingBuffer buf(8);

WebController *WebController::GetInstance()
{
    return &theInstance;
}

WebController::SetOnAction(IrrigationController *controllerInstance, ValveActionFn action)
{
    actionController = controllerInstance;
    onAction = action;
}

WebController::SetOffAction(IrrigationController *controllerInstance, ValveActionFn action)
{
    actionController = controllerInstance;
    offAction = action;
}

WebController::SetResetAction(IrrigationController *controllerInstance, ValveActionFn action)
{
    actionController = controllerInstance;
    resetAction = action;
}

WebController::WebController()
{
    server = new WiFiEspServer(80);
}

void WebController::Setup()
{
    server->begin();
    Serial.println("HTTP server started");
}

void WebController::ProcessMainLoop()
{
    currentAction = NULL;
    WiFiEspClient client = server->available(); // listen for incoming clients
    if (client)
    {                                 // if you get a client,
        Serial.println("New client"); // print a message out the serial port
        buf.init();                   // initialize the circular buffer

        while (client.connected())
        { // loop while the client's connected
            if (client.available())
            {                           // if there's bytes to read from the client,
                char c = client.read(); // read a byte, then
                buf.push(c);            // push it to the ring buffer

                // you got two newline characters in a row
                // that's the end of the HTTP request, so send a response
                if (buf.endsWith("\r\n\r\n"))
                {
                    Serial.println("Sending Response");
                    SendHttpResponse(client);
                    buf.init();
                    break;
                }

                // Check to see if the client request was "GET /ON" or "GET /OFF":
                if (buf.endsWith("GET /ON"))
                {
                    currentAction = onAction;
                }
                else if (buf.endsWith("GET /OFF"))
                {
                    currentAction = offAction;
                }
                else if (buf.endsWith("GET /RESET"))
                {
                    currentAction = resetAction;
                }
            }
        }

        // close the connection
        client.stop();
        Serial.println("Client disconnected");

        if (actionController != NULL && currentAction != NULL)
        {
            (actionController->*currentAction)();
        }
    }
}

void WebController::SendHttpResponse(WiFiEspClient client)
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
