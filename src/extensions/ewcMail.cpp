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
#include "ewcMail.h"
#include <ewcRTC.h>
#include <ewcConfigServer.h>
#include <Base64.h>
#include "generated/mailSetupHTML.h"
#include "generated//mailStateHTML.h"

using namespace EWC;

Mail::Mail()
    : ConfigInterface("mail")
{
    _tsSendMail = 0;
    _countSend = 0;
    _testMailSend = false;
    _testMailSuccess = false;
    _mailOnWarning = true;
    _mailOnChange = true;
    _mailOnEvent = false;
    _mailServer = DEFAULT_MAIL_SMTP;
    _mailPort = DEFAULT_MAIL_SMTP_PORT;
    _mailSender = DEFAULT_MAIL_SENDER;
    _mailPassword = DEFAULT_MAIL_SENDER_PW;
    _mailReceiver = DEFAULT_MAIL_RECEIVER;    
}

Mail::~Mail()
{
}

void Mail::setup(JsonDocument& config, bool resetConfig)
{
    // email settings
    _fromJson(config);
    WebServer* ws = &EWC::I::get().server().webserver();
    EWC::I::get().server().insertMenuG("Mail", "/mail/setup", "menu_mail", FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_MAIL_SETUP_GZIP, sizeof(HTML_MAIL_SETUP_GZIP), true, 0);
    EWC::I::get().server().webserver().on("/mail/config.json", std::bind(&Mail::_onMailConfig, this, ws));
    EWC::I::get().server().webserver().on("/mail/config/save", std::bind(&Mail::_onMailSave, this, ws, true));
    EWC::I::get().server().webserver().on("/mail/test", std::bind(&Mail::_onMailTest, this, ws));
    EWC::I::get().server().webserver().on("/mail/state.html", std::bind(&ConfigServer::sendContentG, &EWC::I::get().server(), ws, FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_MAIL_STATE_GZIP, sizeof(HTML_MAIL_STATE_GZIP)));
    EWC::I::get().server().webserver().on("/mail/state.json", std::bind(&Mail::_onMailState, this, ws));
}

void Mail::loop()
{
    if (!enabled() && !I::get().server().isConnected())
        return;

    if (!_subjectToSend.isEmpty()) {
        _send(_subjectToSend.c_str(), _bodyToSend.c_str());
        _subjectToSend = "";
        _bodyToSend = "";
    }

    if (_wifiClient.available()) {
        while (_wifiClient.available()) {
            String line = _wifiClient.readStringUntil('\n');
            EWC::I::get().logger() << F("Mail response: ") << line << endl;
            if (line.indexOf("Bye") > -1) {    // wenn Zeile der Antwort "Bye" enthaelt war das Senden erfolgreich
                EWC::I::get().logger() << F("******* mail sent *******") << endl;
                _countSend--;
                if (_countSend == 0) {
                    _setTestResult(true, "Successfully!");
                    _wifiClient.stop();
                }
            }
        }
    } else {
        unsigned long wts = millis() - _tsSendMail;
        if (wts > MAIL_SEND_TIMEOUT) {
            _setTestResult(false, "Timeout while receiving ACK");
            // reset after timeout
            _tsSendMail = 0;
            _countSend = 0;
        }
    }
}

void Mail::fillJson(JsonDocument& config)
{
    config["mail"]["on_warning"] = _mailOnWarning;
    config["mail"]["on_change"] = _mailOnChange;
    config["mail"]["on_event"] = _mailOnEvent;
    config["mail"]["smtp"] = _mailServer;
    config["mail"]["port"] = _mailPort;
    config["mail"]["sender"] = _mailSender;
    config["mail"]["passphrase"] = _mailPassword;
    config["mail"]["receiver"] = _mailReceiver;
}

