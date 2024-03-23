// load json files on load this script
if (typeof jsons === "undefined") {
  var jsons = [];
}
jsons.push(["/menu.json", "menu"]);

let language = "en";
let langmap = {};
let cjson_url = undefined;
console.log("load json files: " + jsons);
_loadJson();

// use this getJSON to invoke the load of extended info
function getJSON(uri, callback) {
  console.log("getJSON add " + callback + " uri: " + uri);
  jsons.push([uri, callback]);
  _loadJson();
}

function _loadJson() {
  if (cjson_url === undefined) {
    let tuple = jsons.pop();
    if (tuple != undefined) {
      console.log("_loadJson, tuple: " + tuple);
      let url = tuple[0];
      cjson_url = url;
      let func = tuple[1];
      let request = new XMLHttpRequest();
      request.open("GET", tuple[0]);
      request.setRequestHeader("Cache-Control", "no-cache");
      request.overrideMimeType("application/json; charset=UTF-8");
      request.onreadystatechange = function () {
        try {
          if (request.readyState === XMLHttpRequest.DONE) {
            if (request.status === 200) {
              console.log("--> got " + tuple[0]);
              console.log("  data:" + request.responseText);
              var data = JSON.parse(request.responseText);
              window[func](data, url);
            }
            cjson_url = undefined;
            _loadJson();
          }
        } catch (e) {
          console.error(e);
          cjson_url = undefined;
          _loadJson();
        }
      };
      request.send();
    }
  }
}

function menu(data, uri) {
  console.log("load header");
  hh = '<header id="lb" class="lb-fixed">';
  hh += '<input type="checkbox" class="lb-cb" id="lb-cb"/>';
  hh += '<div class="lb-menu lb-menu-right lb-menu-material">';
  hh += '  <ul class="lb-navigation">';
  hh += '    <li class="lb-header">';
  hh +=
    '      <a href="' +
    data["brandUri"] +
    '" class="lb-brand">' +
    data["brand"] +
    "</a>";
  hh +=
    '      <label class="lb-burger lb-burger-dblspin" id="lb-burger" for="lb-cb"><span></span></label>';
  hh += "    </li>";
  elements = data["elements"];
  for (i = 0; i < elements.length; i++) {
    hh +=
      '    <li class="lb-item"><a href="' +
      elements[i]["href"] +
      '" id="' +
      elements[i]["id"] +
      '">' +
      elements[i]["name"] +
      "</a></li>";
  }
  hh += "  </ul>";
  hh += "</div>";
  hh += "</header>";
  console.log(hh);
  document.getElementById("header").innerHTML = hh;
  language = data["language"];
  if (language != "en") {
    getJSON("/languages.json", "applyLanguage");
  }
}

function redirect(url) {
  location.href = url;
}

/** merge the loaded JSON with launguage data to local map and replaces all elements with id in the map. **/
function applyLanguage(data, uri) {
  for (var k in data) {
    langmap[k] = data[k];
    updateLanguageKey(k);
  }
}

/** Replaces the text of one documentElement with given id by current language. **/
function updateLanguageKey(id) {
  element = langmap[id];
  if (element != undefined) {
    value = element[language];
    if (value != undefined) {
      item = document.getElementById(id);
      if (item != undefined) {
        // console.log(id, ":", value);
        // console.log(id, ":", item.tagName);
        // console.log(id, ":", item.text);
        if (item.tagName.toUpperCase() == "BUTTON") {
          if (item.innerHTML) {
            item.innerHTML = value;
          }
        } else if (item.tagName.toUpperCase() == "INPUT") {
          if (item.value) {
            item.value = value;
          }
        } else {
          if (item.innerHTML) {
            item.innerHTML = value;
          }
        }
      }
    }
  }
}

/** Searches for given ids in the launguage map and replaces with current language. **/
function updateLanguageKeys(ids) {
  for (var idx in ids) {
    updateLanguageKey(ids[idx]);
  }
}
