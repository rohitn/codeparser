
#include "Symbol.h"

#include "Token.h"
#include "ParserSession.h"

#if USE_MATHLINK
#include "mathlink.h"
#undef P
#endif // USE_MATHLINK

#if USE_EXPR_LIB
#include "ExprLibrary.h"
#endif // USE_EXPR_LIB

#include <cassert>


bool operator==(Symbol a, Symbol b) {
    return a.Id == b.Id;
}

bool operator!=(Symbol a, Symbol b) {
    return a.Id != b.Id;
}

void Symbol::print(std::ostream& s) const {
    s << Name;
}

#if USE_MATHLINK
void Symbol::put(ParserSessionPtr session) const {

    auto link = session->getMathLink();

    if (!MLPutSymbol(link, Name)) {
        assert(false);
    }
}
#endif // USE_MATHLINK

#if USE_EXPR_LIB
expr Symbol::toExpr(ParserSessionPtr session) const {
    return Expr_MEncodedStringToSymbolExpr(Name);
}
#endif // USE_EXPR_LIB