void Mail::_fromJson(JsonDocument& config)
{
    JsonVariant jv = config["mail"]["on_warning"];
    if (!jv.isNull()) {
        _mailOnWarning = jv.as<bool>();
    }
    jv = config["mail"]["on_change"];
    if (!jv.isNull()) {
        _mailOnChange = jv.as<bool>();
    }
    jv = config["mail"]["on_event"];
    if (!jv.isNull()) {
        _mailOnEvent = jv.as<bool>();
    }
    jv = config["mail"]["smtp"];
    if (!jv.isNull()) {
        _mailServer = jv.as<String>();
    }
    jv = config["mail"]["port"];
    if (!jv.isNull()) {
        _mailPort = jv.as<int>();
    }
    jv = config["mail"]["sender"];
    if (!jv.isNull()) {
        _mailSender = jv.as<String>();
    }
    jv = config["mail"]["passphrase"];
    if (!jv.isNull()) {
        _mailPassword = jv.as<String>();
    }
    jv = config["mail"]["receiver"];
    if (!jv.isNull()) {
        _mailReceiver = jv.as<String>();
    }
}

void Mail::_onMailConfig(WebServer* webserver)
{
    I::get().logger() << F("[Mail] config request") << endl;
    if (!I::get().server().isAuthenticated(webserver)) {
        I::get().logger() << F("[Mail] not sufficient authentication") << endl;
        return webserver->requestAuthentication();
    }
    DynamicJsonDocument jsonDoc(512);
    fillJson(jsonDoc);
    String output;
    serializeJson(jsonDoc, output);
    webserver->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void Mail::_onMailState(WebServer* webserver)
{
    I::get().logger() << F("[Mail] state request") << endl;
    if (!I::get().server().isAuthenticated(webserver)) {
        I::get().logger() << F("[Mail] not sufficient authentication") << endl;
        return webserver->requestAuthentication();
    }
    DynamicJsonDocument jsonDoc(512);
    jsonDoc["mail"]["test_send"] = _testMailSend;
    jsonDoc["mail"]["test_success"] = _testMailSuccess;
    jsonDoc["mail"]["test_result"] = _testMailResult;
    jsonDoc["mail"]["sending"] = _countSend > 0;
    String output;
    serializeJson(jsonDoc, output);
    webserver->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void Mail::_onMailSave(WebServer* webserver, bool sendResponse)
{
    I::get().logger() << F("[Mail] config save request") << endl;
    if (!I::get().server().isAuthenticated(webserver)) {
        I::get().logger() << F("[Mail] not sufficient authentication") << endl;
        return webserver->requestAuthentication();
    }
    DynamicJsonDocument config(1024);
    if (webserver->hasArg("on_warning")) {
        config["mail"]["on_warning"] = webserver->arg("on_warning").equals("true");
    }
    if (webserver->hasArg("on_change")) {
        config["mail"]["on_change"] = webserver->arg("on_change").equals("true");
    }
    if (webserver->hasArg("on_event")) {
        config["mail"]["on_event"] = webserver->arg("on_event").equals("true");
    }
    if (webserver->hasArg("smtp") && !webserver->arg("smtp").isEmpty()) {
        config["mail"]["smtp"] = webserver->arg("smtp");
    }
    if (webserver->hasArg("port")) {
        long val = webserver->arg("port").toInt();
        if (val > 0) {
            config["mail"]["port"] = val;
        }
    }
    if (webserver->hasArg("sender") && !webserver->arg("sender").isEmpty()) {
        config["mail"]["sender"] = webserver->arg("sender");
    }
    if (webserver->hasArg("passphrase") && !webserver->arg("passphrase").isEmpty()) {
        config["mail"]["passphrase"] = webserver->arg("passphrase");
    }
    if (webserver->hasArg("receiver") && !webserver->arg("receiver").isEmpty()) {
        config["mail"]["receiver"] = webserver->arg("receiver");
    }
    _fromJson(config);
    I::get().configFS().save();
    if (sendResponse) {
        String details;
        serializeJsonPretty(config["mail"], details);
        I::get().server().sendPageSuccess(webserver, "BBS Mail save", "Save successful!", "/mail/setup", "<pre id=\"json\">" + details + "</pre>", "Back", "/mail/test", "Send Test Mail");
    }
}

void Mail::_onMailTest(WebServer* webserver)
{
    _onMailSave(webserver, false);
    I::get().logger() << F("[Mail]: send test mail") << endl;
    if (_criticalSend("Test Mail", "This is a test mail from your BBS.")) {
        I::get().logger() << F("[Mail]: test mail ok") << endl;
        _testMailSend = true;
        webserver->sendHeader("Location", F("/mail/state.html"));
        webserver->send(302, "text/plain", "");
    } else {
        I::get().logger() << F("[Mail]: send test mail failed") << endl;
        I::get().server().sendPageFailed(webserver, F("Test Mail Result"), F("Send test mail failed, one mail already in queue!"), F("/mail/setup"));
    }
}


bool Mail::enabled(MailStates id)
{
    switch (id) {
        case MAIL_SOMEONE:
            return _mailOnWarning || _mailOnChange || _mailOnEvent;
        case MAIL_ON_WARNING:
            return _mailOnWarning;
        case MAIL_ON_CHANGE:
            return _mailOnChange;
        case MAIL_ON_EVENT:
            return _mailOnEvent;
    }
    return _mailOnWarning || _mailOnChange || _mailOnEvent;
}

bool Mail::sendWarning(const char* subject, const char* body)
{
    if (_mailOnWarning) {
        if (_send(subject, body)) {
            return true;
        }
    }
    return false;
}

bool Mail::sendChange(const char* subject, const char* body)
{
    if (_mailOnChange) {
        if (_send(subject, body)) {
            return true;
        }
    }
    return false;
}

bool Mail::sendEvent(const char* subject, const char* body)
{
    if (_mailOnEvent) {
        if (_send(subject, body)) {
            return true;
        }
    }
    return false;
}

void Mail::_setTestResult(bool success, const char* result)
{
    if (_testMailSend) {
        _testMailSuccess = success;
        _testMailResult = result;
        _testMailSend = false;
    }
}

bool Mail::_send(const char* subject, const char* body) {
#ifdef ESP8266
    I::get().logger() << "[Mail]: _send status: " << String(_wifiClient.status()) << endl;
#else
#endif
    if (!_wifiClient.connect(_mailServer.c_str(), _mailPort)) {
        _setTestResult(false, "Send mail failed");
        I::get().logger() << F("[Mail]: connect failed") << endl;
        EWC::I::get().logger() << F("✖ Connection to the server: ") << _mailServer << F(" with port: ") << _mailPort << F(" failed") << endl;
        return false;
    } else {
        _tsSendMail = millis();
        I::get().logger() << F("[Mail]: connected, send...") << endl;
        EWC::I::get().logger() << F("✔ connected to the server: ") << _mailServer << F(" with port: ") << _mailPort << endl;
        _wifiClient.printf("EHLO\nAUTH LOGIN\n");
        _wifiClient.printf("%s\n%s\n", base64::encode(_mailSender).c_str(), base64::encode(_mailPassword).c_str());
        _wifiClient.printf("MAIL From: %s\n", _mailSender.c_str());
        _wifiClient.printf("RCPT To: <%s>\n", _mailReceiver.c_str());
        _wifiClient.printf("DATA\n");
        _wifiClient.printf("To: <%s>\n", _mailReceiver.c_str());
        _wifiClient.printf("From: %s <%s>\n", I::get().config().paramDeviceName.c_str(), _mailSender.c_str());
        _wifiClient.printf("Subject:%s - %s\n", I::get().config().paramDeviceName.c_str(), subject);
        _wifiClient.println(body);
        _wifiClient.println(".\nQUIT");
        I::get().logger() << F("[Mail]: sent") << endl;
        _countSend++;
        return true;
    }
    return false;
}

bool Mail::sendFinished()
{
    return _countSend == 0;
}

bool Mail::_criticalSend(const char* subject, const char* body)
{
    if (_subjectToSend.isEmpty()) {
        _subjectToSend = subject;
        _bodyToSend = body;
        return true;
    }
    return false;
}
