function ab2str(buf) {
  return String.fromCharCode.apply(null, new Uint16Array(buf));
}

function fetchFileAndSendResponse(anchorElement, sendResponse) {
  console.log("fetchFileAndSendResponse");
  var xhr = new XMLHttpRequest();
  xhr.onload = function() {
    if (this.status != 200)
      return;
    sendResponse({revelationData: ab2str(this.response)});
  };
  xhr.open("GET", anchorElement.href, true);
  xhr.responseType = 'arraybuffer';
  xhr.send();
  
}


chrome.extension.onMessage.addListener(
  function(request, sender, sendResponse) {
    var filename = request.filename;
    var fileList = document.getElementById("browse-files");
    var files = fileList.getElementsByTagName("a");
    for (var f = files.length - 1; f >= 0; --f) {
      var file = files[f];
      if (file.textContent == filename) {
        fetchFileAndSendResponse(file, sendResponse);
        break;
      }
    }
    return true;
  });
