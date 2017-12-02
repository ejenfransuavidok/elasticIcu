/*
 * normalize.cpp
 *
 *  Created on: Dec 1, 2017
 *      Author: vidok
 */
#include "normalizer.h"

NormSample::NormSample(UErrorCode &errorCode)
		: 		nfkc(*Normalizer2::getNFDInstance(errorCode)),
				fcd(*Normalizer2::getInstance(NULL, "nfc", UNORM2_DECOMPOSE, errorCode))
{}

// Normalize a string.
UnicodeString NormSample::toNormalize(const char *s, char *key, UNormalization2Mode mode, UErrorCode &errorCode) {
	const Normalizer2 *normalizer = Normalizer2::getInstance(NULL, key, mode, errorCode);
	UnicodeString result = normalizer->normalize(s, errorCode);
	return result;
}

UnicodeString NormSample::processText(const UnicodeString &s, char *key, UNormalization2Mode mode, UErrorCode &errorCode) {
	char tmp[s.length() * 4];
	s.extract(0, s.length(), tmp, sizeof(tmp));
	return this->toNormalize(tmp, key, mode, errorCode);
}
