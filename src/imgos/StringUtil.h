#pragma once
#include <string>

namespace strUtil
{
	// удаление заданного символа с обоих концов строки
	std::string trim(const std::string &str, const char trim_char = ' ');
	// удаление заданного символа в начале строки
	std::string trimLeft(const std::string &str, const char trim_char = ' ');
	// удаление заданного символа в конце строки
	std::string trimRight(const std::string &str, const char trim_char = ' ');
	// замена всех символов src в строке на символы dst
	std::string replaceChar(const std::string &str, const char src, const char dst);
	// замена всех символов, входящих в src в строке на символы dst
	std::string replaceChars(const std::string &str, const std::string &src, const char dst);
	// преорбазование регистра, сделать все буквы большими
	std::string strToUpper(const std::string &str);
	// преорбазование регистра, сделать все буквы маленькими
	std::string strToLower(const std::string &str);
	// сокращение строки до заданной длины.
	std::string CropStr(const std::string &str, const size_t len);
	// сравнение строк без учёта регистра
	bool equalNoCase(const std::string &a, const std::string &b);
}

#define CP_OEMCP                  1           // default to OEM  code page
#define LPCSTR const char*

static inline void MultiByteToWideChar(int cp, int sts, LPCSTR src, int sz, char* dst, int lim) {
	memcpy(dst, src + sts, lim);
}
