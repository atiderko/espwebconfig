/**************************************************************

This file is a part of
https://github.com/atiderko/espwebconfig

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
#ifndef EWC_PAGES_H
#define EWC_PAGES_H

#include "Arduino.h"

namespace EWC {


/** A response page for success states **/
const char  EWC_PAGE_SUCCESS[] PROGMEM = R"(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta name="viewport" charset="UTF-8" content="width=device-width, initial-scale=1, user-scalable=yes">
    <meta http-equiv="refresh" content = "{{TIMEOUT}};url={{REDIRECT}}">
    <title>{{TITLE}}</title>
    <link rel="stylesheet" href="/css/base.css">
  </head>
  <body style="padding-top:58px;">
    <div id="header"></div>
    <div style="position:absolute;left:-100%;right:-100%;text-align:center;margin:10px auto;font-weight:bold;color:#008000;">
        {{SUMMARY}}
    </div>
    <br>
    <div style="text-align:center;padding:10px 10px 10px 10px">
        {{DETAILS}}
    </div>
    <script src="/js/postload.js"></script>
  </body>
</html>
)";


/** A response page for failed states **/
const char  EWC_PAGE_FAIL[] PROGMEM = R"(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta name="viewport" charset="UTF-8" content="width=device-width, initial-scale=1, user-scalable=yes">
    <meta http-equiv="refresh" content = "{{TIMEOUT}};url={{REDIRECT}}">
    <title>{{TITLE}}</title>
    <link rel="stylesheet" href="/css/base.css">
  </head>
  <body style="padding-top:58px;">
    <div id="header"></div>
    <div style="position:absolute;left:-100%;right:-100%;text-align:center;margin:10px auto;font-weight:bold;color:#c50000;">
        {{SUMMARY}}
    </div>
    <br>
    <div style="text-align:center;padding:10px 10px 10px 10px">
        {{DETAILS}}
    </div>
    <script src="/js/postload.js"></script>
  </body>
</html>
)";

};

#endif
