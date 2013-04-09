function select(event) {
  var sourceType = event.target.id;
  enable(sourceType);
  localStorage.sourceType = sourceType;
  localStorage.sourceFile = document.getElementById(
      sourceType == "url" ? "sourceURL" : "sourceDropbox").value;
}

function enable(enableClassName) {
  var toEnable = document.getElementsByClassName(enableClassName);
  var toDisable = document.getElementsByClassName(
      enableClassName == "dropbox" ? "url" : "dropbox");
  for (var i = 0; i < toEnable.length; ++i) {
    toEnable[i].disabled = false;
  }
  for (var i = 0; i < toDisable.length; ++i) {
    toDisable[i].disabled = true;
  }
}

window.onload = function() {
  var radios = document.querySelectorAll("input[type=radio]");
  for (var r = 0; r < radios.length; ++r) {
    var radio = radios[r];
    radio.addEventListener("change", select);
  }

  document.getElementById("load").addEventListener("click", loadURL);

  loadValues();
}

function loadValues() {
  var sourceType = localStorage.sourceType || "dropbox";
  var sourceFile = localStorage.sourceFile || "pwd";
  if (sourceType == "url") {
    document.getElementById("url").checked = true;
    enable("url");
    document.getElementById("sourceURL").value = sourceFile;
  } else if (sourceType == "dropbox") {
    document.getElementById("sourceDropbox").value = sourceFile;
  }
}

function loadURL() {
  var xhr = new XMLHttpRequest();
  xhr.onload = fileLoaded;
  xhr.open("GET", document.getElementById("sourceURL").value, true);
  xhr.responseType = 'arraybuffer';
  xhr.send();
}

function ab2str(buf) {
  return String.fromCharCode.apply(null, new Uint16Array(buf));
}

function fileLoaded() {
  var URL = localStorage.sourceFile || "";
  if (!URL || (URL.indexOf("http") == 0 && this.status != 200))
    return;
  window.localStorage.revelationData = ab2str(this.response);
}
