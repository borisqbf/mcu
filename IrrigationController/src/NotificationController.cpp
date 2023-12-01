#include "NotificationController.h"

// Static members
NotificationController *NotificationController::theInstance = NULL;
Session_Config NotificationController::config;
SMTPSession NotificationController::smtp;
LiquidCrystal_I2C NotificationController::lcd(0x27, 16, 2);

NotificationController::NotificationController()
{
    lcd.init();
    lcd.clear();
    lcd.backlight();
    lcd.home();
}

NotificationController *NotificationController::GetInstance()
{
    if (theInstance == NULL)
    {
        theInstance = new NotificationController();

        // returning the instance pointer
        return theInstance;
    }
    else
    {
        return theInstance;
    }
}


void NotificationController::Setup()
{
    MailClient.networkReconnect(true);

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
    config.time.day_light_offset = 1;
}

void NotificationController::ProcessMainLoop()
{
}

void NotificationController::SmtpCallback(SMTP_Status status)
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

void NotificationController::Alert(const char *alert)
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

void NotificationController::Display(const char *line1, const char *line2)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
    delay(100);
}
