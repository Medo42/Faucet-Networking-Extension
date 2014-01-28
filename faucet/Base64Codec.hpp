#pragma once

#include <faucet/Buffer.hpp>
#include <faucet/ReadWritable.hpp>

#include <string>
#include <cstdint>
#include <array>

class Base64Codec
{
public:
    Base64Codec()
    {
        // Initialize all entries to "invalid character"
        for(int i=0; i<255; i++)
            baseValues_[i] = BV_INVALID;

        // Add the default base64 alphabet characters
        for(int i=0; i<64; i++)
            baseValues_[baseChars_[i]] = i;

        // Add the alternative "filename/URL encoding" safe characters for 62 and 63
        baseValues_['-'] = 0x3e;
        baseValues_['_'] = 0x3f;

        // Add special value for whitespace characters that should be ignored
        baseValues_['\n'] = BV_WHITESPACE;
        baseValues_['\r'] = BV_WHITESPACE;

        // Add special value for padding
        baseValues_['='] = BV_PADDING;

        // Add special value for 0-byte
        baseValues_[0] = BV_END;
    }

    template<typename SrcIter, typename DestIter>
    DestIter encode(SrcIter src, SrcIter srcEnd, DestIter dest) const
    {
        size_t len = srcEnd - src;
        SrcIter bigStepEnd = src+(len/3)*3;

        while(src != bigStepEnd)
        {
            uint32_t b = *src << 16;
            b |= *(++src) << 8;
            b |= *(++src);
            ++src;

            *dest     = baseChars_[(b >> 18) & 0x3f];
            *(++dest) = baseChars_[(b >> 12) & 0x3f];
            *(++dest) = baseChars_[(b >>  6) & 0x3f];
            *(++dest) = baseChars_[(b      ) & 0x3f];
            ++dest;
        }

        // If we still have one or two bytes left in the input, we need to write them with padding
        if(srcEnd - src == 1)
        {
            uint8_t b = *src;
            ++src;

            *dest     = baseChars_[(b >> 2) & 0x3f];
            *(++dest) = baseChars_[(b << 4) & 0x3f];
            *(++dest) = '=';
            *(++dest) = '=';
            ++dest;
        }
        else if(srcEnd - src == 2)
        {
            uint16_t b = *src << 8;
            b |= *(++src);
            ++src;

            *dest     = baseChars_[(b >> 10) & 0x3f];
            *(++dest) = baseChars_[(b >>  4) & 0x3f];
            *(++dest) = baseChars_[(b <<  2) & 0x3f];
            *(++dest) = '=';
            ++dest;
        }

        return dest;
    }

    bool validate(const char *begin, const char *&end, size_t &bytes) const
    {
        size_t quadChars = 0; // the number of characters in unpadded four-character blocks
        size_t extraBytes = 0; // Additional bytes form padded blocks
        uint8_t quadPos = 0; // the position inside the quad

        for(end = begin;; ++end)
        {
            uint8_t charval = baseValues_[*end];
            if(charval < 0x40) // Normal Base64 character
            {
                quadPos++;
                quadChars += quadPos & 0x04; // Branchless version of "if(quadPos == 4) quadChars+=4"
                quadPos &= 0x03;
            }
            else if(charval == BV_WHITESPACE)
            {
                // Whitespace, ignore
            }
            else if(charval == BV_PADDING)
            {
                // We simply interpret a padding character as "end of the current block"
                // That means we accept e.g. "aa=aa=" as a valid Base64 string, but it
                // simplifies the parsing a bit.
                if(quadPos == 1)
                    return -3; // Invalid padding, a quad needs at least two normal characters.

                if(quadPos == 2)
                    extraBytes += 1;

                if(quadPos == 3)
                    extraBytes += 2;

                quadPos = 0;
            }
            else if(charval == BV_END)
            {
                break;
            }
            else
            {
                return false; // Invalid character in base64 string
            }
        }

        bytes = quadChars/4*3 + extraBytes;
        return true;
    }

    std::string readBase64(const uint8_t* src, size_t len) const
    {
        size_t resultLen = (len+2) / 3 * 4;
        if(resultLen < len) // Test for integer overflow
            throw std::length_error("Decoding to hexadecimal would require more memory than can be addressed.");

        std::string result;
        result.resize(resultLen);

        encode(src, src+len, result.begin());
		return result;
    }

    std::string readBase64(Buffer &buf, size_t len) const
    {
        const uint8_t *src = buf.getData() + buf.getReadpos();
		len = std::min(len, buf.bytesRemaining());

        std::string result = readBase64(src, len);

        buf.setReadpos(buf.getReadpos() + len);
        return result;
    }

    double writeBase64(const char *baseStr, ReadWritable &dest) const
    {
        size_t bytes = 0;
        const char *baseEnd = baseStr;

        if(!validate(baseStr, baseEnd, bytes))
            return -2;

        dest.prepareWrite(bytes);

        std::array<uint8_t, 2048> outBuffer;
        auto outIter = outBuffer.begin();

        uint16_t bitBuffer = 0;
        uint8_t bitsInBuffer = 0;

        for(const char *pStr = baseStr;; ++pStr) {
            uint8_t charval = baseValues_[*pStr];
            if(charval < 0x40)
            {
                bitBuffer = (bitBuffer << 6) | charval;

                bitsInBuffer += 6;
                if(bitsInBuffer>=8)
                {
                    bitsInBuffer -= 8;
                    (*outIter) = bitBuffer>>bitsInBuffer;
                    if(++outIter == outBuffer.end())
                    {
                        dest.write(outBuffer.data(), outBuffer.size());
                        outIter = outBuffer.begin();
                    }
                }
            }
            else if(charval == BV_WHITESPACE)
            {
                // Newline, ignore
            }
            else if(charval == BV_PADDING)
            {
                bitsInBuffer = 0;
            }
            else if(charval == BV_END)
            {
                break;
            }
        }

        dest.write(outBuffer.data(), outIter-outBuffer.begin());

        return 1;
    }

private:
    static const uint8_t BV_INVALID = 0xff;
    static const uint8_t BV_WHITESPACE = 0xfe;
    static const uint8_t BV_PADDING = 0xfd;
    static const uint8_t BV_END = 0xfc;

    const char* const baseChars_ = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::array<uint8_t, 256> baseValues_;
};
