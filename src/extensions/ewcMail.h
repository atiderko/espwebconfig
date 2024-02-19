/**************************************************************

This file is a part of BalkonBewaesserungsSteuerung
https://github.com/atiderko/bbs

Copyright [2020] Alexander Tiderko

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

**************************************************************/
#ifndef EWC_MAIL_H
#define EWC_MAIL_H

#include <Arduino.h>
#include <ewcConfigServer.h>
#ifdef ESP8266
    #include <ESP8266WiFi.h>
#elif defined(ESP32)
    #include <WiFi.h>
    #include <vector>
#endif
#include <Base64.h>

namespace EWC {
    #define MAIL_SEND_TIMEOUT 30000 // timeout beim Senden von emails

    const char DEFAULT_MAIL_SMTP[] = "";
    const uint16_t DEFAULT_MAIL_SMTP_PORT = 25;
    const char DEFAULT_MAIL_SENDER[] = "";
    const char DEFAULT_MAIL_SENDER_PW[] = "";
    const char DEFAULT_MAIL_RECEIVER[] = "";

enum MailStates {
    MAIL_SOMEONE = 0,
    MAIL_ON_WARNING = 1,
    MAIL_ON_CHANGE = 2,
    MAIL_ON_EVENT = 3
};

class MailConfig {
public:
    String hello;
    String auth;
    String from;
    String to;
    MailConfig() {}
    MailConfig(String smtpServer, String sender, String password, String receiver)
    {
        hello = "EHLO " + smtpServer + "\n";
        auth = "AUTH LOGIN\n" + base64::encode(sender) + "\n" + base64::encode(password) + "\n";
        from = "MAIL FROM: <" + sender + ">\r\n";
        to = "RCPT TO: <" + receiver + ">\r\n";
    }
};

class Mail : public EWC::ConfigInterface {
public:
    Mail();
    ~Mail();
    /** === ConfigInterface Methods === **/
    void setup(JsonDocument& config, bool resetConfig=false);
    void fillJson(JsonDocument& config);

    /** === OWN Methods === **/
    void loop();

    /** Checks if for given id mail send is enabled. **/
    bool enabled(MailStates id=MAIL_SOMEONE);
    bool sendWarning(const char* subject, const char* body);
    bool sendChange(const char* subject, const char* body);
    bool sendEvent(const char* subject, const char* body);
    /** Checks if for all mails send a response from server was received. **/
    bool sendFinished();

protected:
    WiFiClient _wifiClient;
    unsigned long _tsSendMail;
    unsigned int _countSend;
    bool _testMailSend;
    bool _testMailSuccess;
    String _testMailResult;
    MailConfig _mailConfig;
    String _mailData;

    // mail configuration parameter
    boolean _mailOnWarning;
    boolean _mailOnChange;
    boolean _mailOnEvent;
    String _mailServer;
    uint16_t _mailPort;
    String _mailSender;
    String _mailPassword;
    String _mailReceiver;

    void _onMailConfig(WebServer* webserver);
    void _onMailState(WebServer* webserver);
    void _onMailSave(WebServer* webserver, bool sendResponse=true);
    void _onMailTest(WebServer* webserver);
    bool _send(const char* betreff, const char* body);
    void _setTestResult(bool success, const char* result);
    void _fromJson(JsonDocument& config);
};
};
#endif
