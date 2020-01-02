
#include "Node.h"

#include "Parser.h" // for TheParser
#include "ByteEncoder.h" // for ByteEncoder
#include "API.h" // for TheParserSession
//#include "Symbol.h"

#include <numeric> // for accumulate

void NodeSeq::append(NodePtr N) {
    vec.push_back(std::move(N));
}

void NodeSeq::appendIfNonEmpty(LeafSeq L) {
    if (!L.empty()) {
        append(NodePtr(new LeafSeqNode(std::move(L))));
    }
}

bool NodeSeq::empty() const {
    return vec.empty();
}

size_t NodeSeq::size() const {
    
    auto accum = std::accumulate(vec.begin(), vec.end(), static_cast<size_t>(0), [](size_t a, const NodePtr& b){ return a + b->size(); });
    
    return accum;
}

void NodeSeq::reserve(size_t i) {
    vec.reserve(i);
}

const Node* NodeSeq::first() const {
    
    auto i = 0;
    
    auto F = vec.at(i).get();
    
    auto FF = F->first();
    
    return FF;
}

const Node* NodeSeq::last() const {
    
    auto i = vec.size()-1;
    
    auto L = vec.at(i).get();
    
    auto LL = L->last();
    
    return LL;
}


void NodeSeq::print(std::ostream& s) const {
    
    s << SYMBOL_LIST->name() << "[";
    
    print0(s);
    
    s << "]";
}

void NodeSeq::print0(std::ostream& s) const {
    
    for (auto& C : vec) {
        C->print(s);
        s << ", ";
    }
}

void LeafSeq::print0(std::ostream& s) const {
    
    for (auto& C : vec) {
        C->print(s);
        s << ", ";
    }
}

LeafSeq::~LeafSeq() {
    
    if (!moved) {
        TheParser->prependInReverse(vec);
    }
}

void LeafSeq::append(LeafNodePtr N) {
    vec.push_back(std::move(N));
}

bool LeafSeq::empty() const {
    return vec.empty();
}

size_t LeafSeq::size() const {
    
    auto accum = std::accumulate(vec.begin(), vec.end(), static_cast<size_t>(0), [](size_t a, const LeafNodePtr& b){ return a + b->size(); });
    
    return accum;
}

const Node* LeafSeq::first() const {
    
    auto F = vec.at(0).get();
    
    auto FF = F->first();
    
    return FF;
}

const Node* LeafSeq::last() const {
    
    auto L = vec.at(vec.size()-1).get();
    
    auto LL = L->last();
    
    return LL;
}


Node::Node(NodeSeq ChildrenIn) : Children(std::move(ChildrenIn)) {
#ifndef NDEBUG
    //
    // These are very useful asserts to help find problems with trivia
    //

    // TODO: enable again after whitespace work
    
//    auto F = Children.first();
//    auto L = Children.last();
//
//    assert(!F->isTrivia());
//    assert(!L->isTrivia());
#endif
}

bool Node::isTrivia() const {
    return false;
}

bool Node::isError() const {
    return false;
}

bool Node::isEmpty() const {
    return false;
}

Source Node::getSource() const {
    
    assert(!Children.empty());
    
    auto First = Children.first();
    auto Last = Children.last();
    
    auto FirstSrc = First->getSource();
    auto LastSrc = Last->getSource();
    
    return Source(FirstSrc, LastSrc);
}

size_t Node::size() const {
    return 1;
}

const Node* Node::first() const {
    return this;
}

const Node* Node::last() const {
    return this;
}

void Node::printChildren(std::ostream& s) const {
    
    Children.print(s);
}

void LeafSeqNode::print(std::ostream& s) const {
    
    Children.print0(s);
}

size_t LeafSeqNode::size() const {
    return Children.size();
}

const Node* LeafSeqNode::first() const {
    assert(!Children.empty());
    return Children.first();
}

const Node* LeafSeqNode::last() const {
    assert(!Children.empty());
    return Children.last();
}


