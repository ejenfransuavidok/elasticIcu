/*
 * normalizer.hpp
 *
 *  Created on: Dec 1, 2017
 *      Author: vidok
 */
#ifndef NORMALIZER_H_
#define NORMALIZER_H_

#include <unicode/unistr.h>
#include <unicode/ustdio.h>
#include <unicode/brkiter.h>
#include <unicode/utypes.h>
#include <unicode/uchar.h>
#include <unicode/locid.h>
#include <unicode/ustring.h>
#include <unicode/ucnv.h>
#include <unicode/regex.h>
#include <unicode/normalizer2.h>

class NormSample {
public:
  // ICU service objects should be cached and reused, as usual.
  NormSample(UErrorCode &errorCode);

  // Normalize a string.
  UnicodeString toNormalize(const char *s, char *key, UNormalization2Mode mode, UErrorCode &errorCode);
  // Ensure FCD before processing (like in sort key generation).
  // In practice, almost all strings pass the FCD test, so it might make sense to
  // test for it and only normalize when necessary, rather than always normalizing.
  UnicodeString processText(const UnicodeString &s, char *key, UNormalization2Mode mode, UErrorCode &errorCode);

private:
  const Normalizer2 &nfkc;
  const Normalizer2 &fcd;
};

#endif /* NORMALIZER_H_ */
