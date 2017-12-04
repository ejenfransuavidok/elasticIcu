/*
 * normalize.cpp
 *
 *  Created on: Dec 1, 2017
 *      Author: vidok
 */
#include "normalizer.h"

NormSample::NormSample(UErrorCode &errorCode)
{}

// Normalize a string.
UnicodeString NormSample::toNormalize(const char *s, char *key, UNormalization2Mode mode, UErrorCode &errorCode) {
	UErrorCode uErrorCode = U_ZERO_ERROR;
	const Normalizer2 *normalizer = Normalizer2::getInstance(NULL, key, mode, uErrorCode);
	if(U_FAILURE(uErrorCode)) {
		errorCode = uErrorCode;
		return (UnicodeString)-1;
	}
	const UnicodeString src(s);
	UnicodeString result = normalizer->normalize(src, errorCode);
	return result;
}

UnicodeString NormSample::processText(const UnicodeString &s, char *key, UNormalization2Mode mode, UErrorCode &errorCode) {
	char tmp[s.length() * 4];
	s.extract(0, s.length(), tmp, sizeof(tmp));
	return this->toNormalize(tmp, key, mode, errorCode);
}
