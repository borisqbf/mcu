#include "WebController.h"

static WebController theInstance;

WebController *WebController::GetInstance()
{
    return &theInstance;
}

WebController::SetOnAction(IrrigationController *controllerInstance, ValveActionFn action)
{
    actionController = controllerInstance;
    OnAction = action;
}

WebController::SetOffAction(IrrigationController *controllerInstance, ValveActionFn action)
{
    actionController = controllerInstance;
    OffAction = action;
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
    RingBuffer buf(8);
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
                Serial.print(c);
                buf.push(c);            // push it to the ring buffer

                // you got two newline characters in a row
                // that's the end of the HTTP request, so send a response
                if (buf.endsWith("\r\n\r\n"))
                {
                    Serial.println("Sending Responce");
                    SendHttpResponse(client);
                    break;
                }

                // Check to see if the client request was "GET /ON" or "GET /OFF":
                if (buf.endsWith("GET /ON"))
                {
                    Serial.println("Turn valve ON");
                    if (actionController != NULL && OnAction != NULL)
                        (actionController->*OnAction)();
                }
                else if (buf.endsWith("GET /OFF"))
                {
                    if (actionController != NULL && OffAction != NULL)
                        (actionController->*OffAction)();
                }
            }
        }

        // close the connection
        client.stop();
        Serial.println("Client disconnected");
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
    client.print("OK");

    // The HTTP response ends with another blank line:
    client.println();
}
