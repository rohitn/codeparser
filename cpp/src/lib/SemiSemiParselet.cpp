
#include "Parselet.h"
#include "ParseletRegistration.h" // for prefixParselets
#include "ParserSession.h"
#include "Symbol.h"
#include "MyString.h"
#include "Parser.h"

#if USE_MUSTTAIL
#define MUSTTAIL [[clang::musttail]]
#else
#define MUSTTAIL
#endif // USE_MUSTTAIL

//
// SemiSemiParselet is complicated enough to warrant its own implementation file.
// The syntax for ;; is complicated and has a lot of edge cases.
//

Precedence SemiSemiParselet::getPrecedence() const {
    return PRECEDENCE_SEMISEMI;
}

Token SemiSemiParselet::processImplicitTimes(Token TokIn) const {
    
    //
    // SemiSemi was already parsed with look-ahead with the assumption that implicit Times will be handled correctly
    //
    
    if (TheParser->getNodeStackSize() == 0) {
        
        //
        // no Node, so this means that Args has already started
        //
                
        return Token(TOKEN_FAKE_IMPLICITTIMES, TokIn.BufLen.buffer, TokIn.Src.Start);
    }
    
    auto& N = TheParser->topNode();
    
    if (auto B = dynamic_cast<BinaryNode *>(N.get())) {
        
        auto Op = B->getOp();
        
        if (Op == SYMBOL_SPAN) {
            
            return Token(TOKEN_FAKE_IMPLICITTIMES, TokIn.BufLen.buffer, TokIn.Src.Start);
        }
        
        //
        // there is a Node, but it is not a Span
        //
        
        return TokIn;
    }
    
    if (auto T = dynamic_cast<TernaryNode *>(N.get())) {
        
        auto Op = T->getOp();
        
        if (Op == SYMBOL_SPAN) {
            
            return Token(TOKEN_FAKE_IMPLICITTIMES, TokIn.BufLen.buffer, TokIn.Src.Start);
        }
        
        //
        // there is a Node, but it is not a Span
        //
        
        return TokIn;
    }
    
    //
    // there is a Node, but it is not a Span
    //
    
    return TokIn;
}

ParseFunction SemiSemiParselet::parsePrefix() const {
    return SemiSemiParselet_parsePrefix;
}

void SemiSemiParselet_parsePrefix(ParseletPtr Ignored, Token TokIn) {
    
#if CHECK_ABORT
    if (TheParserSession->isAbort()) {
        TheParser->pushNode(new AbortNode());
        return Parser_tryContinue(Ignored, TokIn/*ignored*/);
    }
#endif // CHECK_ABORT
    
    TheParser->pushContextV(PRECEDENCE_SEMISEMI);
    
    TheParser->appendArg(new LeafNode(Token(TOKEN_FAKE_IMPLICITONE, TokIn.BufLen.buffer, TokIn.Src.Start)));
    
    //
    // nextToken() is not needed after an implicit token
    //
    
    MUSTTAIL
    return SemiSemiParselet_parseInfix(Ignored, TokIn);
}

ParseFunction SemiSemiParselet::parseInfix() const {
    return SemiSemiParselet_parseInfix;
}

void SemiSemiParselet_parseInfix(ParseletPtr Ignored, Token TokIn) {
    
#if CHECK_ABORT
    if (TheParserSession->isAbort()) {
        TheParser->popContext();
        TheParser->pushNode(new AbortNode());
        return Parser_tryContinue(Ignored, TokIn/*ignored*/);
    }
#endif // CHECK_ABORT
    
    TheParser->appendLeafArgAndNext(TokIn);
    
    MUSTTAIL
    return SemiSemiParselet_parse1(Ignored, TokIn/*ignored*/);
}

