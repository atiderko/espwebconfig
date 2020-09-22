// load json files on load this script
if (typeof jsons === "undefined") {
  var jsons = [];
}
jsons.push(["/menu", "menu"]);

let running_json = false;
console.log('load json files: ' + jsons);
_loadJson();

// use this getJSON to invoke the load of extended info 
function getJSON(uri, callback) {
  console.log("getJSON add " + callback + " uri: " + uri);
  jsons.push([uri, callback]);
  _loadJson();
}

function _loadJson() {
  console.log('_loadJson, running_json: ' + running_json);
  if (running_json) {
    return;
  }
  let tuple = jsons.pop();
  console.log('_loadJson, tuple: ' + tuple);
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
    _loadJson();
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

function redirect(url) {
  location.href = url;
}
