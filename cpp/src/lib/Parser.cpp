
#include "Parser.h"

#include "API.h" // for TheParserSession
#include "Parselet.h" // for SymbolParselet, UnderParselet, etc.
#include "Tokenizer.h" // for Tokenizer
#include "ParseletRegistration.h"
#include "ByteDecoder.h" // for ByteDecoder
#include "ByteBuffer.h" // for ByteBuffer


Parser::Parser() : ArgsStack(), NodeStack(), ContextStack(), GroupStack() {}

void Parser::init() {
    
    clearContextStack();
    pushFreshContext();
    
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
            
            auto peek = currentToken(TOPLEVEL);
            
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
            
            peek = currentToken(TOPLEVEL);
            
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
                
                auto peek = currentToken(TOPLEVEL);
                
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
            
            auto peek = currentToken(TOPLEVEL);
            
            if (peek.Tok != TOKEN_HASH) {
                
                //
                // TODO: add to Issues
                //

                return;
            }
            
            nextToken(peek);
            
            peek = currentToken(TOPLEVEL);
            
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
                
                auto peek = currentToken(TOPLEVEL);
                
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


Token Parser::nextToken0(NextPolicy policy) {
    
    auto insideGroup = getGroupDepth() > 0;
    
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

Token Parser::currentToken(NextPolicy policy) const {
    
    auto insideGroup = getGroupDepth() > 0;
    
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

void Parser_parseClimb(ParseletPtr Ignored, Token Ignored2) {
    
    //
    // Check isAbort() inside loops
    //
    HANDLE_ABORT;
    
    Precedence TokenPrecedence;
    InfixParseletPtr I;
    Token token;
    {
        TriviaSeq Trivia1;
        
        token = TheParser->currentToken(TOPLEVEL);
        token = TheParser->eatTriviaButNotToplevelNewlines(token, TOPLEVEL, Trivia1);
        
        I = infixParselets[token.Tok.value()];
        
        token = I->processImplicitTimes(token);
        I = infixParselets[token.Tok.value()];
        
        TokenPrecedence = I->getPrecedence();
        
        //
        // if (Ctxt.Prec > TokenPrecedence)
        //   break;
        // else if (Ctxt.Prec == TokenPrecedence && Ctxt.Prec.Associativity is NonRight)
        //   break;
        //
        if ((TheParser->topContext().Prec | 0x1) > TokenPrecedence) {
                
            Trivia1.reset();
            
            return;
        }
        
        auto Left1 = TheParser->popNode();
        
        auto& LeftSeq = TheParser->pushArgs();
        
        LeftSeq.append(std::move(Left1));
        LeftSeq.appendSeq(std::move(Trivia1));
    }
    
    MUSTTAIL
    return (I->parseInfix())(I, token);
}

Token Parser::eatTrivia(Token T, NextPolicy policy, TriviaSeq& Args) {
    
    while (T.Tok.isTrivia()) {
        
        //
        // No need to check isAbort() inside tokenizer loops
        //
        
        Args.append(LeafNodePtr(new LeafNode(T)));
        
        nextToken(T);
        
        T = currentToken(policy);
    }
    
    return T;
}

Token Parser::eatTrivia_stringifyAsFile(Token T, TriviaSeq& Args) {
    
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

Token Parser::eatTriviaButNotToplevelNewlines(Token T, NextPolicy policy, TriviaSeq& Args) {
    
    while (T.Tok.isTriviaButNotToplevelNewline()) {
        
        //
        // No need to check isAbort() inside tokenizer loops
        //
        
        Args.append(LeafNodePtr(new LeafNode(T)));
        
        nextToken(T);
        
        T = currentToken(policy);
    }
    
    return T;
}

Token Parser::eatTriviaButNotToplevelNewlines_stringifyAsFile(Token T, TriviaSeq& Args) {
    
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

NodeSeq& Parser::pushArgs() {
    ArgsStack.push_back(NodeSeq());
    return ArgsStack.back();
}

NodeSeq Parser::popArgs() {
    auto top = std::move(ArgsStack.back());
    ArgsStack.pop_back();
    return top;
}

NodeSeq& Parser::peekArgs() {
    return ArgsStack.back();
}

size_t Parser::getArgsStackSize() const {
    return ArgsStack.size();
}

void Parser::pushNode(NodePtr N) {
    NodeStack.push_back(std::move(N));
}

NodePtr Parser::popNode() {
    auto top = std::move(NodeStack.back());
    NodeStack.pop_back();
    return top;
}

size_t Parser::getNodeStackSize() const {
    return NodeStack.size();
}

void Parser::pushGroup(Closer Closr) {
    GroupStack.push_back(Closr);
}

void Parser::popGroup() {
    GroupStack.pop_back();
}

size_t Parser::getGroupDepth() const {
    return GroupStack.size();
}

bool Parser::checkGroup(Closer Closr) const {
    
    for (auto rit = GroupStack.rbegin(); rit != GroupStack.rend(); rit++) {
        
        if (*rit == Closr) {
            return true;
        }
    }
    
    return false;
}

ParserContext& Parser::topContext() {
    return ContextStack.back();
}

ParserContext& Parser::pushFreshContext() {
    ContextStack.push_back(PRECEDENCE_LOWEST);
    return ContextStack.back();
}

ParserContext& Parser::pushInheritedContext(Precedence Prec) {
    auto& Ctxt = ContextStack.back();
    auto insideColon = ((Ctxt.Flag & PARSER_INSIDE_COLON) == PARSER_INSIDE_COLON);
    ContextStack.push_back(Prec);
    if (insideColon) {
        auto& Ctxt2 = ContextStack.back();
        Ctxt2.Flag |= PARSER_INSIDE_COLON;
        return Ctxt2;
    }
    return ContextStack.back();
}

void Parser::popContext() {
    ContextStack.pop_back();
}

size_t Parser::getContextStackSize() const {
    return ContextStack.size();
}

void Parser::clearContextStack() {
    while (!ContextStack.empty()) {
        ContextStack.pop_back();
    }
}

ParserPtr TheParser = nullptr;
