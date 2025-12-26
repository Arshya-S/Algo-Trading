#include "kraken_base.h"
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <ctime>
#include <iostream>

KrakenBase::KrakenBase(const std::string& api_key, const std::string& private_key, const std::string& base_endpoint): 
    api_key_(api_key), 
    api_secret_(private_key), 
    base_endpoint_(base_endpoint) {}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
  s->append((char*)contents, size * nmemb);
  return size * nmemb;
}

std::string Base64Decode(const std::string& encoded) {
  BIO *bio = BIO_new_mem_buf(encoded.c_str(), encoded.length());
  BIO *b64 = BIO_new(BIO_f_base64());
  bio = BIO_push(b64, bio);
  BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
  
  std::vector<char> buffer(encoded.length());
  int len = BIO_read(bio, buffer.data(), encoded.length());
  BIO_free_all(bio);
  
  return std::string(buffer.data(), len);
}

std::string Base64Encode(const unsigned char* buf, size_t len) {
  BIO *b64 = BIO_new(BIO_f_base64());
  BIO *bio = BIO_new(BIO_s_mem());
  bio = BIO_push(b64, bio);
  BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
  BIO_write(bio, buf, len);
  BIO_flush(bio);
  
  BUF_MEM *ptr;
  BIO_get_mem_ptr(bio, &ptr);
  std::string result(ptr->data, ptr->length);
  BIO_free_all(bio);
  
  return result;
}

std::string KrakenBase::GetAccountBalance() {
  CURL* curl = curl_easy_init();
  std::string response;
  std::string path = "/0/private/Balance";
  std::string nonce = std::to_string(std::time(nullptr) * 1000);
  std::string postdata = "nonce=" + nonce;
  
  // Create signature
  std::string decoded_secret = Base64Decode(api_secret_);
  unsigned char sha256[SHA256_DIGEST_LENGTH];
  SHA256((unsigned char*)(nonce + postdata).c_str(), (nonce + postdata).length(), sha256);
  
  std::string msg = path + std::string((char*)sha256, SHA256_DIGEST_LENGTH);
  unsigned char hmac[EVP_MAX_MD_SIZE];
  unsigned int hmac_len;
  HMAC(EVP_sha512(), decoded_secret.c_str(), decoded_secret.length(),
       (unsigned char*)msg.c_str(), msg.length(), hmac, &hmac_len);
  
  std::string signature = Base64Encode(hmac, hmac_len);
  
  // Setup request
  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, ("API-Key: " + api_key_).c_str());
  headers = curl_slist_append(headers, ("API-Sign: " + signature).c_str());
  
  curl_easy_setopt(curl, CURLOPT_URL, (base_endpoint_ + path).c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  
  curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  
  return response;
}