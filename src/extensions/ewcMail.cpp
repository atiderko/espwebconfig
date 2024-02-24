/**************************************************************

This file is a part of
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
#include "ewcTime.h"
#include <ewcRTC.h>
#include <ewcConfigServer.h>
#include <vector>
#include "generated/mailSetupHTML.h"
#include "generated/mailStateHTML.h"

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

void Mail::setup(JsonDocument &config, bool resetConfig)
{
  // email settings
  _fromJson(config);
  WebServer *ws = &EWC::I::get().server().webServer();
  EWC::I::get().server().insertMenuG("Mail", "/mail/setup", "menu_mail", FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_MAIL_SETUP_GZIP, sizeof(HTML_MAIL_SETUP_GZIP), true, 0);
  EWC::I::get().server().webServer().on("/mail/config.json", std::bind(&Mail::_onMailConfig, this, ws));
  EWC::I::get().server().webServer().on("/mail/config/save", std::bind(&Mail::_onMailSave, this, ws, true));
  EWC::I::get().server().webServer().on("/mail/test", std::bind(&Mail::_onMailTest, this, ws));
  EWC::I::get().server().webServer().on("/mail/state.html", std::bind(&ConfigServer::sendContentG, &EWC::I::get().server(), ws, FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_MAIL_STATE_GZIP, sizeof(HTML_MAIL_STATE_GZIP)));
  EWC::I::get().server().webServer().on("/mail/state.json", std::bind(&Mail::_onMailState, this, ws));
}

void Mail::loop()
{
  if (!enabled() && !I::get().server().isConnected())
    return;

  if (_wifiClient.available())
  {
    while (_wifiClient.available())
    {
      String line = _wifiClient.readStringUntil('\n');
      EWC::I::get().logger() << F("Mail response: ") << line << endl;
      if (line.indexOf("Bye") > -1)
      { // if reply from server contains "Bye" send was successful
        EWC::I::get().logger() << F("******* mail sent *******") << endl;
        _countSend--;
        if (_countSend == 0)
        {
          _setTestResult(true, "Successfully!");
          _wifiClient.stop();
        }
      }
      else if (line.indexOf("Error") > -1)
      {
        _setTestResult(false, line.c_str());
        _wifiClient.stop();
        _tsSendMail = 0;
        _countSend = 0;
        _mailData = "";
      }
      else if (line.startsWith("220"))
      {
        // HELLO accepted send Authentication
        _wifiClient.print(_mailConfig.auth);
      }
      else if (line.startsWith("235"))
      {
        // Authenticated, send from/to params
        _wifiClient.print(_mailConfig.from);
        _wifiClient.print(_mailConfig.to);
      }
      else if (line.startsWith("250 2.1.5"))
      {
        // To accepted, send request for DATA
        _wifiClient.print("DATA\r\n");
      }
      else if (line.startsWith("354"))
      {
        // DATA accepted, send mail data
        _wifiClient.print(_mailData);
      }
      else if (line.startsWith("250 2.0.0"))
      {
        // DATA ok, quit
        _wifiClient.print("QUIT\r\n");
      }
      else if (line.startsWith("221"))
      {
        // Mail send OK
        _mailData = "";
      }
      else
      {
        EWC::I::get().logger() << F("Mail not handled response: ") << line << endl;
      }
    }
  }
  else
  {
    unsigned long wts = millis() - _tsSendMail;
    if (wts > MAIL_SEND_TIMEOUT)
    {
      _setTestResult(false, "Timeout while receiving ACK");
      // reset after timeout
      _tsSendMail = 0;
      _countSend = 0;
    }
  }
}

void Mail::fillJson(JsonDocument &config)
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

void Mail::_fromJson(JsonDocument &config)
{
  JsonVariant jv = config["mail"]["on_warning"];
  if (!jv.isNull())
  {
    _mailOnWarning = jv.as<bool>();
  }
  jv = config["mail"]["on_change"];
  if (!jv.isNull())
  {
    _mailOnChange = jv.as<bool>();
  }
  jv = config["mail"]["on_event"];
  if (!jv.isNull())
  {
    _mailOnEvent = jv.as<bool>();
  }
  jv = config["mail"]["smtp"];
  if (!jv.isNull())
  {
    _mailServer = jv.as<String>();
  }
  jv = config["mail"]["port"];
  if (!jv.isNull())
  {
    _mailPort = jv.as<int>();
  }
  jv = config["mail"]["sender"];
  if (!jv.isNull())
  {
    _mailSender = jv.as<String>();
  }
  jv = config["mail"]["passphrase"];
  if (!jv.isNull())
  {
    _mailPassword = jv.as<String>();
  }
  jv = config["mail"]["receiver"];
  if (!jv.isNull())
  {
    _mailReceiver = jv.as<String>();
  }
  _mailConfig = MailConfig(_mailServer, _mailSender, _mailPassword, _mailReceiver);
}

void Mail::_onMailConfig(WebServer *webServer)
{
  I::get().logger() << F("[Mail] config request") << endl;
  if (!I::get().server().isAuthenticated(webServer))
  {
    I::get().logger() << F("[Mail] not sufficient authentication") << endl;
    return webServer->requestAuthentication();
  }
  JsonDocument jsonDoc;
  fillJson(jsonDoc);
  String output;
  serializeJson(jsonDoc, output);
  webServer->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void Mail::_onMailState(WebServer *webServer)
{
  I::get().logger() << F("[Mail] state request") << endl;
  if (!I::get().server().isAuthenticated(webServer))
  {
    I::get().logger() << F("[Mail] not sufficient authentication") << endl;
    return webServer->requestAuthentication();
  }
  JsonDocument jsonDoc;
  jsonDoc["mail"]["test_send"] = _testMailSend;
  jsonDoc["mail"]["test_success"] = _testMailSuccess;
  jsonDoc["mail"]["test_result"] = _testMailResult;
  jsonDoc["mail"]["sending"] = _countSend > 0;
  String output;
  serializeJson(jsonDoc, output);
  webServer->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void Mail::_onMailSave(WebServer *webServer, bool sendResponse)
{
  I::get().logger() << F("[Mail] config save request") << endl;
  if (!I::get().server().isAuthenticated(webServer))
  {
    I::get().logger() << F("[Mail] not sufficient authentication") << endl;
    return webServer->requestAuthentication();
  }
  JsonDocument config;
  if (webServer->hasArg("on_warning"))
  {
    config["mail"]["on_warning"] = webServer->arg("on_warning").equals("true");
  }
  if (webServer->hasArg("on_change"))
  {
    config["mail"]["on_change"] = webServer->arg("on_change").equals("true");
  }
  if (webServer->hasArg("on_event"))
  {
    config["mail"]["on_event"] = webServer->arg("on_event").equals("true");
  }
  if (webServer->hasArg("smtp") && !webServer->arg("smtp").isEmpty())
  {
    config["mail"]["smtp"] = webServer->arg("smtp");
  }
  if (webServer->hasArg("port"))
  {
    long val = webServer->arg("port").toInt();
    if (val > 0)
    {
      config["mail"]["port"] = val;
    }
  }
  if (webServer->hasArg("sender") && !webServer->arg("sender").isEmpty())
  {
    config["mail"]["sender"] = webServer->arg("sender");
  }
  if (webServer->hasArg("passphrase") && !webServer->arg("passphrase").isEmpty())
  {
    config["mail"]["passphrase"] = webServer->arg("passphrase");
  }
  if (webServer->hasArg("receiver") && !webServer->arg("receiver").isEmpty())
  {
    config["mail"]["receiver"] = webServer->arg("receiver");
  }
  _fromJson(config);
  I::get().configFS().save();
  if (sendResponse)
  {
    String details;
    serializeJsonPretty(config["mail"], details);
    I::get().server().sendPageSuccess(webServer, "BBS Mail save", "Save successful!", "/mail/setup", "<pre id=\"json\">" + details + "</pre>", "Back", "/mail/test", "Send Test Mail");
  }
}

void Mail::_onMailTest(WebServer *webServer)
{
  _onMailSave(webServer, false);
  I::get().logger() << F("[Mail]: send test mail...") << endl;
  String body = String("This is a test mail from your ") + I::get().config().paramDeviceName;
  if (_send("Test Mail", body.c_str()))
  {
    _testMailSend = true;
    webServer->sendHeader("Location", F("/mail/state.html"));
    webServer->send(302, "text/plain", "");
  }
  else
  {
    I::get().logger() << F("[Mail]: send test mail failed") << endl;
    I::get().server().sendPageFailed(webServer, F("Test Mail Result"), F("Send test mail failed, one mail already in queue!"), F("/mail/setup"));
  }
}

bool Mail::enabled(MailStates id)
{
  switch (id)
  {
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

bool Mail::sendWarning(const char *subject, const char *body)
{
  if (_mailOnWarning)
  {
    if (_send(subject, body))
    {
      return true;
    }
  }
  return false;
}

bool Mail::sendChange(const char *subject, const char *body)
{
  if (_mailOnChange)
  {
    if (_send(subject, body))
    {
      return true;
    }
  }
  return false;
}

bool Mail::sendEvent(const char *subject, const char *body)
{
  if (_mailOnEvent)
  {
    if (_send(subject, body))
    {
      return true;
    }
  }
  return false;
}

void Mail::_setTestResult(bool success, const char *result)
{
  if (_testMailSend)
  {
    _testMailSuccess = success;
    _testMailResult = result;
    _testMailSend = false;
  }
}

bool Mail::_send(const char *subject, const char *body)
{
#ifdef ESP8266
  I::get().logger() << "[Mail]: _send status: " << String(_wifiClient.status()) << endl;
#else
#endif
  if (_mailConfig.hello.length() == 0)
  {
    I::get().logger() << F("[Mail]: not configured, skip mail with subject: ") << subject << endl;
    _setTestResult(false, "Mail not configured");
    return false;
  }
  if (_mailData.length() > 0)
  {
    _setTestResult(false, "Mail already sending");
    I::get().logger() << F("[Mail]: already sending, skip mail with subject: ") << subject << endl;
    return false;
  }
  EWC::I::get().logger() << F("[Mail] connect to the server: ") << _mailServer << F(":") << _mailPort << endl;
  if (!_wifiClient.connect(_mailServer.c_str(), _mailPort))
  {
    _setTestResult(false, "Send mail failed");
    I::get().logger() << F("[Mail]: connect failed") << endl;
    EWC::I::get().logger() << F("✖ Connection to the server: ") << _mailServer << F(" with port: ") << _mailPort << F(" failed") << endl;
    return false;
  }
  else
  {
    EWC::I::get().logger() << F("✔ connected to the server: ") << _mailServer << F(":") << _mailPort << endl;
    _tsSendMail = millis();
    _countSend = 1;
    I::get().logger() << F("[Mail]: connected, send...") << endl;
    _wifiClient.print(_mailConfig.hello);
    // create DATA string
    String data;
    data += "From: " + I::get().config().paramDeviceName + " <" + _mailSender + ">\r\n";
    data += "To: <" + _mailReceiver + ">\r\n";
    data += "Subject: " + I::get().config().paramDeviceName + " - " + subject + "\r\n";
    data += "Date: " + EWC::I::get().time().str() + "\r\n";
    data += "" + String(body) + "\r\n.\r\n";
    _mailData = data;
    return true;
  }
  return false;
}

bool Mail::sendFinished()
{
  return _countSend == 0;
}
