/*****************************************************************************
 * Copyright (c) 2014-2020 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#if defined(__MINGW32__) && !defined(WINVER) && !defined(_WIN32_WINNT)
// 0x0600 == vista
#    define WINVER 0x0600
#    define _WIN32_WINNT 0x0600
#endif // __MINGW32__

#include <algorithm>
#include <cwctype>
#include <stdexcept>
#include <vector>
#ifndef _WIN32
#    include <unicode/ucnv.h>
#    include <unicode/unistr.h>
#    include <unicode/utypes.h>
#endif

#ifdef _WIN32
#    include <windows.h>
#endif

#include "../common.h"
#include "../localisation/ConversionTables.h"
#include "../localisation/FormatCodes.h"
#include "../localisation/Language.h"
#include "../util/Util.h"
#include "Memory.hpp"
#include "String.hpp"
#include "StringBuilder.h"

namespace String
{
    std::string ToStd(const utf8* str)
    {
        if (str == nullptr)
            return std::string();
        else
            return std::string(str);
    }

    std::string StdFormat_VA(const utf8* format, va_list args)
    {
        auto buffer = Format_VA(format, args);
        auto returnValue = ToStd(buffer);
        Memory::Free(buffer);
        return returnValue;
    }

    std::string StdFormat(const utf8* format, ...)
    {
        va_list args;
        va_start(args, format);
        const utf8* buffer = Format_VA(format, args);
        va_end(args);
        std::string returnValue = ToStd(buffer);
        Memory::Free(buffer);
        return returnValue;
    }

    std::string ToUtf8(const std::wstring_view& src)
    {
#ifdef _WIN32
        int srcLen = static_cast<int>(src.size());
        int sizeReq = WideCharToMultiByte(CODE_PAGE::CP_UTF8, 0, src.data(), srcLen, nullptr, 0, nullptr, nullptr);
        auto result = std::string(sizeReq, 0);
        WideCharToMultiByte(CODE_PAGE::CP_UTF8, 0, src.data(), srcLen, result.data(), sizeReq, nullptr, nullptr);
        return result;
#else
// Which constructor to use depends on the size of wchar_t...
// UTF-32 is the default on most POSIX systems; Windows uses UTF-16.
// Unfortunately, we'll have to help the compiler here.
#    if U_SIZEOF_WCHAR_T == 4
        icu::UnicodeString str = icu::UnicodeString::fromUTF32(reinterpret_cast<const UChar32*>(src.data()), src.length());
#    elif U_SIZEOF_WCHAR_T == 2
        std::wstring wstr = std::wstring(src);
        icu::UnicodeString str = icu::UnicodeString(static_cast<const wchar_t*>(wstr.c_str()));
#    else
#        error Unsupported U_SIZEOF_WCHAR_T size
#    endif

        std::string result;
        str.toUTF8String(result);

        return result;
#endif
    }

    std::wstring ToWideChar(const std::string_view& src)
    {
#ifdef _WIN32
        int srcLen = static_cast<int>(src.size());
        int sizeReq = MultiByteToWideChar(CODE_PAGE::CP_UTF8, 0, src.data(), srcLen, nullptr, 0);
        auto result = std::wstring(sizeReq, 0);
        MultiByteToWideChar(CODE_PAGE::CP_UTF8, 0, src.data(), srcLen, result.data(), sizeReq);
        return result;
#else
        icu::UnicodeString str = icu::UnicodeString::fromUTF8(std::string(src));

// Which constructor to use depends on the size of wchar_t...
// UTF-32 is the default on most POSIX systems; Windows uses UTF-16.
// Unfortunately, we'll have to help the compiler here.
#    if U_SIZEOF_WCHAR_T == 4
        size_t length = static_cast<size_t>(str.length());
        std::wstring result(length, '\0');

        UErrorCode status = U_ZERO_ERROR;
        str.toUTF32(reinterpret_cast<UChar32*>(&result[0]), str.length(), status);

#    elif U_SIZEOF_WCHAR_T == 2
        const char16_t* buffer = str.getBuffer();
        std::wstring result = static_cast<wchar_t*>(buffer);

#    else
#        error Unsupported U_SIZEOF_WCHAR_T size
#    endif

        return result;
#endif
    }

    std::string_view ToStringView(const char* ch, size_t maxLen)
    {
        size_t len{};
        for (size_t i = 0; i < maxLen; i++)
        {
            if (ch[i] == '\0')
            {
                break;
            }
            else
            {
                len++;
            }
        }
        return std::string_view(ch, len);
    }

    bool IsNullOrEmpty(const utf8* str)
    {
        return str == nullptr || str[0] == '\0';
    }

    int32_t Compare(const std::string& a, const std::string& b, bool ignoreCase)
    {
        return Compare(a.c_str(), b.c_str(), ignoreCase);
    }

    int32_t Compare(const utf8* a, const utf8* b, bool ignoreCase)
    {
        if (a == b)
            return 0;
        if (a == nullptr)
            a = "";
        if (b == nullptr)
            b = "";
        if (ignoreCase)
        {
            return _stricmp(a, b);
        }
        else
        {
            return strcmp(a, b);
        }
    }

    bool Equals(const std::string_view a, const std::string_view b, bool ignoreCase)
    {
        if (ignoreCase)
        {
            if (a.size() == b.size())
            {
                for (size_t i = 0; i < a.size(); i++)
                {
                    if (tolower(a[i]) != tolower(b[i]))
                    {
                        return false;
                    }
                }
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return a == b;
        }
    }

    bool Equals(const std::string& a, const std::string& b, bool ignoreCase)
    {
        return Equals(a.c_str(), b.c_str(), ignoreCase);
    }

    bool Equals(const utf8* a, const utf8* b, bool ignoreCase)
    {
        if (a == b)
            return true;
        if (a == nullptr || b == nullptr)
            return false;

        if (ignoreCase)
        {
            return _stricmp(a, b) == 0;
        }
        else
        {
            return strcmp(a, b) == 0;
        }
    }

    bool StartsWith(const utf8* str, const utf8* match, bool ignoreCase)
    {
        if (ignoreCase)
        {
            while (*match != '\0')
            {
                if (*str == '\0' || tolower(*str++) != tolower(*match++))
                {
                    return false;
                }
            }
            return true;
        }
        else
        {
            while (*match != '\0')
            {
                if (*str == '\0' || *str++ != *match++)
                {
                    return false;
                }
            }
            return true;
        }
    }

    bool StartsWith(const std::string& str, const std::string& match, bool ignoreCase)
    {
        return StartsWith(str.c_str(), match.c_str(), ignoreCase);
    }

    bool EndsWith(const std::string_view& str, const std::string_view& match, bool ignoreCase)
    {
        if (str.size() >= match.size())
        {
            auto view = str.substr(str.size() - match.size());
            return Equals(view.data(), match.data(), ignoreCase);
        }
        return false;
    }

    size_t IndexOf(const utf8* str, utf8 match, size_t startIndex)
    {
        const utf8* ch = str + startIndex;
        for (; *ch != '\0'; ch++)
        {
            if (*ch == match)
            {
                return static_cast<size_t>(ch - str);
            }
        }
        return SIZE_MAX;
    }

    ptrdiff_t LastIndexOf(const utf8* str, utf8 match)
    {
        const utf8* lastOccurance = nullptr;
        const utf8* ch = str;
        for (; *ch != '\0'; ch++)
        {
            if (*ch == match)
            {
                lastOccurance = ch;
            }
        }

        if (lastOccurance == nullptr)
        {
            return -1;
        }
        else
        {
            return lastOccurance - str;
        }
    }

    size_t LengthOf(const utf8* str)
    {
        return utf8_length(str);
    }

    size_t SizeOf(const utf8* str)
    {
        return strlen(str);
    }

    utf8* Set(utf8* buffer, size_t bufferSize, const utf8* src)
    {
        return safe_strcpy(buffer, src, bufferSize);
    }

    utf8* Set(utf8* buffer, size_t bufferSize, const utf8* src, size_t srcSize)
    {
        utf8* dst = buffer;
        size_t minSize = std::min(bufferSize - 1, srcSize);
        for (size_t i = 0; i < minSize; i++)
        {
            *dst++ = *src;
            if (*src == '\0')
                break;
            src++;
        }
        *dst = '\0';
        return buffer;
    }

    utf8* Append(utf8* buffer, size_t bufferSize, const utf8* src)
    {
        return safe_strcat(buffer, src, bufferSize);
    }

    utf8* Format(utf8* buffer, size_t bufferSize, const utf8* format, ...)
    {
        va_list args;

        va_start(args, format);
        vsnprintf(buffer, bufferSize, format, args);
        va_end(args);

        // Terminate buffer in case formatted string overflowed
        buffer[bufferSize - 1] = '\0';

        return buffer;
    }

    utf8* Format(const utf8* format, ...)
    {
        va_list args;
        va_start(args, format);
        utf8* result = Format_VA(format, args);
        va_end(args);
        return result;
    }

    utf8* Format_VA(const utf8* format, va_list args)
    {
        va_list args1, args2;
        va_copy(args1, args);
        va_copy(args2, args);

        // Try to format to a initial buffer, enlarge if not big enough
        size_t bufferSize = 4096;
        utf8* buffer = Memory::Allocate<utf8>(bufferSize);

        // Start with initial buffer
        int32_t len = vsnprintf(buffer, bufferSize, format, args);
        if (len < 0)
        {
            Memory::Free(buffer);
            va_end(args1);
            va_end(args2);

            // An error occurred...
            return nullptr;
        }

        size_t requiredSize = static_cast<size_t>(len) + 1;
        if (requiredSize > bufferSize)
        {
            // Try again with bigger buffer
            buffer = Memory::Reallocate<utf8>(buffer, bufferSize);
            len = vsnprintf(buffer, bufferSize, format, args);
            if (len < 0)
            {
                Memory::Free(buffer);
                va_end(args1);
                va_end(args2);

                // An error occurred...
                return nullptr;
            }
        }
        else
        {
            // Reduce buffer size to only what was required
            bufferSize = requiredSize;
            buffer = Memory::Reallocate<utf8>(buffer, bufferSize);
        }

        // Ensure buffer is terminated
        buffer[bufferSize - 1] = '\0';

        va_end(args1);
        va_end(args2);
        return buffer;
    }

    utf8* AppendFormat(utf8* buffer, size_t bufferSize, const utf8* format, ...)
    {
        utf8* dst = buffer;
        size_t i;
        for (i = 0; i < bufferSize; i++)
        {
            if (*dst == '\0')
                break;
            dst++;
        }

        if (i < bufferSize - 1)
        {
            va_list args;
            va_start(args, format);
            vsnprintf(dst, bufferSize - i - 1, format, args);
            va_end(args);

            // Terminate buffer in case formatted string overflowed
            buffer[bufferSize - 1] = '\0';
        }

        return buffer;
    }

    utf8* Duplicate(const std::string& src)
    {
        return String::Duplicate(src.c_str());
    }

    utf8* Duplicate(const utf8* src)
    {
        utf8* result = nullptr;
        if (src != nullptr)
        {
            size_t srcSize = SizeOf(src) + 1;
            result = Memory::Allocate<utf8>(srcSize);
            std::memcpy(result, src, srcSize);
        }
        return result;
    }

    utf8* DiscardUse(utf8** ptr, utf8* replacement)
    {
        Memory::Free(*ptr);
        *ptr = replacement;
        return replacement;
    }

    utf8* DiscardDuplicate(utf8** ptr, const utf8* replacement)
    {
        return DiscardUse(ptr, String::Duplicate(replacement));
    }

    std::vector<std::string> Split(const std::string& s, const std::string& delimiter)
    {
        if (delimiter.empty())
        {
            throw std::invalid_argument(nameof(delimiter) " can not be empty.");
        }

        std::vector<std::string> results;
        if (!s.empty())
        {
            size_t index = 0;
            size_t nextIndex;
            do
            {
                nextIndex = s.find(delimiter, index);
                std::string value;
                if (nextIndex == std::string::npos)
                {
                    value = s.substr(index);
                }
                else
                {
                    value = s.substr(index, nextIndex - index);
                }
                results.push_back(value);
                index = nextIndex + delimiter.size();
            } while (nextIndex != SIZE_MAX);
        }
        return results;
    }

    utf8* SkipBOM(utf8* buffer)
    {
        return const_cast<utf8*>(SkipBOM(static_cast<const utf8*>(buffer)));
    }

    const utf8* SkipBOM(const utf8* buffer)
    {
        if (static_cast<uint8_t>(buffer[0]) == 0xEF && static_cast<uint8_t>(buffer[1]) == 0xBB
            && static_cast<uint8_t>(buffer[2]) == 0xBF)
        {
            return buffer + 3;
        }
        return buffer;
    }

    size_t GetCodepointLength(codepoint_t codepoint)
    {
        return utf8_get_codepoint_length(codepoint);
    }

    codepoint_t GetNextCodepoint(utf8* ptr, utf8** nextPtr)
    {
        return GetNextCodepoint(static_cast<const utf8*>(ptr), const_cast<const utf8**>(nextPtr));
    }

    codepoint_t GetNextCodepoint(const utf8* ptr, const utf8** nextPtr)
    {
        return utf8_get_next(ptr, nextPtr);
    }

    utf8* WriteCodepoint(utf8* dst, codepoint_t codepoint)
    {
        return utf8_write_codepoint(dst, codepoint);
    }

    void AppendCodepoint(std::string& str, codepoint_t codepoint)
    {
        char buffer[8]{};
        utf8_write_codepoint(buffer, codepoint);
        str.append(buffer);
    }

    bool IsWhiteSpace(codepoint_t codepoint)
    {
        // 0x3000 is the 'ideographic space', a 'fullwidth' character used in CJK languages.
        return iswspace(static_cast<wchar_t>(codepoint)) || codepoint == 0x3000;
    }

    utf8* Trim(utf8* str)
    {
        utf8* firstNonWhitespace = nullptr;

        codepoint_t codepoint;
        utf8* ch = str;
        utf8* nextCh;
        while ((codepoint = GetNextCodepoint(ch, &nextCh)) != '\0')
        {
            if (codepoint <= WCHAR_MAX && !IsWhiteSpace(codepoint))
            {
                if (firstNonWhitespace == nullptr)
                {
                    firstNonWhitespace = ch;
                }
            }
            ch = nextCh;
        }

        if (firstNonWhitespace != nullptr && firstNonWhitespace != str)
        {
            // Take multibyte characters into account: use the last byte of the
            // current character.
            size_t newStringSize = (nextCh - 1) - firstNonWhitespace;

#ifdef DEBUG
            size_t currentStringSize = String::SizeOf(str);
            Guard::Assert(newStringSize < currentStringSize, GUARD_LINE);
#endif

            std::memmove(str, firstNonWhitespace, newStringSize);
            str[newStringSize] = '\0';
        }
        else
        {
            *ch = '\0';
        }

        return str;
    }

    const utf8* TrimStart(const utf8* str)
    {
        codepoint_t codepoint;
        const utf8* ch = str;
        const utf8* nextCh;
        while ((codepoint = GetNextCodepoint(ch, &nextCh)) != '\0')
        {
            if (codepoint <= WCHAR_MAX && !IsWhiteSpace(codepoint))
            {
                return ch;
            }
            ch = nextCh;
        }
        // String is all whitespace
        return ch;
    }

    utf8* TrimStart(utf8* buffer, size_t bufferSize, const utf8* src)
    {
        return String::Set(buffer, bufferSize, TrimStart(src));
    }

    std::string TrimStart(const std::string& s)
    {
        const utf8* trimmed = TrimStart(s.c_str());
        return std::string(trimmed);
    }

    std::string Trim(const std::string& s)
    {
        codepoint_t codepoint;
        const utf8* ch = s.c_str();
        const utf8* nextCh;
        const utf8* startSubstr = nullptr;
        const utf8* endSubstr = nullptr;
        while ((codepoint = GetNextCodepoint(ch, &nextCh)) != '\0')
        {
            bool isWhiteSpace = codepoint <= WCHAR_MAX && IsWhiteSpace(codepoint);
            if (!isWhiteSpace)
            {
                if (startSubstr == nullptr)
                {
                    startSubstr = ch;
                }

                // Take multibyte characters into account: move pointer towards
                // the last byte of the current character.
                endSubstr = nextCh - 1;
            }
            ch = nextCh;
        }

        if (startSubstr == nullptr)
        {
            // String is all whitespace
            return std::string();
        }

        size_t stringLength = endSubstr - startSubstr + 1;
        return std::string(startSubstr, stringLength);
    }

#ifndef _WIN32
    static const char* GetIcuCodePage(int32_t codePage)
    {
        switch (codePage)
        {
            case CODE_PAGE::CP_932:
                return "windows-932";

            case CODE_PAGE::CP_936:
                return "GB2312";

            case CODE_PAGE::CP_949:
                return "windows-949";

            case CODE_PAGE::CP_950:
                return "big5";

            case CODE_PAGE::CP_1252:
                return "windows-1252";

            case CODE_PAGE::CP_UTF8:
                return "utf-8";

            default:
                throw std::runtime_error("Unsupported code page: " + std::to_string(codePage));
        }
    }

    static std::string CodePageFromUnicode(icu::UnicodeString src, int32_t dstCodePage)
    {
        UConverter* conv;
        UErrorCode status = U_ZERO_ERROR;

        const char* codepage = GetIcuCodePage(dstCodePage);
        conv = ucnv_open(codepage, &status);

        if (U_FAILURE(status))
        {
            log_error("ICU error: %s", u_errorName(status));
            return nullptr;
        }

        // Allocate buffer to convert to.
        int8_t char_size = ucnv_getMaxCharSize(conv);
        std::string buffer(char_size * src.length(), '\0');

        char* buffer_limit = &buffer[0] + (char_size * src.length());

        // Ready the source string as well...
        const char16_t* source = src.getTerminatedBuffer();
        const char16_t* source_limit = source + src.length();

        // Convert the lot.
        char* buffer_target = &buffer[0];
        ucnv_fromUnicode(
            conv, &buffer_target, buffer_limit, static_cast<const UChar**>(&source), source_limit, nullptr, true, &status);

        if (U_FAILURE(status))
        {
            log_error("ICU error: %s", u_errorName(status));
            return nullptr;
        }

        ucnv_close(conv);

        return buffer;
    }
#endif

    std::string Convert(const std::string_view& src, int32_t srcCodePage, int32_t dstCodePage)
    {
#ifdef _WIN32
        // Convert from source code page to UTF-16
        std::wstring u16;
        {
            int srcLen = static_cast<int>(src.size());
            int sizeReq = MultiByteToWideChar(srcCodePage, 0, src.data(), srcLen, nullptr, 0);
            u16 = std::wstring(sizeReq, 0);
            MultiByteToWideChar(srcCodePage, 0, src.data(), srcLen, u16.data(), sizeReq);
        }

        // Convert from UTF-16 to destination code page
        std::string dst;
        {
            int srcLen = static_cast<int>(u16.size());
            int sizeReq = WideCharToMultiByte(dstCodePage, 0, u16.data(), srcLen, nullptr, 0, nullptr, nullptr);
            dst = std::string(sizeReq, 0);
            WideCharToMultiByte(dstCodePage, 0, u16.data(), srcLen, dst.data(), sizeReq, nullptr, nullptr);
        }

        return dst;
#else
        const char* codepage = GetIcuCodePage(srcCodePage);
        icu::UnicodeString convertString(src.data(), codepage);

        std::string result;
        if (dstCodePage == CODE_PAGE::CP_UTF8)
        {
            convertString.toUTF8String(result);
        }
        else
        {
            result = CodePageFromUnicode(convertString, dstCodePage);
        }

        return result;
#endif
    }

    std::string ToUpper(const std::string_view& src)
    {
#ifdef _WIN32
#    if _WIN32_WINNT >= 0x0600
        auto srcW = ToWideChar(src);

        // Measure how long the destination needs to be
        auto requiredSize = LCMapStringEx(
            LOCALE_NAME_USER_DEFAULT, LCMAP_UPPERCASE | LCMAP_LINGUISTIC_CASING, srcW.c_str(), static_cast<int>(srcW.length()),
            nullptr, 0, nullptr, nullptr, 0);

        auto dstW = std::wstring();
        dstW.resize(requiredSize);

        // Transform the string
        auto result = LCMapStringEx(
            LOCALE_NAME_USER_DEFAULT, LCMAP_UPPERCASE | LCMAP_LINGUISTIC_CASING, srcW.c_str(), static_cast<int>(srcW.length()),
            dstW.data(), static_cast<int>(dstW.length()), nullptr, nullptr, 0);
        if (result == 0)
        {
            // Check the error
            auto error = GetLastError();
            log_warning("LCMapStringEx failed with %d", error);
            return std::string(src);
        }
        else
        {
            return String::ToUtf8(dstW);
        }
#    else
        std::string dst = std::string(src);
        std::transform(dst.begin(), dst.end(), dst.begin(), [](unsigned char c) { return std::toupper(c); });
        return dst;
#    endif
#else
        icu::UnicodeString str = icu::UnicodeString::fromUTF8(std::string(src));
        str.toUpper();

        std::string res;
        str.toUTF8String(res);

        return res;
#endif
    }
} // namespace String

char32_t CodepointView::iterator::GetNextCodepoint(const char* ch, const char** next)
{
    return utf8_get_next(ch, next);
}
