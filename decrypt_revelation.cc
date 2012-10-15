/// Copyright (c) 2012 The Native Client Authors. All rights reserved.
/// Use of this source code is governed by a BSD-style license that can be
/// found in the LICENSE file.
///
/// This example demonstrates loading, running and scripting a very simple NaCl
/// module.  To load the NaCl module, the browser first looks for the
/// CreateModule() factory method (at the end of this file).  It calls
/// CreateModule() once to load the module code from your .nexe.  After the
/// .nexe code is loaded, CreateModule() is not called again.
///
/// Once the .nexe code is loaded, the browser than calls the CreateInstance()
/// method on the object returned by CreateModule().  It calls CreateInstance()
/// each time it encounters an <embed> tag that references your NaCl module.
///
/// The browser can talk to your NaCl module via the postMessage() Javascript
/// function.  When you call postMessage() on your NaCl module from the browser,
/// this becomes a call to the HandleMessage() method of your pp::Instance
/// subclass.  You can send messages back to the browser by calling the
/// PostMessage() method on your pp::Instance.  Note that these two methods
/// (postMessage() in Javascript and PostMessage() in C++) are asynchronous.
/// This means they return immediately - there is no waiting for the message
/// to be handled.  This has implications in your program design, particularly
/// when mutating property values that are exposed to both the browser and the
/// NaCl module.

#include <algorithm>
#include <cstdio>
#include <string>
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array_buffer.h"
#include "tomcrypt.h"
#include "zlib.h"

static const size_t kGzipChunk = 256 * 1024;

// scoped_array from
// http://src.chromium.org/viewvc/chrome/trunk/src/base/memory/scoped_ptr.h
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in http://src.chromium.org/viewvc/chrome/trunk/src/LICENSE .
template <class C>
class scoped_array {
 public:
  scoped_array(C* p = NULL, size_t size = 0) : array_(p), size_(size) {}
  ~scoped_array() {
    delete[] array_;
  }

  C* get() const {
    return array_;
  }

  C& operator[](ptrdiff_t i) const {
    return array_[i];
  }

  void reset(C* p = NULL, size_t size = 0) {
    if (p == array_)
      return;
    delete[] array_;
    array_ = p;
    size_ = size;
  }

  size_t size() const {
    return size_;
  }

 private:
  C* array_;
  size_t size_;
};

/// The Instance class.  One of these exists for each instance of your NaCl
/// module on the web page.  The browser will ask the Module object to create
/// a new Instance for each occurence of the <embed> tag that has these
/// attributes:
///     type="application/x-nacl"
///     src="decrypt_revelation.nmf"
/// To communicate with the browser, you must override HandleMessage() for
/// receiving messages from the browser, and use PostMessage() to send messages
/// back to the browser.  Note that this interface is asynchronous.
class DecryptRevelationInstance : public pp::Instance {
 public:
  /// The constructor creates the plugin-side instance.
  /// @param[in] instance the handle to the browser-side plugin instance.
  explicit DecryptRevelationInstance(PP_Instance instance) : pp::Instance(instance)
  {}
  virtual ~DecryptRevelationInstance() {}

  /// Handler for messages coming in from the browser via postMessage().  The
  /// @a var_message can contain anything: a JSON string; a string that encodes
  /// method names and arguments; etc.  For example, you could use
  /// JSON.stringify in the browser to create a message that contains a method
  /// name and some parameters, something like this:
  ///   var json_message = JSON.stringify({ "myMethod" : "3.14159" });
  ///   nacl_module.postMessage(json_message);
  /// On receipt of this message in @a var_message, you could parse the JSON to
  /// retrieve the method name, match it to a function call, and then call it
  /// with the parameter.
  /// @param[in] var_message The message posted by the browser.
  virtual void HandleMessage(const pp::Var& var_message) {
    if (var_message.is_string()) {
      password_ = var_message.AsString();
      return;
    }
    if (var_message.is_array_buffer()) {
      if (password_.empty()) {
        PostMessage(pp::Var("Error: Password not set."));
        return;
      }
      scoped_array<unsigned char> data_decrypted;
      DecryptData(var_message, &data_decrypted);
      if (!data_decrypted.get())
        return;
      UnzipDataAndPostMessage(data_decrypted);
      return;
    }
    PostMessage(pp::Var("Error: Unknown message type."));
  }

