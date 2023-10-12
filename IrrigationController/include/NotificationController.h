#ifndef NOTIFICATIONCONTROLLER_H
#define NOTIFICATIONCONTROLLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include <LiquidCrystal_I2C.h>

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT esp_mail_smtp_port_587

/* The log in credentials */
#define AUTHOR_EMAIL "borisqbf@gmail.com"
#define AUTHOR_PASSWORD "wxumeztmydfrakhd"
#define RECIPIENT_EMAIL "boris_qbf@hotmail.com"

class NotificationController
{

public:
    void Setup();
    void ProcessMainLoop();
    void Alert(const char *message);
    void Display(const char *line1, const char *line2);
    static NotificationController *GetInstance();

private:
    NotificationController();
    static NotificationController *theInstance;

    /* Callback function to get the Email sending status */
    static void SmtpCallback(SMTP_Status status);
    static Session_Config config;
    static SMTPSession smtp;

    static LiquidCrystal_I2C lcd;
};

#endif