size_t NodeSeqNode::size() const {
    return Children.size();
}

const Node* NodeSeqNode::first() const {
    assert(!Children.empty());
    return Children.first();
}

const Node* NodeSeqNode::last() const {
    assert(!Children.empty());
    return Children.last();
}

void NodeSeqNode::print(std::ostream& s) const {
    
    Children.print0(s);
}

void OperatorNode::print(std::ostream& s) const {
    
    s << MakeSym->name() << "[";
    
    s << Op->name();
    s << ", ";
    
    printChildren(s);
    s << ", ";
    
    getSource().print(s);
    
    s << "]";
}


void LeafNode::print(std::ostream& s) const {
    
    Tok.print(s);
}

bool LeafNode::isTrivia() const {
    return Tok.Tok.isTrivia();
}

bool LeafNode::isEmpty() const {
    return Tok.Tok.isEmpty();
}

void CallNode::print(std::ostream& s) const {
    
    auto Src = getSource();
    
    s << SYMBOL_AST_LIBRARY_MAKECALLNODE->name() << "[";
    
    Head.print(s);
    s << ", ";
    
    printChildren(s);
    s << ", ";
    
    Src.print(s);
    
    s << "]";
}

Source CallNode::getSource() const {
    
    const auto& First = Head.first();
    
    const auto& Children = getChildrenSafe();
    const auto& Last = Children.last();
    
    auto FirstSrc = First->getSource();
    auto LastSrc = Last->getSource();
    
    return Source(FirstSrc, LastSrc);
}


void SyntaxErrorNode::print(std::ostream& s) const {
    
    auto Src = getSource();
    
    s << SYMBOL_AST_LIBRARY_MAKESYNTAXERRORNODE->name() << "[";
    
    s << SyntaxErrorToString(Err);
    s << ", ";
    
    printChildren(s);
    s << ", ";
    
    Src.print(s);
    s << ", ";
    
    s << "]";
}

bool SyntaxErrorNode::isError() const {
    return true;
}

void CollectedExpressionsNode::print(std::ostream& s) const {
    
    s << "List[";
    
    for (auto& E : Exprs) {
        E->print(s);
        s << ", ";
    }
    
    s << "]";
}

void CollectedIssuesNode::print(std::ostream& s) const {
    
    s << "List[";
    
    for (auto& I : Issues) {
        I->print(s);
        s << ", ";
    }
    
    s << "]";
}


void ListNode::print(std::ostream& s) const {
    
    s << "List[";
    
    for (auto& NN : N) {
        NN->print(s);
        s << ", ";
    }
    
    s << "]";
}

void SourceCharacterNode::print(std::ostream& s) const {
    
    s << SYMBOL_AST_LIBRARY_MAKESOURCECHARACTERNODE->name() << "[";
    
    s << SYMBOL_AST_SOURCECHARACTER->name() << ", ";
    
    s << Char;
    
    s << "]\n";
}




#if USE_MATHLINK

void NodeSeq::put(MLINK mlp) const {
    
    auto s = size();
    
    if(!MLPutFunction(mlp, SYMBOL_LIST->name(), static_cast<int>(s))) {
        assert(false);
    }
    
    put0(mlp);
}

void NodeSeq::put0(MLINK mlp) const {
    
    for (auto& C : vec) {
        
#if !NABORT
        //
        // Check isAbort() inside loops
        //
        if (TheParserSession->isAbort()) {
            
            TheParserSession->handleAbort();
            return;
        }
#endif // !NABORT
        
        C->put(mlp);
    }
}

void LeafSeq::put0(MLINK mlp) const {
    
    for (auto& C : vec) {
        
#if !NABORT
        //
        // Check isAbort() inside loops
        //
        if (TheParserSession->isAbort()) {
            
            TheParserSession->handleAbort();
            return;
        }
#endif // !NABORT
        
        C->put(mlp);
    }
}

