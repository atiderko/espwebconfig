if (typeof jsons === "undefined") {
  var jsons = [];
}
//jsons.unshift(["/menu", "menu"]);
jsons.push(["/menu", "menu"]);

let countGetWifiStations = 0;
let countGetWifiState = 0;
let running_json = false;
console.log('load json files: ' + jsons);
loadJson();

function loadJson() {
  console.log('loadJson, running_json: ' + running_json);
  if (running_json) {
    return;
  }
  let tuple = jsons.pop();
  console.log('loadJson, tuple: ' + tuple);
  if (tuple != undefined) {
    running_json = true;
    //for (var i = 0; i < jsons.length; i++) {
    let url = tuple[0];
    let func = tuple[1];
    let request = new XMLHttpRequest();
    request.open("GET",  tuple[0]);
    request.setRequestHeader('Cache-Control', 'no-cache');
    request.onreadystatechange = function() {
      try {
        if(request.readyState === XMLHttpRequest.DONE && request.status === 200) {
          console.log('--> got ' + tuple[0]);
          console.log('  data:' + request.responseText);
          var data = JSON.parse(request.responseText);
          window[func](data, url);
        }
      } catch(e) {
        console.error(e)
      }
    }
    request.send();
    running_json = false;
    loadJson();
  }
}

function menu(data, uri) {
  console.log("load header");
  hh = '<header id="lb" class="lb-fixed">';
  hh += '<input type="checkbox" class="lb-cb" id="lb-cb"/>';
  hh += '<div class="lb-menu lb-menu-right lb-menu-material">';
  hh += '  <ul class="lb-navigation">';
  hh += '    <li class="lb-header">';
  hh += '      <a href="/" class="lb-brand">' + data["brand"] + '</a>';
  hh += '      <label class="lb-burger lb-burger-dblspin" id="lb-burger" for="lb-cb"><span></span></label>';
  hh += '    </li>'
  elements = data["elements"];
  for (i = 0; i < elements.length; i++) {
    hh += '    <li class="lb-item" id="' + elements[i]["id"] + '"><a href="' + elements[i]["href"] + '">' + elements[i]["name"] + '</a></li>';
  }
  hh += '  </ul>';
  hh += '</div>';
  hh += '<div class="lap" id="rdlg"><a href="#reset" class="overlap"></a>';
  hh += '    <div class="modal_button"><h2><a href="/reset" class="modal_button">RESET</a></h2></div>';
  hh += '</div>';
  hh += '</header>';
  console.log(hh);
  document.getElementById("header").innerHTML = hh;
}

function replaceInfo(data) {
  console.log("load config");
  document.getElementById("title").innerHTML = data["name"];
  document.getElementById("version").innerHTML = data["version"];
}

function wifistations(data, uri) {
  console.log("add stations", uri);
  if (data["failed"]) {
    document.getElementById("list_ssid").innerHTML = '<div style="color:red;">scan for WiFi stations failed!</div>';
    document.getElementById("list_ssid_info").innerText = "";
    return;
  }
  if (! data["finished"]) {
    countGetWifiStations += 1;
    setTimeout(getJSON.bind(null, "/wifi/stations.json", "wifistations"), 1000); //Ruft die Callback-Funktion nach 1 Sekunde auf
    document.getElementById("list_ssid").innerText = "scan in progress..." + countGetWifiStations;
    document.getElementById("list_ssid_info").innerText = "";
    return;
  }
  networks = data["networks"];
  var hiddenCount = 0;
  hh = "";
  for (i = 0; i < networks.length; i++) {
    network = networks[i];
    hh += '<input type="button" onClick="onFocus(this.getAttribute(\'value\'))" value="' + network["ssid"] + '">';
    hh += '<label class="slist">' + getRSSIasQuality(network["rssi"]) + '&#037;&ensp;Ch.' + network["channel"] + '</label>';
    if (network["encrypted"]) {
      hh += '<span class="img-lock"></span>';
    }
    if (network["hidden"]) {
      hh += '<span class="img-hidden"></span>';
      hiddenCount += 1;
    }
    hh += '<br>';
    // this is for pages
    //hh += '<button type="submit" name="page" value="' + pageNr + '" formaction=" AUTOCONNECT_URI_CONFIG ">' + page + '</button>&emsp;';
  }

  console.log(hh);
  document.getElementById("list_ssid").innerHTML = hh;
  document.getElementById("list_ssid_info").innerText = "Total: " + networks.length + " Hidden: " + hiddenCount;
}

function getRSSIasQuality(rssi) {
  let quality = 0;
  if (rssi <= -100) {
    quality = 0;
  } else if (rssi >= -50) {
    quality = 100;
  } else {
    quality = 2 * (rssi + 100);
  }
  return quality;
}

function getJSON(uri, callback) {
  console.log("getJSON add " + callback + " uri: " + uri);
  jsons.push([uri, callback]);
  loadJson();
  // let request = new XMLHttpRequest();
  // request.open("GET",  uri);
  // request.setRequestHeader('Cache-Control', 'no-cache');
  // request.onreadystatechange = function() {
  //     if(request.readyState === XMLHttpRequest.DONE) {

  //       if (request.status === 200) {
  //         var data = JSON.parse(request.responseText);
  //         callback(data, uri);
  //       } else {
  //         callback(JSON.parse("{}"), uri);
  //       }
  //     }
  // }
  // request.send();
}

function wifistate(data, uri) {
  console.log("load wifi state " + uri);
  hh = '<label style="color:black;">' + data["ssid"] + ': </label>';
  if (data["ssid"].length > 0) {
    if (data["connected"]) {
      hh += '<label style="color:green;">connected </label>';
      hh += '<input id="btn_disconnect" type="button" style="padding:3px; width:9em; background: #FF8C00;" onClick="location.href=\'/wifi/disconnect\'" value="disconnect">';
      dbl = document.getElementById("dbl");
      if (dbl != null) {
        dbl.remove();
      }
      if (document.getElementById("list_ssid") == null) {
        setTimeout(redirect.bind(null, "/ewc/info"), 2000);
      }
    } else if (data["failed"]) {
      hh += '<label style="color:red;">' + data["reason"] + '</label>';
      dbl = document.getElementById("dbl");
      if (dbl != null) {
        dbl.remove();
      }
      if (document.getElementById("list_ssid") == null) {
        setTimeout(redirect.bind(null, "/ewc/info"), 2000);
      }
    } else if (data["ssid"].length > 0) {
      countGetWifiState += 1;
      hh += '<label> connecting...' + countGetWifiState + '</label>';
      setTimeout(getJSON.bind(null, "/wifi/state.json", "wifistate"), 1000);
    }
  } else {
    hh = "not connected";
  }
  document.getElementById("ssid_current").innerHTML = hh;
}

function redirect(url) {
  location.href = url;
}
