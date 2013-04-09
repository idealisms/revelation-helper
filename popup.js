decryptRevelationModule = null;  // Global application object.
statusText = 'NO-STATUS';

// Indicate load success.
function moduleDidLoad() {
  decryptRevelationModule = document.getElementById('nacl_module');
  updateStatus('moduleDidLoad');
}

// The 'message' event handler.  This handler is fired when the NaCl module
// posts a message to the browser by calling PPB_Messaging.PostMessage()
// (in C) or pp::Instance.PostMessage() (in C++).  This implementation
// simply displays the content of the message in an alert panel.
function handleMessage(message_event) {
  if ("Error" == message_event.data.substring(0, 5)) {
    updateStatus(message_event.data);
    return;
  }
  parseRevelationXML(message_event.data);
}

function parseRevelationXML(xmlText) {
  var parser = new DOMParser();
  var xmlDoc = parser.parseFromString(xmlText, "text/xml");
  if (xmlDoc.firstChild.firstChild.nodeName == "parsererror") {
    updateStatus("XML parser error");
    return;
  }
  var nameToShow = document.getElementById("names").value;
  updateNames(xmlDoc);
  showName(nameToShow, xmlDoc);
}

function updateNames(xmlDoc) {
  var entries = xmlDoc.getElementsByTagName("entry");
  var namesHTML = "";
  for (var i = 0; i < entries.length; ++i) {
    var entry = entries[i];
    var name = entry.getElementsByTagName("name")[0].textContent;
    var type = entry.getAttribute("type");
    var displayName = name;
    if (type != "website")
      displayName += " (" + type + ")";
    namesHTML += "<option value='" + name.replace("'", "&apos;") + "'>" +
        displayName.replace("<", "&lt;") + "</option>";
  }
  var selectElement = document.getElementById("names");
  selectElement.disabled = false;
  selectElement.innerHTML = namesHTML;
  window.localStorage.namesHTML = namesHTML;
}

function showName(nameToShow, xmlDoc) {
  var optionElement = document.querySelector(
      "option[value=\"" + nameToShow + "\"]");

  if (!optionElement) {
    updateStatus("Not found: " + nameToShow);
    return;
  }
  optionElement.selected = true;
  var entries = xmlDoc.getElementsByTagName("entry");
  for (var i = 0; i < entries.length; ++i) {
    var entry = entries[i];
    var name = entry.getElementsByTagName("name")[0].textContent;
    if (name == nameToShow) {
      document.getElementById("name").value = name;
      document.getElementById("description").value =
          entry.getElementsByTagName("description")[0].textContent;
      var fields = entry.getElementsByTagName("field");
      for (var j = 0; j < fields.length; ++j) {
        var field = fields[j];
        if (field.getAttribute("id") == "generic-username") {
          document.getElementById("username").value = field.textContent;
        } else if (field.getAttribute("id") == "generic-password") {
          document.getElementById("site-password").value = field.textContent;
        }
      }
      return;
    }
  }
}

// If the page loads before the Native Client module loads, then set the
// status message indicating that the module is still loading.  Otherwise,
// do not change the status message.
function pageDidLoad() {
  updateStatus('LOADING...');
  var listener = document.getElementById('listener');
  listener.addEventListener('load', moduleDidLoad, true);
  listener.addEventListener('message', handleMessage, true);

  var formElement = document.getElementById("form");
  formElement.addEventListener('submit', decrypt, true);

  var loadRealButton = document.getElementById('load-real');
  loadRealButton.addEventListener('click', loadRealFile, true);
  var sourceType = localStorage.sourceType || "dropbox";
  if (sourceType == "url")
    document.getElementById("load-real-container").style.display = "none";

  var embed = document.createElement("embed");
  embed.setAttribute("name", "nacl_module");
  embed.setAttribute("id", "nacl_module");
  embed.setAttribute("type", "application/x-nacl");
  embed.setAttribute("src", "decrypt_revelation.nmf");
  listener.appendChild(embed);

  if (window.localStorage.namesHTML) {
    var selectElement = document.getElementById("names");
    selectElement.innerHTML = window.localStorage.namesHTML;
    selectElement.disabled = false;
  }

  document.getElementById("password").focus();
}

// Set the global status message.  If the element with id 'statusField'
// exists, then set its HTML to the status message as well.
// opt_message The message test.  If this is null or undefined, then
// attempt to set the element with id 'statusField' to the value of
// |statusText|.
function updateStatus(opt_message) {
  if (opt_message)
    statusText = opt_message;
  var statusField = document.getElementById('status-field');
  if (statusField) {
    statusField.innerHTML = statusText;
  }
}

function fileLoaded() {
  if (this.status != 200)
    return;
  window.localStorage.revelationData = ab2str(this.response);
  updateStatus("loaded file");
}

function decrypt(event) {
  event.preventDefault();
  var password = document.getElementById("password").value;
  decryptRevelationModule.postMessage(password);
  decryptRevelationModule.postMessage(
      str2ab(window.localStorage.revelationData));
}

function loadRealFile() {
  var sourceFile = localStorage.sourceFile || "pwd";
  updateStatus("loading '" + sourceFile + "' from dropbox");

  chrome.tabs.getSelected(null, function(tab) {
    chrome.tabs.sendMessage(tab.id, {filename: sourceFile}, function(response) {
      window.localStorage.revelationData = response.revelationData;
      updateStatus("saved real file");
    });
  });
}

function ab2str(buf) {
  return String.fromCharCode.apply(null, new Uint16Array(buf));
}

function str2ab(str) {
  var buf = new ArrayBuffer(str.length * 2);
  var bufView = new Uint16Array(buf);
  for (var i = 0; i < str.length; i++) {
    bufView[i] = str.charCodeAt(i);
  }
  return buf;
}

window.onload = pageDidLoad;