void Node::putChildren(MLINK mlp) const {
    
    Children.put(mlp);
}

void LeafSeqNode::put(MLINK mlp) const {
    
    Children.put0(mlp);
}

void NodeSeqNode::put(MLINK mlp) const {
    
    Children.put0(mlp);
}

void OperatorNode::put(MLINK mlp) const {

    if(!MLPutFunction(mlp, MakeSym->name(), static_cast<int>(2 + 4))) {
        assert(false);
    }
    
    if(!MLPutSymbol(mlp, Op->name())) {
        assert(false);
    }
    
    putChildren(mlp);
    
    getSource().put(mlp);
}

void LeafNode::put(MLINK mlp) const {
    
    Tok.put(mlp);
}

void CallNode::put(MLINK mlp) const {
    
    auto Src = getSource();
    
    if (!MLPutFunction(mlp, SYMBOL_AST_LIBRARY_MAKECALLNODE->name(), static_cast<int>(2 + 4))) {
        assert(false);
    }
    
    Head.put(mlp);
    
    putChildren(mlp);
    
    Src.put(mlp);
}

void SyntaxErrorNode::put(MLINK mlp) const {
    
    auto Src = getSource();
    
    if (!MLPutFunction(mlp, SYMBOL_AST_LIBRARY_MAKESYNTAXERRORNODE->name(), static_cast<int>(2 + 4))) {
        assert(false);
    }
    
    if (!MLPutSymbol(mlp, SyntaxErrorToString(Err).c_str())) {
        assert(false);
    }
    
    putChildren(mlp);
    
    Src.put(mlp);
}

void CollectedExpressionsNode::put(MLINK mlp) const {
    
    if (!MLPutFunction(mlp, SYMBOL_LIST->name(), static_cast<int>(Exprs.size()))) {
        assert(false);
    }
    
    for (auto& E : Exprs) {
        
#if !NABORT
        //
        // Check isAbort() inside loops
        //
        if (TheParserSession->isAbort()) {
            
            TheParserSession->handleAbort();
            return;
        }
#endif // !NABORT
        
        E->put(mlp);
    }
}

void CollectedIssuesNode::put(MLINK mlp) const {
    
    if (!MLPutFunction(mlp, SYMBOL_LIST->name(), static_cast<int>(Issues.size()))) {
        assert(false);
    }
    
    for (auto& I : Issues) {
        
#if !NABORT
        //
        // Check isAbort() inside loops
        //
        if (TheParserSession->isAbort()) {
            
            TheParserSession->handleAbort();
            return;
        }
#endif // !NABORT
        
        I->put(mlp);
    }
}

void ListNode::put(MLINK mlp) const {
    
    if (!MLPutFunction(mlp, SYMBOL_LIST->name(), static_cast<int>(N.size()))) {
        assert(false);
    }
    
    for (auto& NN : N) {
        
#if !NABORT
        //
        // Check isAbort() inside loops
        //
        if (TheParserSession->isAbort()) {
            
            TheParserSession->handleAbort();
            return;
        }
#endif // !NABORT
        
        NN->put(mlp);
    }
}

void SourceCharacterNode::put(MLINK mlp) const {
    
    if (!MLPutFunction(mlp, SYMBOL_AST_LIBRARY_MAKESOURCECHARACTERNODE->name(), static_cast<int>(2))) {
        assert(false);
    }
    
    if (!MLPutSymbol(mlp, SYMBOL_AST_SOURCECHARACTER->name())) {
        assert(false);
    }
    
    auto val = Char.to_point();
    
    auto S = ByteEncoder::size(val);
        
    std::array<unsigned char, 4> Arr;
    ByteEncoderState state;
    
    ByteEncoder::encodeBytes(Arr, val, &state);
    
    if (!MLPutUTF8String(mlp, reinterpret_cast<Buffer>(Arr.data()), static_cast<int>(S))) {
        assert(false);
    }
}

#endif // USE_MATHLINK

