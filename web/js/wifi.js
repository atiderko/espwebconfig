let countGetWifiStations = 0;
let countGetWifiState = 0;

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
  