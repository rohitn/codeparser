
#include "Parser.h"

#include "API.h" // for TheParserSession
#include "Parselet.h" // for SymbolParselet, UnderParselet, etc.
#include "Tokenizer.h" // for Tokenizer
#include "ParseletRegistration.h"
#include "ByteDecoder.h" // for ByteDecoder
#include "ByteBuffer.h" // for ByteBuffer


Parser::Parser() {}

Parser::~Parser() {}

void Parser::init() {
    
    handleFirstLine(TheParserSession->firstLineBehavior);
}

void Parser::handleFirstLine(FirstLineBehavior firstLineBehavior) {
    
    switch (firstLineBehavior) {
        case FIRSTLINEBEHAVIOR_NOTSCRIPT: {
            
            return;
        }
        case FIRSTLINEBEHAVIOR_CHECK: {
            
            //
            // Handle the optional #! shebang
            //
            
            ParserContext Ctxt;
            
            auto peek = currentToken(Ctxt, TOPLEVEL);
            
            if (peek.Tok != TOKEN_HASH) {
                
                // not #!
                
                //
                // reset
                //
                TheByteBuffer->buffer = peek.BufLen.buffer;
                TheByteDecoder->SrcLoc = peek.Src.Start;
                
                return;
            }
            
            nextToken(peek);
            
            peek = currentToken(Ctxt, TOPLEVEL);
            
            if (peek.Tok != TOKEN_BANG) {
                
                // not #!
                
                //
                // reset
                //
                TheByteBuffer->buffer = peek.BufLen.buffer;
                TheByteDecoder->SrcLoc = peek.Src.Start;
                
                return;
            }
            
            //
            // Definitely a shebang
            //
            
            nextToken(peek);
            
            while (true) {
                
        #if !NABORT
                if (TheParserSession->isAbort()) {
                    
                    break;
                }
        #endif // !NABORT
                
                auto peek = currentToken(Ctxt, TOPLEVEL);
                
                if (peek.Tok == TOKEN_ENDOFFILE) {
                    
                    break;
                }
                
                if (peek.Tok == TOKEN_TOPLEVELNEWLINE) {
                    
                    nextToken(peek);
                    
                    break;
                }
                
                nextToken(peek);
                
            } // while (true)
            
            //
            // TODO: if anyone ever asks, then consider providing the shebang as a token
            // but only after BIGCODEMERGE!!
            //
            
            break;
        }
        case FIRSTLINEBEHAVIOR_SCRIPT: {
            
            //
            // Handle the #! shebang
            //
            
            ParserContext Ctxt;
            
            auto peek = currentToken(Ctxt, TOPLEVEL);
            
            if (peek.Tok != TOKEN_HASH) {
                
                //
                // TODO: add to Issues
                //

                return;
            }
            
            nextToken(peek);
            
            peek = currentToken(Ctxt, TOPLEVEL);
            
            if (peek.Tok != TOKEN_BANG) {
                
                //
                // TODO: add to Issues
                //

                return;
            }
            
            nextToken(peek);
            
            while (true) {
                
        #if !NABORT
                if (TheParserSession->isAbort()) {
                    
                    break;
                }
        #endif // !NABORT
                
                auto peek = currentToken(Ctxt, TOPLEVEL);
                
                if (peek.Tok == TOKEN_ENDOFFILE) {
                    
                    break;
                }
                
                if (peek.Tok == TOKEN_TOPLEVELNEWLINE) {
                    
                    nextToken(peek);
                    
                    break;
                }
                
                nextToken(peek);
                
            } // while (true)
            
            break;
        }
    }
}


void Parser::deinit() {
    
}

void Parser::nextToken(Token Tok) {
    
    TheTokenizer->nextToken(Tok);
}


Token Parser::nextToken0(ParserContext Ctxt, NextPolicy policy) {
    
    auto insideGroup = (Ctxt.Closr != CLOSER_OPEN);
    //
    // if insideGroup:
    //   returnInternalNewlineMask is 0b100
    // else:
    //   returnInternalNewlineMask is 0b000
    //
    auto returnInternalNewlineMask = static_cast<uint8_t>(insideGroup) << 2;
    auto Tok = TheTokenizer->nextToken0(policy & ~(returnInternalNewlineMask));
    
    return Tok;
}

