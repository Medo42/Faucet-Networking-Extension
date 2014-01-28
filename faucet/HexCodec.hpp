#pragma once

#include <faucet/Buffer.hpp>
#include <faucet/ReadWritable.hpp>

#include <string>
#include <cstdint>

/**
 * The HexCodec class contains encoding and decoding functions for binary data <-> hexadecimal text.
 * Hex text looks like "1231abcd". It must consist of an even number of letters without any non-hex
 * characters.
 * This class does not attempt to be generic and re-usable in other projects, but is focused on good
 * performance with Faucet Net's data structures. It is written as a class to facilitate resource
 * management, because there are some auxiliary tables that need to be precalculated and that take
 * up some memory.
 */
class HexCodec
{
public:
    /**
     * Precalculate the hexValue table, which contains an entry for every combination of two hex characters
     * as they appear in a hex string, including uppercase and lowercase letters.
     */
    HexCodec()
    {
        uint16_t i=0;
        uint8_t *firstDigit = reinterpret_cast<uint8_t*>(&i);
        uint8_t *secondDigit = firstDigit+1;
        do
        {
            hexValue[i] = getHexValueSlow(*firstDigit)*16 + getHexValueSlow(*secondDigit);
            ++i;
        } while(i != 0);
    }

    template<typename SrcIter, typename DestIter>
    DestIter encode(SrcIter src, SrcIter srcEnd, DestIter dest) const
    {
        for(; src < srcEnd; ++src)
        {
            uint8_t c = *src;
            *dest = hexNibble[(c & 0xf0) >> 4];
            *(++dest) = hexNibble[c & 0x0f];
            ++dest;
        }
		return dest;
    }

    bool validate(const char *begin, const char *&end, size_t &bytes) const
    {
        for(end = begin; *end != 0; ++end)
        {
            if(!isValidHex(*end))
                return false;
        }

        if((end-begin)%2)
            return false;

        bytes = (end-begin)/2;
        return true;
    }

    template<typename DestIter>
    DestIter decode(const char *src, const char *srcEnd, DestIter dest) const
    {
        for(; src < srcEnd; src+=2)
        {
            uint16_t cc = *reinterpret_cast<const uint16_t*>(src);
            *dest = hexValue[cc];
            ++dest;
        }
        return dest;
    }

    std::string readHex(const uint8_t* src, size_t len) const
    {
        if(len*2 < len)
            throw std::length_error("Decoding to hexadecimal would require more memory than can be addressed.");

		std::string result;
		result.resize(len*2);

        encode(src, src+len, result.begin());
		return result;
    }

    std::string readHex(Buffer &buf, size_t len) const
    {
        const uint8_t *src = buf.getData() + buf.getReadpos();
		len = std::min(len, buf.bytesRemaining());

        std::string result = readHex(src, len);

        buf.setReadpos(buf.getReadpos() + len);
        return result;
    }

    double writeHex(const char *hexStr, ReadWritable &dest) const
    {
        size_t bytes = 0;
        const char *hexEnd = hexStr;

        if(!validate(hexStr, hexEnd, bytes))
            return -2;

        dest.prepareWrite(bytes);

        uint8_t outBuffer[2048];
        uint8_t *outBufferStart = &(outBuffer[0]);
        uint8_t *outBufferEnd = outBufferStart+sizeof(outBuffer);

        uint8_t *outBufPtr = outBufferStart;
        for(const char *pStr = hexStr; pStr < hexEnd; pStr+=2) {
            uint16_t cc = *reinterpret_cast<const uint16_t*>(pStr);
            *outBufPtr = hexValue[cc];
            ++outBufPtr;

            if(outBufPtr >= outBufferEnd) {
                dest.write(outBufferStart, sizeof(outBuffer));
                outBufPtr = outBufferStart;
            }
        }

        dest.write(outBufferStart, outBufPtr-outBufferStart);
        return 1;
    }

private:
    uint8_t hexValue[65536];

    /**
     * This table allows the parser to determine quickly whether a byte is a valid hex digit.
     * The entries for the ascii values of all digits, letters from a-f and A-F are set to one.
     */
    const uint8_t validHex[256] =
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, // Digits
        0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, // Uppercase
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, // Lowercase
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    const char hexNibble[16] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };

    bool isValidHex(const char c) const
    {
        return validHex[(uint8_t)c];
    }

    /**
     * Used for precalculating the hexValue-table which is then used for the actual encoding/decoding.
     * Returns 0 for invalid characters.
     */
    uint8_t getHexValueSlow(uint8_t c)
    {
        return (c >= '0' && c <= '9') ? (c - '0') :
               (c >= 'a' && c <= 'f') ? (c - 'a' + 10) :
               (c >= 'A' && c <= 'F') ? (c - 'A' + 10) :
                   0;
    }
};
