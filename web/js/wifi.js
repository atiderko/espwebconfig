var timerInfo;
var timerSetup;
var timerState;
let countGetWifiStations = 0;
let countGetWifiState = 0;

function wifistations(data, uri) {
  console.log("add stations", uri);
  if (data["failed"]) {
    ih =
      '<label id="lbl_list_ssid_failed" style="color:red;">scan for WiFi stations failed!</label>';
    document.getElementById("list_ssid").innerHTML = ih;
    document.getElementById("list_ssid_info").innerText = "";
    updateLanguageKeys(["lbl_list_ssid_failed"]);
    return;
  }
  if (!data["finished"]) {
    countGetWifiStations += 1;
    setTimeout(getJSON.bind(null, "/wifi/stations.json", "wifistations"), 1000); //Ruft die Callback-Funktion nach 1 Sekunde auf
    ih = '<label id="lbl_list_ssid_scan">scan in progress...</label>';
    ih +=
      '<label id="lbl_list_ssid_scan_count">' +
      countGetWifiStations +
      "</label>";
    document.getElementById("list_ssid").innerHTML = ih;
    document.getElementById("list_ssid_info").innerText = "";
    updateLanguageKeys(["lbl_list_ssid_scan"]);
    return;
  }
  networks = data["networks"];
  var hiddenCount = 0;
  hh = "";
  for (i = 0; i < networks.length; i++) {
    network = networks[i];
    hh +=
      '<input type="button" onClick="onFocus(this.getAttribute(\'value\'))" value="' +
      network["ssid"] +
      '">';
    hh +=
      '<label class="slist">' +
      getRSSIasQuality(network["rssi"]) +
      "&#037;&ensp;Ch." +
      network["channel"] +
      "</label>";
    if (network["encrypted"]) {
      hh += '<span class="svg-lock"></span>';
      //hh += '<span><svg class="icon icon-connection"><use xlink:href="#icon-connection"></use></svg></span>';
    }
    if (network["hidden"]) {
      hh += '<span class="svg-hidden"></span>';
      hiddenCount += 1;
    }
    hh += "<br>";
  }

  console.log(hh);
  document.getElementById("list_ssid").innerHTML = hh;
  il = '<label id="lbl_total">Total:</label>';
  il += " " + networks.length + " ";
  il += '<label id="lbl_hidden">Hidden:</label>';
  il += " " + hiddenCount;
  document.getElementById("list_ssid_info").innerHTML = il;
  updateLanguageKeys(["lbl_total", "lbl_hidden"]);
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

function wifistate(data, uri) {
  console.log("load wifi state " + uri);
  hh = '<label style="color:black;">' + data["ssid"] + " </label>";
  if (data["ssid"].length > 0) {
    if (data["connected"]) {
      // hh += '<label id="lbl_connected" style="color:green;">connected</label>';
      hh +=
        '<label id="lbl_local_ip" style="color:green">' +
        data["local_ip"] +
        "</label>";
      hh +=
        '<input id="btn_disconnect" type="button" style="padding:3px; width:9em; background: #FF8C00;" onClick="location.href=\'/wifi/disconnect\'" value="disconnect">';
      dbl = document.getElementById("dbl");
      if (dbl != null) {
        dbl.remove();
      }
      if (document.getElementById("list_ssid") == null) {
        clearTimeout(timerInfo);
        timerInfo = setTimeout(redirect.bind(null, "/ewc/info"), 3000);
      }
    } else if (data["failed"]) {
      hh += '<label style="color:red;">' + data["reason"] + "</label>";
      dbl = document.getElementById("dbl");
      if (dbl != null) {
        dbl.remove();
      }
      if (document.getElementById("list_ssid") == null) {
        clearTimeout(timerSetup);
        timerSetup = setTimeout(redirect.bind(null, "/wifi/setup"), 3000);
      }
    } else if (data["ssid"].length > 0) {
      countGetWifiState += 1;
      hh += '<label id="lbl_connecting">connecting...</label>';
      hh +=
        '<label id="lbl_connecting_count">' + countGetWifiState + "</label>";
      clearTimeout(timerState);
      timerState = setTimeout(
        getJSON.bind(null, "/wifi/state.json", "wifistate"),
        1000
      );
    }
  } else {
    hh = "not connected";
  }
  document.getElementById("ssid_current").innerHTML = hh;
  updateLanguageKeys(["btn_disconnect", "lbl_connected", "lbl_connecting"]);
}
