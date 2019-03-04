#pragma once

// crypto -- https://www.cryptopp.com/
#include <cryptopp/aes.h>
#include <cryptopp/base64.h>
#include <cryptopp/ccm.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>

#include "buffer.hpp"

// Encryptor
class Encryptor
{
public:
	Encryptor() {}
	void encrypt(const std::string& key, const Buffer& payload, std::string& cipher, std::vector<std::uint8_t>& iv) {
		cipher.clear();
		// generate iv
		using byte = unsigned char;
		CryptoPP::AutoSeededRandomPool rnd;
		rnd.GenerateBlock(iv.data(), iv.size());

		// decode key
		std::string decoded_key;
		CryptoPP::StringSource decoder(key, true, new CryptoPP::Base64Decoder(new CryptoPP::StringSink(decoded_key)));

		// encrypt payload to cipher
		CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption enc;
		enc.SetKeyWithIV(reinterpret_cast<const byte*>(decoded_key.c_str()), decoded_key.size(), iv.data());
		CryptoPP::StringSource s(payload.get(), true, new CryptoPP::StreamTransformationFilter(enc, new CryptoPP::StringSink(cipher)));
	}
};