  void DecryptData(const pp::Var& var_message, scoped_array<unsigned char>* data_decrypted) {
    std::string password = password_;
    password.resize(32);
    for (size_t i = password_.length(); i < password.length(); ++i)
      password[i] = '\0';

    pp::VarArrayBuffer var_array_buffer(var_message);
    size_t array_buffer_size = var_array_buffer.ByteLength();
    if (array_buffer_size < 29) {
      PostMessage(pp::Var("Error: input file is too small."));
      return;
    }

    symmetric_key skey;
    if (CRYPT_OK != aes_setup(reinterpret_cast<const unsigned char*>(password.data()),
                              password.length(), 0, &skey)) {
      PostMessage(pp::Var("Error: aes_setup failed."));
      return;
    }

    unsigned char* buffer_data = static_cast<unsigned char*>(var_array_buffer.Map());
    unsigned char iv_encrypted[16];
    for (size_t i = 0; i < 16; ++i)
      iv_encrypted[i] = *(buffer_data + 12 + i);

    unsigned char iv_decrypted[16];
    if (CRYPT_OK != aes_ecb_decrypt(iv_encrypted, iv_decrypted, &skey)) {
      PostMessage(pp::Var("Error: aes_ecb_decrypt failed."));
      aes_done(&skey);
      return;
    }
    aes_done(&skey);

    if (-1 == register_cipher(&aes_desc)) {
      PostMessage(pp::Var("Error: Failed to register aes cipher."));
      return;
    }

    symmetric_CBC scbc;
    if (CRYPT_OK != cbc_start(find_cipher("aes"), iv_decrypted,
        reinterpret_cast<const unsigned char*>(password.data()), 32, 0, &scbc)) {
      PostMessage(pp::Var("Error: cbc_start failed."));
      return;
    }

    size_t decrypted_data_size = array_buffer_size - 28;
    data_decrypted->reset(new unsigned char[decrypted_data_size], decrypted_data_size);
    if (CRYPT_OK != cbc_decrypt(
        buffer_data + 28, data_decrypted->get(), decrypted_data_size, &scbc)) {
      PostMessage(pp::Var("Error: cbc_decrypt failed."));
      data_decrypted->reset();
    }
    cbc_done(&scbc);
  }
  
  void UnzipDataAndPostMessage(const scoped_array<unsigned char>& compressed_data) {
    size_t padding_length = compressed_data[compressed_data.size() - 1];
    size_t compressed_data_size = compressed_data.size() - padding_length;

    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    unsigned char unzip_buffer[kGzipChunk];
    std::string uncompressed;

    if (Z_OK != inflateInit(&strm)) {
      PostMessage(pp::Var("Error: inflateInit failed."));
      return;
    }

    int zlib_result;
    unsigned char* next_in = compressed_data.get();
    do {
      strm.avail_in = std::min(compressed_data_size, kGzipChunk);
      strm.next_in = next_in;
      next_in += strm.avail_in;
      compressed_data_size -= strm.avail_in;
      do {
        strm.avail_out = kGzipChunk;
        strm.next_out = unzip_buffer;
        zlib_result = inflate(&strm, Z_NO_FLUSH);
        if (zlib_result == Z_NEED_DICT || zlib_result == Z_DATA_ERROR ||
            zlib_result == Z_MEM_ERROR) {
          PostMessage(pp::Var("Error: inflate failed."));
          inflateEnd(&strm);
          return;
        }

        size_t written_bytes = kGzipChunk - strm.avail_out;
        for (size_t i = 0; i < written_bytes; ++i)
          uncompressed += unzip_buffer[i];
      } while (strm.avail_out == 0);
    } while (zlib_result != Z_STREAM_END);
    inflateEnd(&strm);
    
    pp::Var reply(uncompressed);
    PostMessage(reply);
  }

 private:
  std::string password_;
};

/// The Module class.  The browser calls the CreateInstance() method to create
/// an instance of your NaCl module on the web page.  The browser creates a new
/// instance for each <embed> tag with type="application/x-nacl".
class DecryptRevelationModule : public pp::Module {
 public:
  DecryptRevelationModule() : pp::Module() {}
  virtual ~DecryptRevelationModule() {}

  /// Create and return a DecryptRevelationInstance object.
  /// @param[in] instance The browser-side instance.
  /// @return the plugin-side instance.
  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new DecryptRevelationInstance(instance);
  }
};

namespace pp {
/// Factory function called by the browser when the module is first loaded.
/// The browser keeps a singleton of this module.  It calls the
/// CreateInstance() method on the object you return to make instances.  There
/// is one instance per <embed> tag on the page.  This is the main binding
/// point for your NaCl module with the browser.
Module* CreateModule() {
  return new DecryptRevelationModule();
}

}  // namespace pp