void SemiSemiParselet_parse1(ParseletPtr Ignored, Token Ignored2) {
    
#if CHECK_ABORT
    if (TheParserSession->isAbort()) {
        TheParser->popContext();
        TheParser->pushNode(new AbortNode());
        return Parser_tryContinue(Ignored, Ignored2);
    }
#endif // !NABORT
    
    auto& Trivia1 = TheParser->getTrivia1();
    
    auto SecondTok = TheParser->currentToken(TOPLEVEL);
    
    //
    // Span should not cross toplevel newlines
    //
    TheParser->eatTriviaButNotToplevelNewlines(SecondTok, TOPLEVEL, Trivia1);
    
    TheParser->appendArgs(Trivia1);
    
    //
    // a;;
    //  ^~TokIn
    //
    
    if (!SecondTok.Tok.isPossibleBeginning()) {
        
        //
        // a;;&
        //    ^SecondTok
        //
        
        TheParser->pushNode(new LeafNode(Token(TOKEN_FAKE_IMPLICITALL, SecondTok.BufLen.buffer, SecondTok.Src.Start)));
        
        //
        // nextToken() is not needed after an implicit token
        //
        
        MUSTTAIL
        return SemiSemiParselet_reduceBinary(Ignored, Ignored2);
    }
    
    if (SecondTok.Tok != TOKEN_SEMISEMI) {
        
        //
        // a;;b
        //    ^SecondTok
        //
        
        auto& Ctxt = TheParser->topContext();
        assert(Ctxt.F == nullptr);
        Ctxt.F = SemiSemiParselet_parse2;
        
        auto P2 = prefixParselets[SecondTok.Tok.value()];
        
        MUSTTAIL
        return (P2->parsePrefix())(P2, SecondTok);
    }
    
    //
    // a;;;;
    //    ^~SecondTok
    //
    
    TheParser->pushNode(new LeafNode(Token(TOKEN_FAKE_IMPLICITALL, SecondTok.BufLen.buffer, SecondTok.Src.Start)));
    
    //
    // nextToken() is not needed after an implicit token
    //
    
    TheParser->nextToken(SecondTok);
    
    auto ThirdTok = TheParser->currentToken(TOPLEVEL);
    
    //
    // Span should not cross toplevel newlines
    //
    TheParser->eatTriviaButNotToplevelNewlines(ThirdTok, TOPLEVEL, Trivia1);
    
    if (!ThirdTok.Tok.isPossibleBeginning() || ThirdTok.Tok == TOKEN_SEMISEMI) {
        
        //
        // a;;;;&
        //      ^ThirdTok
        //
        
        //
        // a;;;;;;
        //      ^~ThirdTok
        //
        
        Trivia1.reset();
        SecondTok.reset();
        
        MUSTTAIL
        return SemiSemiParselet_reduceBinary(Ignored, Ignored2);
    }
    
    //
    // a;;;;b
    //      ^ThirdTok
    //
    
    TheParser->shift();
    
    TheParser->appendArg(new LeafNode(SecondTok));
    
    //
    // nextToken() already handled above
    //
    
    TheParser->appendArgs(Trivia1);
    
    auto& Ctxt = TheParser->topContext();
    assert(Ctxt.F == nullptr);
    Ctxt.F = SemiSemiParselet_reduceTernary;
    
    auto P2 = prefixParselets[ThirdTok.Tok.value()];
    
    MUSTTAIL
    return (P2->parsePrefix())(P2, ThirdTok);
}

void SemiSemiParselet_parse2(ParseletPtr Ignored, Token Ignored2) {
    
#if CHECK_ABORT
    if (TheParserSession->isAbort()) {
        TheParser->popNode();
        TheParser->popContext();
        TheParser->pushNode(new AbortNode());
        return Parser_tryContinue(Ignored, Ignored2);
    }
#endif // CHECK_ABORT
    
    auto& Trivia1 = TheParser->getTrivia1();
    
    auto ThirdTok = TheParser->currentToken(TOPLEVEL);
    
    //
    // Span should not cross toplevel newlines
    //
    TheParser->eatTriviaButNotToplevelNewlines(ThirdTok, TOPLEVEL, Trivia1);
    
    if (!ThirdTok.Tok.isPossibleBeginning() || ThirdTok.Tok != TOKEN_SEMISEMI) {
        
        //
        // a;;b&
        //     ^ThirdTok
        //
        
        //
        // \[Integral];;x\[DifferentialD]x
        //               ^~~~~~~~~~~~~~~~ThirdTok
        //
        
        Trivia1.reset();
        
        MUSTTAIL
        return SemiSemiParselet_reduceBinary(Ignored, Ignored2);
    }
    
    //
    // a;;b;;
    //     ^~ThirdTok
    //
    
    TheParser->nextToken(ThirdTok);
    
    auto& Trivia2 = TheParser->getTrivia2();
    
    auto FourthTok = TheParser->currentToken(TOPLEVEL);
    
    //
    // Span should not cross toplevel newlines
    //
    TheParser->eatTriviaButNotToplevelNewlines(FourthTok, TOPLEVEL, Trivia2);
    
    if (!FourthTok.Tok.isPossibleBeginning() || FourthTok.Tok == TOKEN_SEMISEMI) {
        
        //
        // a;;b;;&
        //       ^FourthTok
        //
        
        //
        // a;;b;;;;
        //       ^~FourthTok
        //
        
        Trivia2.reset();
        ThirdTok.reset();
        Trivia1.reset();
        
        MUSTTAIL
        return SemiSemiParselet_reduceBinary(Ignored, Ignored2);
    }
        
    //
    // a;;b;;c
    //       ^FourthTok
    //
    
    TheParser->shift();
    
    TheParser->appendArgs(Trivia1);
    
    TheParser->appendArg(new LeafNode(ThirdTok));
    
    //
    // nextToken() already handled above
    //
    
    TheParser->appendArgs(Trivia2);
    
    auto& Ctxt = TheParser->topContext();
    assert(Ctxt.F == SemiSemiParselet_parse2);
    Ctxt.F = SemiSemiParselet_reduceTernary;
    
    auto P2 = prefixParselets[FourthTok.Tok.value()];
    
    MUSTTAIL
    return (P2->parsePrefix())(P2, FourthTok);
}

void SemiSemiParselet_reduceBinary(ParseletPtr Ignored, Token Ignored2) {
    
    TheParser->shift();
    
    TheParser->pushNode(new BinaryNode(SYMBOL_SPAN, TheParser->popContext()));
    
    MUSTTAIL
    return Parser_parseClimb(Ignored, Ignored2);
}

void SemiSemiParselet_reduceTernary(ParseletPtr Ignored, Token Ignored2) {
    
    TheParser->shift();
    
    TheParser->pushNode(new TernaryNode(SYMBOL_SPAN, TheParser->popContext()));
    
    MUSTTAIL
    return Parser_parseClimb(Ignored, Ignored2);
}
