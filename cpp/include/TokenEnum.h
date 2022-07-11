
#pragma once

#include <cstdint> // for uint16_t


//
// All group closers
//
enum Closer : uint8_t {
    CLOSER_BARGREATER,
    CLOSER_CLOSECURLY,
    CLOSER_CLOSEPAREN,
    CLOSER_CLOSESQUARE,
    CLOSER_LONGNAME_CLOSECURLYDOUBLEQUOTE,
    CLOSER_LONGNAME_CLOSECURLYQUOTE,
    CLOSER_LONGNAME_RIGHTANGLEBRACKET,
    CLOSER_LONGNAME_RIGHTASSOCIATION,
    CLOSER_LONGNAME_RIGHTBRACKETINGBAR,
    CLOSER_LONGNAME_RIGHTCEILING,
    CLOSER_LONGNAME_RIGHTDOUBLEBRACKET,
    CLOSER_LONGNAME_RIGHTDOUBLEBRACKETINGBAR,
    CLOSER_LONGNAME_RIGHTFLOOR,
    // UNUSED
    CLOSER_ASSERTFALSE,
};

//
// Representing a token enum, with various properties exposed
//
struct TokenEnum {

    uint16_t T;

    constexpr TokenEnum() : T(0) {}

    constexpr TokenEnum(uint16_t T) : T(T) {}

    constexpr uint16_t value() const {
        return (T & 0x1ff);
    }

    //
    // All trivia matches: 0b0_0000_1xxx (x is unknown)
    //
    //         Mask off 0b1_1111_1000 (0x1f8)
    // And test against 0b0_0000_1000 (0x08)
    //
    constexpr bool isTrivia() const {
        return static_cast<bool>((T & 0x1f8) == 0x08);
    }

    //
    // All trivia but ToplevelNewline matches: 0b0_0000_10xx (x is unknown)
    //
    //         Mask off 0b1_1111_1100 (0x1fc)
    // And test against 0b0_0000_1000 (0x08)
    //
    constexpr bool isTriviaButNotToplevelNewline() const {
        return static_cast<bool>((T & 0x1fc) == 0x08);
    }

    //
    // Group 1 matches: 0b0000_0xx0_0000_0000 (x is unknown)
    //
    //         Mask off 0b0000_0110_0000_0000 (0x600)
    // And test against 0b0000_0010_0000_0000 (0x200)
    //
    constexpr bool isPossibleBeginning() const {
        return static_cast<bool>((T & 0x600) == 0x200);
    }
    
    //
    // Group 1 matches: 0b0000_0xx0_0000_0000 (x is unknown)
    //
    //         Mask off 0b0000_0110_0000_0000 (0x600)
    // And test against 0b0000_0100_0000_0000 (0x400)
    //
    constexpr bool isCloser() const {
        return static_cast<bool>((T & 0x600) == 0x400);
    }
  
    //
    // Group 1 matches: 0b0000_0xx0_0000_0000 (x is unknown)
    //
    //         Mask off 0b0000_0110_0000_0000 (0x600)
    // And test against 0b0000_0110_0000_0000 (0x600)
    //
    constexpr bool isError() const {
        return static_cast<bool>((T & 0x600) == 0x600);
    }

    //
    // isUnterminated value matches: 0b0000_000x_xxxx_xxxx (x is unknown)
    //
    // Only valid if already checked isError
    //
    //         Mask off 0b0000_0000_0001_1100 (0x1c)
    // And test against 0b0000_0000_0001_1100 (0x1c)
    //
    constexpr bool isUnterminated() const {
        return static_cast<bool>((T & 0x1c) == 0x1c);
    }

    //
    // Group 2 matches: 0b000x_x000_0000_0000 (x is unknown)
    //
    //         Mask off 0b0001_1000_0000_0000 (0x1800)
    // And test against 0b0000_1000_0000_0000 (0x0800)
    //
    constexpr bool isEmpty() const {
        return static_cast<bool>((T & 0x1800) == 0x0800);
    }
};

bool operator==(TokenEnum a, TokenEnum b);

bool operator!=(TokenEnum a, TokenEnum b);

Closer GroupOpenerToCloser(TokenEnum T);
Closer TokenToCloser(TokenEnum T);