Token Parser::currentToken(ParserContext Ctxt, NextPolicy policy) const {
    
    auto insideGroup = (Ctxt.Closr != CLOSER_OPEN);
    //
    // if insideGroup:
    //   returnInternalNewlineMask is 0b100
    // else:
    //   returnInternalNewlineMask is 0b000
    //
    auto returnInternalNewlineMask = static_cast<uint8_t>(insideGroup) << 2;
    auto Tok = TheTokenizer->currentToken(policy & ~(returnInternalNewlineMask));
    
    return Tok;
}


Token Parser::currentToken_stringifyAsTag() const {
    
    return TheTokenizer->currentToken_stringifyAsTag();
}

Token Parser::currentToken_stringifyAsFile() const {
    
    return TheTokenizer->currentToken_stringifyAsFile();
}

NodePtr Parser::parseLoop(NodePtr Left, ParserContext Ctxt) {
    
    while (true) {
    
    //
    // Check isAbort() inside loops
    //
    HANDLE_ABORT;
    
    TriviaSeq Trivia1;
    
    auto token = currentToken(Ctxt, TOPLEVEL);
    token = eatTriviaButNotToplevelNewlines(token, Ctxt, TOPLEVEL, Trivia1);
    
    auto I = infixParselets[token.Tok.value()];
    
    token = I->processImplicitTimes(token, Ctxt);
    I = infixParselets[token.Tok.value()];
    
    auto TokenPrecedence = I->getPrecedence(Ctxt);
    
    //
    // if (Ctxt.Prec > TokenPrecedence)
    //   break;
    // else if (Ctxt.Prec == TokenPrecedence && Ctxt.Prec.Associativity is NonRight)
    //   break;
    //
    if ((Ctxt.Prec | 0x1) > TokenPrecedence) {
            
        Trivia1.reset();
        
        return Left;
    }
    
    NodeSeq LeftSeq;
    
    LeftSeq.append(std::move(Left));
    LeftSeq.appendSeq(std::move(Trivia1));
    
    auto Ctxt2 = Ctxt;
    Ctxt2.Prec = TokenPrecedence;
    
    Left = I->parseInfix(std::move(LeftSeq), token, Ctxt2);
    
    } // while (true)
}

Token Parser::eatTrivia(Token T, ParserContext Ctxt, NextPolicy policy, TriviaSeq& Args) {
    
    while (T.Tok.isTrivia()) {
        
        //
        // No need to check isAbort() inside tokenizer loops
        //
        
        Args.append(LeafNodePtr(new LeafNode(T)));
        
        nextToken(T);
        
        T = currentToken(Ctxt, policy);
    }
    
    return T;
}

Token Parser::eatTrivia_stringifyAsFile(Token T, ParserContext Ctxt, TriviaSeq& Args) {
    
    while (T.Tok.isTrivia()) {
        
        //
        // No need to check isAbort() inside tokenizer loops
        //
        
        Args.append(LeafNodePtr(new LeafNode(T)));
        
        nextToken(T);
        
        T = currentToken_stringifyAsFile();
    }
    
    return T;
}

Token Parser::eatTriviaButNotToplevelNewlines(Token T, ParserContext Ctxt, NextPolicy policy, TriviaSeq& Args) {
    
    while (T.Tok.isTriviaButNotToplevelNewline()) {
        
        //
        // No need to check isAbort() inside tokenizer loops
        //
        
        Args.append(LeafNodePtr(new LeafNode(T)));
        
        nextToken(T);
        
        T = currentToken(Ctxt, policy);
    }
    
    return T;
}

Token Parser::eatTriviaButNotToplevelNewlines_stringifyAsFile(Token T, ParserContext Ctxt, TriviaSeq& Args) {
    
    while (T.Tok.isTriviaButNotToplevelNewline()) {
        
        //
        // No need to check isAbort() inside tokenizer loops
        //
        
        Args.append(LeafNodePtr(new LeafNode(T)));
        
        nextToken(T);
        
        T = currentToken_stringifyAsFile();
    }
    
    return T;
}

ParserPtr TheParser = nullptr;
