#include "pch.h"

#include "StringUtil.h"
#include <clocale>
#include <cctype>    // std::tolower
#include <algorithm> // std::equal

// для работы с std::string нет многих стандартных нужных функций.

std::string strUtil::trim(const std::string &str, const char_t trim_char)
{
	std::string res = trimLeft(str, trim_char);
	return trimRight(res, trim_char);
}

std::string strUtil::trimLeft(const std::string &str, const char_t trim_char)
{
	std::string res = str;

	while (!res.empty() && (res.front() == trim_char))
	{
		res.erase(res.begin());
	}

	return res;
}

std::string strUtil::trimRight(const std::string &str, const char_t trim_char)
{
	std::string res = str;

	while (!res.empty() && (res.back() == trim_char))
	{
		res.pop_back();
	}

	return res;
}

std::string strUtil::replaceChar(const std::string &str, const char_t src, const char_t dst)
{
	if (!str.empty())
	{
		std::string res;

		for (auto wch : str)
		{
			if (wch == src)
			{
				wch = dst;
			}

			res.push_back(wch);
		}

		return res;
	}

	return str;
}

std::string strUtil::replaceChars(const std::string &str, const std::string &src, const char_t dst)
{
	if (!str.empty() && !src.empty())
	{
		std::string res;

		for (auto wch : str)
		{
			for (auto s : src)
			{
				if (wch == s)
				{
					wch = dst;
					break;
				}
			}

			res.push_back(wch);
		}

		return res;
	}

	return str;
}

#include <locale>

std::string strUtil::strToUpper(const std::string &str)
{
	std::string res;
	if (!str.empty())
	{
		std::locale loc ("uk_UA.UTF-8") ; // TODO:
		for (auto n : str)
		{
			res.push_back(toupper(n, loc));
		}
	}
	return res;
}

std::string strUtil::strToLower(const std::string &str)
{
	std::string res;
	if (!str.empty())
	{
		std::locale loc ("uk_UA.UTF-8") ; // TODO:
		for (auto n : str)
		{
			res.push_back(tolower(n, loc));
		}
	}
	return res;
}


// сокращение строки до заданной длины.
// Вход: str - строка
// len - желаемая длина
// Выход: сформированная строка
std::string strUtil::CropStr(const std::string &str, const size_t len)
{
	std::string res = trim(str);

	if (res.length() >= len)
	{
		std::string crop = res.substr(0, len - 1); // берём первые len-1 символов
		crop += res.back(); // и берём самый последний символ
		return crop;
	}

	return res;
}

bool strUtil::equalNoCase(const std::string &a, const std::string &b)
{
	return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char_t a, char_t b)
	{
		return std::tolower(a) == std::tolower(b);
	});
}
