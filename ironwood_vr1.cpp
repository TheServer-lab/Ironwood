// ============================================================
//  Ironwood v3.1 — General Purpose Language
//  Compile: g++ -std=c++17 -O2 -o ironwood ironwood_v2.cpp
//  Run:     ./ironwood program.irw [arg1 arg2 ...]
//
//  v2.0:  Classes, error handling, dict ops, file I/O
//  v3.0:  Strings, lambdas, sort, type of, ternary, JSON, args, modules
//  v3.1 New Features:
//    Networking — fetch "url"                         → GET
//                 fetch "url" with {method,body,headers} → POST/PUT/etc.
//                 Returns {body, status, ok}
//    Subprocess — run "ls -la"
//                 Returns {output, code, ok}
// ============================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include <functional>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <unordered_set>
#include <list>
#include <limits>
// networking / subprocess — cross-platform
#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
#  ifndef _SSIZE_T_DEFINED
   typedef int ssize_t;
#  endif
#  define close(s) closesocket(s)
   // popen/pclose are available on Windows as _popen/_pclose
#  define popen  _popen
#  define pclose _pclose
   // WIFEXITED / WEXITSTATUS are not defined on Windows; pclose returns the exit code directly
#  ifndef WIFEXITED
#    define WIFEXITED(s)   (true)
#    define WEXITSTATUS(s) (s)
#  endif
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <netdb.h>
#  include <unistd.h>
#  include <sys/wait.h>
#endif
#include <cstring>
#include <ctime>

// ============================================================
//  TOKENS
// ============================================================

enum class TT {
    NUMBER, STRING, IDENT,
    TRUE_KW, FALSE_KW, NULL_KW,
    LET, SET, FUNCTION, RETURN,
    IF, ELSE, WHILE, FOR, EACH, IN_KW, BREAK, CONTINUE,
    GET, AS, CALL, ASK, SAY, PAUSE, END,
    AND, OR, NOT,
    // v1.1 Scratch-style array keywords
    ADD, TO, LENGTH, OF, ITEM, KEEP, ITEMS, WHERE,
    // v2.0 new keywords
    CLASS, NEW_KW, SELF_KW,
    TRY, CATCH, THROW,
    HAS, KEYS, VALUES,
    // v2.0 Scratch-style file I/O
    FILE_KW, READ_KW, WRITE_KW, APPEND_KW,
    // v3.0 Scratch-style string ops
    SPLIT_KW, BY, JOIN_KW, WITH, TRIM_KW,
    REPLACE_KW, INDEX_KW, UPPER_KW, LOWER_KW, CHARS_KW, FROM,
    // v3.0 misc
    TYPE_KW, THEN, SORT_KW, JSON_KW, PARSE_KW,
    // v3.1 networking + subprocess
    FETCH_KW, RUN_KW,
    // operators
    PLUS, MINUS, STAR, SLASH, PERCENT,
    EQ, NEQ, LT, GT, LEQ, GEQ, ASSIGN,
    LPAREN, RPAREN, LBRACKET, RBRACKET, LBRACE, RBRACE,
    COMMA, DOT, COLON, NEWLINE, EOF_T
};

struct Token { TT type; std::string val; int line{1}; };

// ============================================================
//  LEXER
// ============================================================

class Lexer {
    std::string src;
    size_t pos{0};
    int    line{1};

    static const std::unordered_map<std::string, TT> kw;

    char peek(int off=0) const { size_t i=pos+off; return i<src.size()?src[i]:'\0'; }
    char advance() { char c=src[pos++]; if(c=='\n')line++; return c; }
    void skipComment() { while(pos<src.size()&&src[pos]!='\n')pos++; }

    Token makeStr() {
        std::string s;
        while(pos<src.size()&&src[pos]!='"') {
            if(src[pos]=='\\'){pos++; switch(src[pos]){case 'n':s+='\n';break;case 't':s+='\t';break;default:s+=src[pos];} pos++;}
            else s+=src[pos++];
        }
        if(pos<src.size())pos++;
        return {TT::STRING,s,line};
    }
    Token makeNum() {
        size_t start=pos-1;
        while(pos<src.size()&&(std::isdigit(src[pos])||src[pos]=='.'))pos++;
        return {TT::NUMBER,src.substr(start,pos-start),line};
    }
    Token makeIdent(char first) {
        std::string s(1,first);
        while(pos<src.size()&&(std::isalnum(src[pos])||src[pos]=='_'))s+=src[pos++];
        auto it=kw.find(s);
        return it!=kw.end()?Token{it->second,s,line}:Token{TT::IDENT,s,line};
    }
public:
    Lexer(std::string source):src(std::move(source)){}
    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        bool lastNL=true;
        auto emitNL=[&](){ if(!lastNL){tokens.push_back({TT::NEWLINE,"\n",line});lastNL=true;} };
        while(pos<src.size()) {
            char c=advance();
            if(c==';'){skipComment();continue;}
            if(c=='\n'){emitNL();continue;}
            if(c=='\r')continue;
            if(std::isspace(c))continue;
            lastNL=false;
            if(c=='"'){tokens.push_back(makeStr());continue;}
            if(std::isdigit(c)||(c=='-'&&std::isdigit(peek()))){tokens.push_back(makeNum());continue;}
            if(std::isalpha(c)||c=='_'){tokens.push_back(makeIdent(c));continue;}
            switch(c){
                case '+':tokens.push_back({TT::PLUS,"+",line});break;
                case '-':tokens.push_back({TT::MINUS,"-",line});break;
                case '*':tokens.push_back({TT::STAR,"*",line});break;
                case '/':tokens.push_back({TT::SLASH,"/",line});break;
                case '%':tokens.push_back({TT::PERCENT,"%",line});break;
                case '(':tokens.push_back({TT::LPAREN,"(",line});break;
                case ')':tokens.push_back({TT::RPAREN,")",line});break;
                case '[':tokens.push_back({TT::LBRACKET,"[",line});break;
                case ']':tokens.push_back({TT::RBRACKET,"]",line});break;
                case '{':tokens.push_back({TT::LBRACE,"{",line});break;
                case '}':tokens.push_back({TT::RBRACE,"}",line});break;
                case ',':tokens.push_back({TT::COMMA,",",line});break;
                case '.':tokens.push_back({TT::DOT,".",line});break;
                case ':':tokens.push_back({TT::COLON,":",line});break;
                case '=': if(peek()=='='){pos++;tokens.push_back({TT::EQ,"==",line});}else tokens.push_back({TT::ASSIGN,"=",line});break;
                case '!': if(peek()=='='){pos++;tokens.push_back({TT::NEQ,"!=",line});}break;
                case '<': if(peek()=='='){pos++;tokens.push_back({TT::LEQ,"<=",line});}else tokens.push_back({TT::LT,"<",line});break;
                case '>': if(peek()=='='){pos++;tokens.push_back({TT::GEQ,">=",line});}else tokens.push_back({TT::GT,">",line});break;
                default:break;
            }
        }
        emitNL();
        tokens.push_back({TT::EOF_T,"",line});
        return tokens;
    }
};

const std::unordered_map<std::string,TT> Lexer::kw = {
    {"let",TT::LET},{"set",TT::SET},{"function",TT::FUNCTION},{"return",TT::RETURN},
    {"if",TT::IF},{"else",TT::ELSE},{"while",TT::WHILE},{"for",TT::FOR},{"each",TT::EACH},
    {"in",TT::IN_KW},{"break",TT::BREAK},{"continue",TT::CONTINUE},
    {"get",TT::GET},{"as",TT::AS},{"call",TT::CALL},{"ask",TT::ASK},{"say",TT::SAY},
    {"pause",TT::PAUSE},{"end",TT::END},
    {"true",TT::TRUE_KW},{"false",TT::FALSE_KW},{"null",TT::NULL_KW},
    {"and",TT::AND},{"or",TT::OR},{"not",TT::NOT},
    // v1.1 array
    {"add",TT::ADD},{"to",TT::TO},{"length",TT::LENGTH},{"of",TT::OF},
    {"item",TT::ITEM},{"keep",TT::KEEP},{"items",TT::ITEMS},{"where",TT::WHERE},
    // v2.0
    {"class",TT::CLASS},{"new",TT::NEW_KW},{"self",TT::SELF_KW},
    {"try",TT::TRY},{"catch",TT::CATCH},{"throw",TT::THROW},
    {"has",TT::HAS},{"keys",TT::KEYS},{"values",TT::VALUES},
    // v2.0 Scratch-style file I/O
    {"file",TT::FILE_KW},{"read",TT::READ_KW},{"write",TT::WRITE_KW},{"append",TT::APPEND_KW},
    // v3.0 string ops
    {"split",TT::SPLIT_KW},{"by",TT::BY},{"join",TT::JOIN_KW},{"with",TT::WITH},{"trim",TT::TRIM_KW},
    {"replace",TT::REPLACE_KW},{"index",TT::INDEX_KW},{"uppercase",TT::UPPER_KW},
    {"lowercase",TT::LOWER_KW},{"chars",TT::CHARS_KW},{"from",TT::FROM},
    // v3.0 misc
    {"type",TT::TYPE_KW},{"then",TT::THEN},{"sort",TT::SORT_KW},{"json",TT::JSON_KW},{"parse",TT::PARSE_KW},
    // v3.1
    {"fetch",TT::FETCH_KW},{"run",TT::RUN_KW},
};

// ============================================================
//  AST  — Expressions
// ============================================================

struct Expr; struct Stmt;
using ExprPtr  = std::unique_ptr<Expr>;
using StmtPtr  = std::unique_ptr<Stmt>;
using StmtList = std::vector<StmtPtr>;

struct NumberLit    { double value; };
struct StringLit    { std::string value; };
struct BoolLit      { bool value; };
struct NullLit      {};
struct ArrayLit     { std::vector<ExprPtr> elems; };
struct ObjectLit    { std::vector<std::pair<std::string,ExprPtr>> pairs; };
struct VarExpr      { std::string name; };
struct BinExpr      { std::string op; ExprPtr left, right; };
struct UnaryExpr    { std::string op; ExprPtr operand; };
struct IndexExpr    { ExprPtr obj, index; };
struct MemberExpr   { ExprPtr obj; std::string field; };
struct CallExpr     { ExprPtr callee; std::vector<ExprPtr> args; };
// v1.1 Scratch-style
struct LengthOfExpr { ExprPtr arr; };
struct ItemOfExpr   { ExprPtr index, arr; };
struct KeepWhereExpr{ ExprPtr arr, fn; };
// v2.0 new
struct ClassNewExpr { std::string className; std::vector<ExprPtr> args; };
struct HasExpr      { ExprPtr item, collection; };
struct KeysOfExpr   { ExprPtr dict; };
struct ValuesOfExpr { ExprPtr dict; };
// v2.0 Scratch-style file I/O expressions
struct ReadFileExpr    { ExprPtr path; };          // read file <path>
struct FileExistsExpr  { ExprPtr path; };          // file exists <path>
struct LinesOfFileExpr { ExprPtr path; };          // lines of file <path>
// v3.0 lambda
struct FuncExpr     { std::vector<std::string> params; StmtList body; };
// v3.0 ternary
struct TernaryExpr  { ExprPtr cond, thenE, elseE; };
// v3.0 string ops
struct SplitExpr    { ExprPtr str, sep; };         // split str by sep
struct JoinExpr     { ExprPtr arr, sep; };         // join arr with sep
struct TrimExpr     { ExprPtr str; };              // trim str
struct ReplaceExpr  { ExprPtr str, from, to; };    // replace x with y in str
struct IndexOfExpr  { ExprPtr sub, str; };         // index of sub in str
struct UpperExpr    { ExprPtr str; };              // uppercase str
struct LowerExpr    { ExprPtr str; };              // lowercase str
struct SubstrExpr   { ExprPtr str, from, to; };    // chars i to j of str
// v3.0 misc
struct TypeOfExpr   { ExprPtr val; };              // type of x
struct SortExpr     { ExprPtr arr; ExprPtr key; }; // sort arr [by fn]
struct ParseJsonExpr{ ExprPtr str; };              // parse json str
struct JsonOfExpr   { ExprPtr val; };              // json of val
// v3.1 networking + subprocess
struct FetchExpr    { ExprPtr url; ExprPtr opts; };// fetch "url" [with {...}]
struct RunExpr      { ExprPtr cmd; };              // run "cmd"
// ask as expression
struct AskExpr      { ExprPtr prompt; };           // ask "prompt"

struct Expr {
    std::variant<
        NumberLit,StringLit,BoolLit,NullLit,
        ArrayLit,ObjectLit,VarExpr,BinExpr,UnaryExpr,
        IndexExpr,MemberExpr,CallExpr,
        LengthOfExpr,ItemOfExpr,KeepWhereExpr,
        ClassNewExpr,HasExpr,KeysOfExpr,ValuesOfExpr,
        ReadFileExpr,FileExistsExpr,LinesOfFileExpr,
        FuncExpr,TernaryExpr,
        SplitExpr,JoinExpr,TrimExpr,ReplaceExpr,IndexOfExpr,
        UpperExpr,LowerExpr,SubstrExpr,
        TypeOfExpr,SortExpr,ParseJsonExpr,JsonOfExpr,
        FetchExpr,RunExpr,AskExpr
    > node;
};

// ============================================================
//  AST  — Statements
// ============================================================

struct LetStmt      { std::string name; ExprPtr init; };
struct SetStmt      { ExprPtr target; ExprPtr value; };
struct SayStmt      { ExprPtr expr; };
struct AskStmt      { std::string varName; ExprPtr prompt; };
struct PauseStmt    {};
struct IfStmt       { ExprPtr cond; StmtList thenBody, elseBody; };
struct WhileStmt    { ExprPtr cond; StmtList body; };
struct ForStmt      { std::string var; ExprPtr iterable; StmtList body; };
struct BreakStmt    {};
struct ContinueStmt {};
struct ReturnStmt   { ExprPtr value; };
struct FuncStmt     { std::string name; std::vector<std::string> params; StmtList body; };
struct CallStmt     { ExprPtr call; };
struct GetStmt      { std::string path, alias; };
struct ExprStmt     { ExprPtr expr; };
struct AddToStmt    { ExprPtr value; ExprPtr target; }; // target can be obj.field, arr, etc.
// v2.0 new
struct ClassStmt    { std::string name; StmtList body; };
struct TryStmt      { StmtList body; std::string catchVar; StmtList catchBody; };
struct ThrowStmt    { ExprPtr value; };
// v2.0 Scratch-style file I/O statements
struct WriteFileStmt  { ExprPtr content; ExprPtr path; }; // write <content> to file <path>
struct AppendFileStmt { ExprPtr content; ExprPtr path; }; // append <content> to file <path>

struct Stmt {
    std::variant<
        LetStmt,SetStmt,SayStmt,AskStmt,PauseStmt,
        IfStmt,WhileStmt,ForStmt,BreakStmt,ContinueStmt,ReturnStmt,
        FuncStmt,CallStmt,GetStmt,ExprStmt,AddToStmt,
        ClassStmt,TryStmt,ThrowStmt,
        WriteFileStmt,AppendFileStmt
    > node;
};

// ============================================================
//  PARSER
// ============================================================

class Parser {
    std::vector<Token> tokens;
    size_t pos{0};

    Token& peek(int off=0){return tokens[std::min(pos+(size_t)off,tokens.size()-1)];}
    Token  consume(){return tokens[pos++];}
    bool   check(TT t,int off=0){return peek(off).type==t;}
    bool   match(TT t){if(check(t)){pos++;return true;}return false;}
    Token  expect(TT t,const std::string& msg){
        if(!check(t))throw std::runtime_error("Line "+std::to_string(peek().line)+": "+msg+" (got '"+peek().val+"')");
        return consume();
    }
    // Scratch-style contextual keywords can also be used as variable/parameter names
    bool isName(){
        static const std::unordered_set<TT> ctx={
            TT::IDENT,TT::ITEM,TT::ADD,TT::TO,TT::LENGTH,TT::OF,TT::KEEP,
            TT::ITEMS,TT::WHERE,TT::EACH,TT::HAS,TT::KEYS,TT::VALUES,TT::SELF_KW,
            TT::FILE_KW,TT::READ_KW,TT::WRITE_KW,TT::APPEND_KW,
            TT::SPLIT_KW,TT::BY,TT::JOIN_KW,TT::WITH,TT::TRIM_KW,
            TT::REPLACE_KW,TT::INDEX_KW,TT::UPPER_KW,TT::LOWER_KW,TT::CHARS_KW,TT::FROM,
            TT::TYPE_KW,TT::THEN,TT::SORT_KW,TT::JSON_KW,TT::PARSE_KW,
            TT::FETCH_KW,TT::RUN_KW
        };
        return ctx.count(peek().type)>0;
    }
    Token expectName(const std::string& msg){
        if(!isName())throw std::runtime_error("Line "+std::to_string(peek().line)+": "+msg+" (got '"+peek().val+"')");
        return consume();
    }
    void skipNL(){while(check(TT::NEWLINE))pos++;}
    void expectNL(){if(check(TT::NEWLINE)||check(TT::EOF_T)){if(check(TT::NEWLINE))consume();}}

    template<typename T> ExprPtr makeExpr(T t){auto e=std::make_unique<Expr>();e->node=std::move(t);return e;}
    template<typename T> StmtPtr makeStmt(T t){auto s=std::make_unique<Stmt>();s->node=std::move(t);return s;}

    // ---- Expressions ----
    ExprPtr parseExpr(){return parseOr();}
    ExprPtr parseOr(){
        auto l=parseAnd();
        while(check(TT::OR)){consume();auto r=parseAnd();l=makeExpr(BinExpr{"or",std::move(l),std::move(r)});}
        return l;
    }
    ExprPtr parseAnd(){
        auto l=parseEquality();
        while(check(TT::AND)){consume();auto r=parseEquality();l=makeExpr(BinExpr{"and",std::move(l),std::move(r)});}
        return l;
    }
    ExprPtr parseEquality(){
        auto l=parseComparison();
        while(check(TT::EQ)||check(TT::NEQ)){auto op=consume().val;auto r=parseComparison();l=makeExpr(BinExpr{op,std::move(l),std::move(r)});}
        return l;
    }
    ExprPtr parseComparison(){
        auto l=parseAddSub();
        while(check(TT::LT)||check(TT::GT)||check(TT::LEQ)||check(TT::GEQ)){auto op=consume().val;auto r=parseAddSub();l=makeExpr(BinExpr{op,std::move(l),std::move(r)});}
        return l;
    }
    ExprPtr parseAddSub(){
        auto l=parseMulDiv();
        while(check(TT::PLUS)||check(TT::MINUS)){auto op=consume().val;auto r=parseMulDiv();l=makeExpr(BinExpr{op,std::move(l),std::move(r)});}
        return l;
    }
    ExprPtr parseMulDiv(){
        auto l=parseUnary();
        while(check(TT::STAR)||check(TT::SLASH)||check(TT::PERCENT)){auto op=consume().val;auto r=parseUnary();l=makeExpr(BinExpr{op,std::move(l),std::move(r)});}
        return l;
    }
    ExprPtr parseUnary(){
        if(check(TT::MINUS)){consume();return makeExpr(UnaryExpr{"-",parsePostfix()});}
        if(check(TT::NOT))  {consume();return makeExpr(UnaryExpr{"not",parsePostfix()});}
        return parsePostfix();
    }
    ExprPtr parsePostfix(){
        auto expr=parsePrimary();
        while(true){
            if(check(TT::DOT)){
                consume();
                auto name=expect(TT::IDENT,"Expected field name after '.'").val;
                if(check(TT::LPAREN)){
                    consume();
                    std::vector<ExprPtr> args;
                    if(!check(TT::RPAREN)){args.push_back(parseExpr());while(match(TT::COMMA))args.push_back(parseExpr());}
                    expect(TT::RPAREN,"Expected ')'");
                    expr=makeExpr(CallExpr{makeExpr(MemberExpr{std::move(expr),name}),std::move(args)});
                } else {
                    expr=makeExpr(MemberExpr{std::move(expr),name});
                }
            } else if(check(TT::LBRACKET)){
                consume();auto idx=parseExpr();expect(TT::RBRACKET,"Expected ']'");
                expr=makeExpr(IndexExpr{std::move(expr),std::move(idx)});
            } else if(check(TT::LPAREN)){
                consume();
                std::vector<ExprPtr> args;
                if(!check(TT::RPAREN)){args.push_back(parseExpr());while(match(TT::COMMA))args.push_back(parseExpr());}
                expect(TT::RPAREN,"Expected ')'");
                expr=makeExpr(CallExpr{std::move(expr),std::move(args)});
            } else break;
        }
        return expr;
    }
    ExprPtr parsePrimary(){
        // Scratch-style: length of <expr>
        // Only parse as "length of" if immediately followed by OF
        if(check(TT::LENGTH) && check(TT::OF,1)){
            consume();consume(); // consume "length" and "of"
            return makeExpr(LengthOfExpr{parsePostfix()});
        }
        // Scratch-style: item N of <arr>
        // Only parse as "item N of" if followed by a simple expression then OF
        // (avoids conflict with "item" used as a variable name like: add item to list)
        if(check(TT::ITEM)){
            bool nextIsExprStart = (peek(1).type==TT::NUMBER || peek(1).type==TT::IDENT);
            if(nextIsExprStart && check(TT::OF,2)){
                consume();auto idx=parseAddSub();expect(TT::OF,"Expected 'of' after item number");
                return makeExpr(ItemOfExpr{std::move(idx),parsePostfix()});
            }
            // otherwise: treat "item" as a plain variable name
            return makeExpr(VarExpr{consume().val});
        }
        // Scratch-style: keep items in <arr> where <fn>
        if(check(TT::KEEP)){
            consume();expect(TT::ITEMS,"Expected 'items' after 'keep'");
            expect(TT::IN_KW,"Expected 'in' after 'items'");
            auto arr=parsePostfix();expect(TT::WHERE,"Expected 'where'");
            return makeExpr(KeepWhereExpr{std::move(arr),parsePostfix()});
        }
        // v2.0: keys of <dict>
        if(check(TT::KEYS)){
            consume();expect(TT::OF,"Expected 'of' after 'keys'");
            return makeExpr(KeysOfExpr{parsePostfix()});
        }
        // v2.0: values of <dict>
        if(check(TT::VALUES)){
            consume();expect(TT::OF,"Expected 'of' after 'values'");
            return makeExpr(ValuesOfExpr{parsePostfix()});
        }
        // v2.0: has <expr> in <collection>
        if(check(TT::HAS)){
            consume();auto item=parseAddSub();
            expect(TT::IN_KW,"Expected 'in' after value  (usage: has x in myList)");
            return makeExpr(HasExpr{std::move(item),parsePostfix()});
        }
        // v2.0 Scratch-style file I/O expressions
        // read file <path>
        if(check(TT::READ_KW) && check(TT::FILE_KW,1)){
            consume();consume();
            return makeExpr(ReadFileExpr{parsePostfix()});
        }
        // file exists <path>
        if(check(TT::FILE_KW) && peek(1).val=="exists"){
            consume();consume();
            return makeExpr(FileExistsExpr{parsePostfix()});
        }
        // lines of file <path>  (mirrors "length of arr")
        if(check(TT::IDENT) && peek().val=="lines" && check(TT::OF,1) && check(TT::FILE_KW,2)){
            consume();consume();consume();
            return makeExpr(LinesOfFileExpr{parsePostfix()});
        }

        // v3.0: lambda  — function(params) ... end
        if(check(TT::FUNCTION) && check(TT::LPAREN,1)){
            consume(); // "function"
            expect(TT::LPAREN,"Expected '('");
            std::vector<std::string> params;
            if(!check(TT::RPAREN)){params.push_back(expectName("Expected param").val);while(match(TT::COMMA))params.push_back(expectName("Expected param").val);}
            expect(TT::RPAREN,"Expected ')'");expectNL();
            auto body=parseBlock([&]{return check(TT::END);});
            expect(TT::END,"Expected 'end' after function");
            return makeExpr(FuncExpr{std::move(params),std::move(body)});
        }

        // v3.0: ternary  — if cond then expr else expr
        if(check(TT::IF)){
            consume();
            auto cond=parseOr();
            expect(TT::THEN,"Expected 'then' in ternary  (usage: if cond then a else b)");
            auto thenE=parseOr();
            expect(TT::ELSE,"Expected 'else' in ternary");
            auto elseE=parseOr();
            return makeExpr(TernaryExpr{std::move(cond),std::move(thenE),std::move(elseE)});
        }

        // v3.0: split str by sep
        if(check(TT::SPLIT_KW)){
            consume();auto str=parseAddSub();
            expect(TT::BY,"Expected 'by' after string  (usage: split str by sep)");
            return makeExpr(SplitExpr{std::move(str),parseAddSub()});
        }
        // v3.0: join arr with sep
        if(check(TT::JOIN_KW)){
            consume();auto arr=parseAddSub();
            expect(TT::WITH,"Expected 'with' after list  (usage: join list with sep)");
            return makeExpr(JoinExpr{std::move(arr),parseAddSub()});
        }
        // v3.0: trim str
        if(check(TT::TRIM_KW)){consume();return makeExpr(TrimExpr{parsePostfix()});}
        // v3.0: replace x with y in str
        if(check(TT::REPLACE_KW)){
            consume();auto from=parseAddSub();
            expect(TT::WITH,"Expected 'with'  (usage: replace x with y in str)");
            auto to=parseAddSub();
            expect(TT::IN_KW,"Expected 'in'");
            return makeExpr(ReplaceExpr{parseAddSub(),std::move(from),std::move(to)});
        }
        // v3.0: index of sub in str
        if(check(TT::INDEX_KW) && check(TT::OF,1)){
            consume();consume();auto sub=parseAddSub();
            expect(TT::IN_KW,"Expected 'in'  (usage: index of sub in str)");
            return makeExpr(IndexOfExpr{std::move(sub),parseAddSub()});
        }
        // v3.0: uppercase str  (no "of" — matches trim style)
        if(check(TT::UPPER_KW)){consume();return makeExpr(UpperExpr{parsePostfix()});}
        // v3.0: lowercase str
        if(check(TT::LOWER_KW)){consume();return makeExpr(LowerExpr{parsePostfix()});}
        // v3.0: chars i to j of str  (mirrors "item N of list")
        if(check(TT::CHARS_KW)){
            consume();
            auto from=parseAddSub();
            expect(TT::TO,"Expected 'to'  (usage: chars i to j of str)");
            auto to=parseAddSub();
            expect(TT::OF,"Expected 'of'");
            return makeExpr(SubstrExpr{parsePostfix(),std::move(from),std::move(to)});
        }
        // v3.0: type of x
        if(check(TT::TYPE_KW) && check(TT::OF,1)){consume();consume();return makeExpr(TypeOfExpr{parsePostfix()});}
        // v3.0: sort arr / sort arr by field / sort arr by function(x)...end
        if(check(TT::SORT_KW)){
            consume();auto arr=parsePostfix();
            if(!check(TT::BY))return makeExpr(SortExpr{std::move(arr),nullptr});
            consume(); // "by"
            // bare identifier → field name shorthand  e.g.  sort people by age
            if(isName()&&!check(TT::FUNCTION)){
                auto field=consume().val;
                // encode as StringLit so the evaluator knows it's a field key
                return makeExpr(SortExpr{std::move(arr),makeExpr(StringLit{field})});
            }
            // full lambda for computed keys  e.g.  sort people by function(x) return -x.score end
            return makeExpr(SortExpr{std::move(arr),parsePostfix()});
        }
        // v3.0: parse json str
        if(check(TT::PARSE_KW) && check(TT::JSON_KW,1)){consume();consume();return makeExpr(ParseJsonExpr{parsePostfix()});}
        // v3.0: json of val
        if(check(TT::JSON_KW) && check(TT::OF,1)){consume();consume();return makeExpr(JsonOfExpr{parsePostfix()});}
        // v3.1: fetch "url" [with options]
        if(check(TT::FETCH_KW)){
            consume();auto url=parsePostfix();
            ExprPtr opts;
            if(check(TT::WITH)){consume();opts=parsePostfix();}
            return makeExpr(FetchExpr{std::move(url),std::move(opts)});
        }
        // v3.1: run "cmd"
        if(check(TT::RUN_KW)){consume();return makeExpr(RunExpr{parsePostfix()});}
        // v2.0: new ClassName(args)
        if(check(TT::NEW_KW)){
            consume();auto name=expectName("Expected class name after 'new'").val;
            std::vector<ExprPtr> args;
            if(match(TT::LPAREN)){
                if(!check(TT::RPAREN)){args.push_back(parseExpr());while(match(TT::COMMA))args.push_back(parseExpr());}
                expect(TT::RPAREN,"Expected ')'");
            }
            return makeExpr(ClassNewExpr{name,std::move(args)});
        }
        // v2.0: self
        if(check(TT::SELF_KW)){consume();return makeExpr(VarExpr{"self"});}

        // ask as expression: ask "prompt"  or  ask someVar
        if(check(TT::ASK)){
            consume();
            ExprPtr prompt;
            if(!check(TT::NEWLINE)&&!check(TT::EOF_T))prompt=parseAddSub();
            else prompt=makeExpr(StringLit{""});
            return makeExpr(AskExpr{std::move(prompt)});
        }
        if(check(TT::NUMBER)) {auto v=consume().val;return makeExpr(NumberLit{std::stod(v)});}
        if(check(TT::STRING)) {auto v=consume().val;return makeExpr(StringLit{v});}
        if(check(TT::TRUE_KW)){consume();return makeExpr(BoolLit{true});}
        if(check(TT::FALSE_KW)){consume();return makeExpr(BoolLit{false});}
        if(check(TT::NULL_KW)){consume();return makeExpr(NullLit{});}
        if(check(TT::IDENT))  {return makeExpr(VarExpr{consume().val});}
        if(check(TT::LPAREN)) {consume();auto e=parseExpr();expect(TT::RPAREN,"Expected ')'");return e;}
        if(check(TT::LBRACKET)){
            consume();std::vector<ExprPtr> elems;skipNL();
            if(!check(TT::RBRACKET)){elems.push_back(parseExpr());while(match(TT::COMMA)){skipNL();elems.push_back(parseExpr());}}
            skipNL();expect(TT::RBRACKET,"Expected ']'");
            return makeExpr(ArrayLit{std::move(elems)});
        }
        if(check(TT::LBRACE)){
            consume();std::vector<std::pair<std::string,ExprPtr>> pairs;skipNL();
            if(!check(TT::RBRACE)){
                auto k=expect(TT::IDENT,"Expected key").val;expect(TT::COLON,"Expected ':'");
                auto v=parseExpr();pairs.push_back({k,std::move(v)});
                while(match(TT::COMMA)){skipNL();auto k2=expect(TT::IDENT,"Expected key").val;expect(TT::COLON,"Expected ':'");pairs.push_back({k2,parseExpr()});}
            }
            skipNL();expect(TT::RBRACE,"Expected '}'");
            return makeExpr(ObjectLit{std::move(pairs)});
        }
        throw std::runtime_error("Line "+std::to_string(peek().line)+": Unexpected token '"+peek().val+"'");
    }

    // ---- Statements ----
    StmtList parseBlock(std::function<bool()> end){
        StmtList s;skipNL();
        while(!end()&&!check(TT::EOF_T)){s.push_back(parseStmt());skipNL();}
        return s;
    }

    StmtPtr parseStmt(){
        skipNL();
        switch(peek().type){
            case TT::LET:{
                consume();auto name=expectName("Expected variable name").val;
                ExprPtr init;if(match(TT::ASSIGN))init=parseExpr();else init=makeExpr(NullLit{});
                expectNL();return makeStmt(LetStmt{name,std::move(init)});
            }
            case TT::SET:{
                consume();auto target=parsePostfix();expect(TT::ASSIGN,"Expected '='");
                auto val=parseExpr();expectNL();return makeStmt(SetStmt{std::move(target),std::move(val)});
            }
            case TT::ADD:{
                consume();auto val=parseExpr();
                expect(TT::TO,"Expected 'to' after value  (usage: add x to myList)");
                auto target=parsePostfix(); // supports: add x to list, add x to obj.field
                expectNL();return makeStmt(AddToStmt{std::move(val),std::move(target)});
            }
            case TT::SAY:{consume();auto e=parseExpr();expectNL();return makeStmt(SayStmt{std::move(e)});}
            case TT::ASK:{
                consume();auto name=expectName("Expected variable name").val;
                ExprPtr prompt;
                if(!check(TT::NEWLINE)&&!check(TT::EOF_T))prompt=parseExpr();
                else prompt=makeExpr(StringLit{""});
                expectNL();return makeStmt(AskStmt{name,std::move(prompt)});
            }
            case TT::PAUSE:{consume();expectNL();return makeStmt(PauseStmt{});}
            case TT::IF:{
                consume();auto cond=parseExpr();expectNL();
                auto then=parseBlock([&]{return check(TT::ELSE)||check(TT::END);});
                StmtList els;if(match(TT::ELSE)){expectNL();els=parseBlock([&]{return check(TT::END);});}
                expect(TT::END,"Expected 'end' after if");expectNL();
                return makeStmt(IfStmt{std::move(cond),std::move(then),std::move(els)});
            }
            case TT::WHILE:{
                consume();auto cond=parseExpr();expectNL();
                auto body=parseBlock([&]{return check(TT::END);});
                expect(TT::END,"Expected 'end' after while");expectNL();
                return makeStmt(WhileStmt{std::move(cond),std::move(body)});
            }
            case TT::FOR:{
                consume();match(TT::EACH);
                auto var=expectName("Expected variable name").val;
                expect(TT::IN_KW,"Expected 'in'");auto iter=parseExpr();expectNL();
                auto body=parseBlock([&]{return check(TT::END);});
                expect(TT::END,"Expected 'end' after for");expectNL();
                return makeStmt(ForStmt{var,std::move(iter),std::move(body)});
            }
            case TT::BREAK:{consume();expectNL();return makeStmt(BreakStmt{});}
            case TT::CONTINUE:{consume();expectNL();return makeStmt(ContinueStmt{});}
            case TT::RETURN:{
                consume();ExprPtr val;
                if(!check(TT::NEWLINE)&&!check(TT::EOF_T))val=parseExpr();else val=makeExpr(NullLit{});
                expectNL();return makeStmt(ReturnStmt{std::move(val)});
            }
            case TT::FUNCTION:{
                // function(params) → lambda expression used as a statement value
                if(check(TT::LPAREN,1))break; // fall to default → parseExpr
                consume();auto name=expectName("Expected function name").val;
                expect(TT::LPAREN,"Expected '('");
                std::vector<std::string> params;
                if(!check(TT::RPAREN)){params.push_back(expectName("Expected param").val);while(match(TT::COMMA))params.push_back(expectName("Expected param").val);}
                expect(TT::RPAREN,"Expected ')'");expectNL();
                auto body=parseBlock([&]{return check(TT::END);});
                expect(TT::END,"Expected 'end' after function");expectNL();
                return makeStmt(FuncStmt{name,params,std::move(body)});
            }
            case TT::CALL:{consume();auto e=parseExpr();expectNL();return makeStmt(CallStmt{std::move(e)});}
            case TT::GET:{
                consume();auto path=expect(TT::STRING,"Expected module path").val;
                expect(TT::AS,"Expected 'as'");auto alias=expectName("Expected alias").val;
                expectNL();return makeStmt(GetStmt{path,alias});
            }
            // v2.0: class
            case TT::CLASS:{
                consume();auto name=expectName("Expected class name").val;expectNL();
                auto body=parseBlock([&]{return check(TT::END);});
                expect(TT::END,"Expected 'end' after class");expectNL();
                return makeStmt(ClassStmt{name,std::move(body)});
            }
            // v2.0: try/catch
            case TT::TRY:{
                consume();expectNL();
                auto body=parseBlock([&]{return check(TT::CATCH);});
                expect(TT::CATCH,"Expected 'catch' after try block");
                auto errVar=expectName("Expected error variable name after 'catch'").val;
                expectNL();
                auto catchBody=parseBlock([&]{return check(TT::END);});
                expect(TT::END,"Expected 'end' after catch");expectNL();
                return makeStmt(TryStmt{std::move(body),errVar,std::move(catchBody)});
            }
            // v2.0: throw
            case TT::THROW:{
                consume();auto val=parseExpr();expectNL();
                return makeStmt(ThrowStmt{std::move(val)});
            }
            // v2.0 Scratch-style: write <content> to file <path>
            case TT::WRITE_KW:{
                consume();auto content=parseExpr();
                expect(TT::TO,"Expected 'to' after content  (usage: write \"text\" to file \"name.txt\")");
                expect(TT::FILE_KW,"Expected 'file' after 'to'");
                auto path=parseExpr();expectNL();
                return makeStmt(WriteFileStmt{std::move(content),std::move(path)});
            }
            // v2.0 Scratch-style: append <content> to file <path>
            case TT::APPEND_KW:{
                consume();auto content=parseExpr();
                expect(TT::TO,"Expected 'to' after content  (usage: append \"text\" to file \"name.txt\")");
                expect(TT::FILE_KW,"Expected 'file' after 'to'");
                auto path=parseExpr();expectNL();
                return makeStmt(AppendFileStmt{std::move(content),std::move(path)});
            }
            default:{auto e=parseExpr();expectNL();return makeStmt(ExprStmt{std::move(e)});}
        }
        // Reached only when a case breaks instead of returns (e.g. lambda fallthrough)
        auto e=parseExpr();expectNL();return makeStmt(ExprStmt{std::move(e)});
    }
public:
    Parser(std::vector<Token> toks):tokens(std::move(toks)){}
    StmtList parse(){
        StmtList p;skipNL();
        while(!check(TT::EOF_T)){p.push_back(parseStmt());skipNL();}
        return p;
    }
};

// ============================================================
//  VALUES
// ============================================================

struct IronValue;
using ValuePtr   = std::shared_ptr<IronValue>;
using IronArray  = std::vector<ValuePtr>;
using IronObject = std::unordered_map<std::string,ValuePtr>;
struct IronFunc  { std::vector<std::string> params; const StmtList* body; struct Env* closure; };
using NativeFunc = std::function<ValuePtr(std::vector<ValuePtr>)>;

struct IronValue {
    std::variant<std::nullptr_t,bool,double,std::string,
                 std::shared_ptr<IronArray>,std::shared_ptr<IronObject>,
                 IronFunc,NativeFunc> data;

    static ValuePtr makeNull()             {auto v=std::make_shared<IronValue>();v->data=nullptr;return v;}
    static ValuePtr makeBool(bool b)       {auto v=std::make_shared<IronValue>();v->data=b;return v;}
    static ValuePtr makeNum(double d)      {auto v=std::make_shared<IronValue>();v->data=d;return v;}
    static ValuePtr makeStr(std::string s) {auto v=std::make_shared<IronValue>();v->data=std::move(s);return v;}
    static ValuePtr makeArr(std::shared_ptr<IronArray> a){auto v=std::make_shared<IronValue>();v->data=a;return v;}
    static ValuePtr makeObj(std::shared_ptr<IronObject> o){auto v=std::make_shared<IronValue>();v->data=o;return v;}
    static ValuePtr makeFunc(IronFunc f)   {auto v=std::make_shared<IronValue>();v->data=f;return v;}
    static ValuePtr makeNative(NativeFunc f){auto v=std::make_shared<IronValue>();v->data=f;return v;}

    bool isTruthy() const {
        if(std::get_if<std::nullptr_t>(&data))return false;
        if(auto*b=std::get_if<bool>(&data))return *b;
        if(auto*n=std::get_if<double>(&data))return *n!=0.0;
        if(auto*s=std::get_if<std::string>(&data))return !s->empty();
        return true;
    }
    std::string toString() const {
        if(std::get_if<std::nullptr_t>(&data))return "null";
        if(auto*b=std::get_if<bool>(&data))return *b?"true":"false";
        if(auto*n=std::get_if<double>(&data)){
            if(*n==std::floor(*n)&&std::abs(*n)<1e15)return std::to_string((long long)*n);
            std::ostringstream oss;oss<<*n;return oss.str();
        }
        if(auto*s=std::get_if<std::string>(&data))return *s;
        if(auto*ap=std::get_if<std::shared_ptr<IronArray>>(&data)){
            std::string s="[";
            for(size_t i=0;i<(*ap)->size();i++){if(i)s+=",";s+=(**ap)[i]->toString();}
            return s+"]";
        }
        if(auto*op=std::get_if<std::shared_ptr<IronObject>>(&data)){
            // Check if it's a class instance
            auto ci=(*op)->find("__class__");
            if(ci!=(*op)->end()){
                auto cn=ci->second->toString();
                std::string s=cn+"{ ";bool first=true;
                for(auto&[k,v]:**op){if(k=="__class__")continue;if(!first)s+=", ";s+=k+": "+v->toString();first=false;}
                return s+" }";
            }
            std::string s="{";bool first=true;
            for(auto&[k,v]:**op){if(!first)s+=",";s+=k+":"+v->toString();first=false;}
            return s+"}";
        }
        if(std::get_if<IronFunc>(&data)||std::get_if<NativeFunc>(&data))return "<function>";
        return "null";
    }
};

// ============================================================
//  ENVIRONMENT
// ============================================================

struct Env {
    std::unordered_map<std::string,ValuePtr> vars;
    Env* parent{nullptr};
    ValuePtr get(const std::string& n) const {
        auto it=vars.find(n);if(it!=vars.end())return it->second;
        if(parent)return parent->get(n);
        throw std::runtime_error("I don't know what '"+n+"' is — did you forget 'let "+n+" = ...'?");
    }
    void define(const std::string& n,ValuePtr v){vars[n]=v;}
    void assign(const std::string& n,ValuePtr v){
        auto it=vars.find(n);if(it!=vars.end()){it->second=v;return;}
        if(parent){parent->assign(n,v);return;}
        throw std::runtime_error("Can't change '"+n+"' — use 'let "+n+" = ...' to create it first.");
    }
};

// ============================================================
//  CONTROL FLOW SIGNALS
// ============================================================

struct ReturnSignal   { ValuePtr value; };
struct BreakSignal    {};
struct ContinueSignal {};
struct ThrowSignal    { std::string message; };  // v2.0 user throws

// ============================================================
//  CLASS REGISTRY  (v2.0)
// ============================================================

struct ClassDef {
    std::string name;
    std::vector<std::pair<std::string,const Expr*>> fields;  // name → default expr (raw ptr into AST)
    std::unordered_map<std::string,IronFunc> methods;
    Env* definitionEnv{nullptr};
};

// ============================================================
//  INTERPRETER
// ============================================================

class Interpreter {
    Env globalEnv;
    std::unordered_map<std::string,ClassDef> classRegistry;
    std::list<Env> moduleEnvs;  // keeps module envs alive so function closures don't dangle
    std::list<StmtList> moduleAsts; // keeps module ASTs alive so IronFunc body ptrs don't dangle

    // ---- String interpolation ----
    std::string interpolate(const std::string& s,Env& env){
        std::string result;size_t i=0;
        while(i<s.size()){
            if(s[i]=='{'){
                size_t j=i+1;int depth=1;
                while(j<s.size()&&depth>0){if(s[j]=='{')depth++;else if(s[j]=='}')depth--;if(depth>0)j++;}
                std::string inner=s.substr(i+1,j-i-1);
                Lexer lex(inner);auto toks=lex.tokenize();
                Parser p(std::move(toks));auto stmts=p.parse();
                if(!stmts.empty())if(auto*es=std::get_if<ExprStmt>(&stmts[0]->node))result+=evalExpr(*es->expr,env)->toString();
                i=j+1;
            } else result+=s[i++];
        }
        return result;
    }

    // ---- Call a method on a class instance ----
    ValuePtr callMethod(ValuePtr instance,const IronFunc& method,std::vector<ValuePtr> args){
        Env me;me.parent=method.closure;
        me.define("self",instance);
        for(size_t i=0;i<method.params.size();i++)
            me.define(method.params[i],i<args.size()?args[i]:IronValue::makeNull());
        try{execBlock(*method.body,me);}catch(ReturnSignal&r){return r.value;}
        return IronValue::makeNull();
    }

    // ---- Evaluate expression ----
    ValuePtr evalExpr(const Expr& expr,Env& env){
        return std::visit([&](auto& node)->ValuePtr{
            using T=std::decay_t<decltype(node)>;

            if constexpr(std::is_same_v<T,NumberLit>) return IronValue::makeNum(node.value);
            if constexpr(std::is_same_v<T,BoolLit>)   return IronValue::makeBool(node.value);
            if constexpr(std::is_same_v<T,NullLit>)   return IronValue::makeNull();
            if constexpr(std::is_same_v<T,StringLit>)  return IronValue::makeStr(interpolate(node.value,env));
            if constexpr(std::is_same_v<T,VarExpr>)    return env.get(node.name);

            if constexpr(std::is_same_v<T,ArrayLit>){
                auto arr=std::make_shared<IronArray>();
                for(auto&e:node.elems)arr->push_back(evalExpr(*e,env));
                return IronValue::makeArr(arr);
            }
            if constexpr(std::is_same_v<T,ObjectLit>){
                auto obj=std::make_shared<IronObject>();
                for(auto&[k,v]:node.pairs)(*obj)[k]=evalExpr(*v,env);
                return IronValue::makeObj(obj);
            }

            // ---- length of ----
            if constexpr(std::is_same_v<T,LengthOfExpr>){
                auto val=evalExpr(*node.arr,env);
                if(auto*ap=std::get_if<std::shared_ptr<IronArray>>(&val->data))return IronValue::makeNum((*ap)->size());
                if(auto*sp=std::get_if<std::string>(&val->data))return IronValue::makeNum(sp->size());
                throw std::runtime_error("'length of' works on lists and text, not "+val->toString());
            }
            // ---- item N of (1-indexed) ----
            if constexpr(std::is_same_v<T,ItemOfExpr>){
                auto iv=evalExpr(*node.index,env);auto av=evalExpr(*node.arr,env);
                if(auto*n=std::get_if<double>(&iv->data)){
                    int i=(int)*n-1;
                    if(auto*ap=std::get_if<std::shared_ptr<IronArray>>(&av->data)){
                        if(i<0||i>=(int)(*ap)->size())throw std::runtime_error("Item "+std::to_string((int)*n)+" is out of bounds — the list has "+std::to_string((*ap)->size())+" items.");
                        return (**ap)[i];
                    }
                }
                return IronValue::makeNull();
            }
            // ---- keep items in ... where ----
            if constexpr(std::is_same_v<T,KeepWhereExpr>){
                auto av=evalExpr(*node.arr,env);auto fn=evalExpr(*node.fn,env);
                if(auto*ap=std::get_if<std::shared_ptr<IronArray>>(&av->data)){
                    auto res=std::make_shared<IronArray>();
                    for(auto&item:**ap)if(callValue(fn,{item})->isTruthy())res->push_back(item);
                    return IronValue::makeArr(res);
                }
                throw std::runtime_error("'keep items in' expects a list");
            }

            // ---- v2.0: keys of ----
            if constexpr(std::is_same_v<T,KeysOfExpr>){
                auto val=evalExpr(*node.dict,env);
                if(auto*op=std::get_if<std::shared_ptr<IronObject>>(&val->data)){
                    auto arr=std::make_shared<IronArray>();
                    for(auto&[k,v]:**op)if(k!="__class__")arr->push_back(IronValue::makeStr(k));
                    return IronValue::makeArr(arr);
                }
                throw std::runtime_error("'keys of' expects an object/dictionary");
            }
            // ---- v2.0: values of ----
            if constexpr(std::is_same_v<T,ValuesOfExpr>){
                auto val=evalExpr(*node.dict,env);
                if(auto*op=std::get_if<std::shared_ptr<IronObject>>(&val->data)){
                    auto arr=std::make_shared<IronArray>();
                    for(auto&[k,v]:**op)if(k!="__class__")arr->push_back(v);
                    return IronValue::makeArr(arr);
                }
                throw std::runtime_error("'values of' expects an object/dictionary");
            }
            // ---- v2.0: has x in collection ----
            if constexpr(std::is_same_v<T,HasExpr>){
                auto item=evalExpr(*node.item,env);
                auto coll=evalExpr(*node.collection,env);
                if(auto*ap=std::get_if<std::shared_ptr<IronArray>>(&coll->data)){
                    for(auto&elem:**ap)if(elem->toString()==item->toString())return IronValue::makeBool(true);
                    return IronValue::makeBool(false);
                }
                if(auto*op=std::get_if<std::shared_ptr<IronObject>>(&coll->data)){
                    std::string key=item->toString();
                    return IronValue::makeBool((*op)->find(key)!=(*op)->end()&&key!="__class__");
                }
                if(auto*sp=std::get_if<std::string>(&coll->data)){
                    return IronValue::makeBool(sp->find(item->toString())!=std::string::npos);
                }
                return IronValue::makeBool(false);
            }
            // ---- v2.0 Scratch-style: read file <path> ----
            if constexpr(std::is_same_v<T,ReadFileExpr>){
                auto p=evalExpr(*node.path,env)->toString();
                std::ifstream f(p);
                if(!f)throw ThrowSignal{"Can't open file: "+p};
                return IronValue::makeStr({std::istreambuf_iterator<char>(f),{}});
            }
            // ---- v2.0 Scratch-style: file exists <path> ----
            if constexpr(std::is_same_v<T,FileExistsExpr>){
                auto p=evalExpr(*node.path,env)->toString();
                return IronValue::makeBool(std::ifstream(p).good());
            }
            // ---- v2.0 Scratch-style: lines of file <path> ----
            if constexpr(std::is_same_v<T,LinesOfFileExpr>){
                auto p=evalExpr(*node.path,env)->toString();
                std::ifstream f(p);
                if(!f)throw ThrowSignal{"Can't open file: "+p};
                auto arr=std::make_shared<IronArray>();
                std::string ln;
                while(std::getline(f,ln))arr->push_back(IronValue::makeStr(ln));
                return IronValue::makeArr(arr);
            }
            // ---- v2.0: new ClassName(args) ----
            if constexpr(std::is_same_v<T,ClassNewExpr>){
                auto it=classRegistry.find(node.className);
                if(it==classRegistry.end())throw std::runtime_error("Unknown class: "+node.className+" — did you define it with 'class "+node.className+"'?");
                auto& cd=it->second;
                auto fields=std::make_shared<IronObject>();
                (*fields)["__class__"]=IronValue::makeStr(node.className);
                // Initialize default field values
                for(auto&[fname,defaultExpr]:cd.fields){
                    if(defaultExpr)(*fields)[fname]=evalExpr(*defaultExpr,*cd.definitionEnv);
                    else(*fields)[fname]=IronValue::makeNull();
                }
                auto instance=IronValue::makeObj(fields);
                // Call init if it exists
                auto initIt=cd.methods.find("init");
                if(initIt!=cd.methods.end()){
                    std::vector<ValuePtr> args;
                    for(auto&a:node.args)args.push_back(evalExpr(*a,env));
                    callMethod(instance,initIt->second,args);
                }
                return instance;
            }

            if constexpr(std::is_same_v<T,UnaryExpr>){
                auto v=evalExpr(*node.operand,env);
                if(node.op=="-"&&std::get_if<double>(&v->data))return IronValue::makeNum(-std::get<double>(v->data));
                if(node.op=="not")return IronValue::makeBool(!v->isTruthy());
                return IronValue::makeNull();
            }
            if constexpr(std::is_same_v<T,BinExpr>){
                if(node.op=="and"){auto l=evalExpr(*node.left,env);return l->isTruthy()?evalExpr(*node.right,env):l;}
                if(node.op=="or") {auto l=evalExpr(*node.left,env);return l->isTruthy()?l:evalExpr(*node.right,env);}
                auto left=evalExpr(*node.left,env);auto right=evalExpr(*node.right,env);
                const std::string& op=node.op;
                auto*ln=std::get_if<double>(&left->data);auto*rn=std::get_if<double>(&right->data);
                if(op=="+"){if(ln&&rn)return IronValue::makeNum(*ln+*rn);return IronValue::makeStr(left->toString()+right->toString());}
                if(ln&&rn){
                    if(op=="-")return IronValue::makeNum(*ln-*rn);
                    if(op=="*")return IronValue::makeNum(*ln**rn);
                    if(op=="/"){if(*rn==0)throw std::runtime_error("Can't divide by zero!");return IronValue::makeNum(*ln/ *rn);}
                    if(op=="%")return IronValue::makeNum(std::fmod(*ln,*rn));
                    if(op=="<")return IronValue::makeBool(*ln<*rn);if(op==">")return IronValue::makeBool(*ln>*rn);
                    if(op=="<=")return IronValue::makeBool(*ln<=*rn);if(op==">=")return IronValue::makeBool(*ln>=*rn);
                }
                if(op=="==")return IronValue::makeBool(left->toString()==right->toString());
                if(op=="!=")return IronValue::makeBool(left->toString()!=right->toString());
                return IronValue::makeNull();
            }

            // ---- Member access (obj.field) — handles class instances ----
            if constexpr(std::is_same_v<T,MemberExpr>){
                auto obj=evalExpr(*node.obj,env);
                if(auto*ap=std::get_if<std::shared_ptr<IronArray>>(&obj->data)){
                    if(node.field=="length")return IronValue::makeNum((*ap)->size());
                    if(node.field=="map"){
                        return IronValue::makeNative([this,ap](std::vector<ValuePtr> args)->ValuePtr{
                            auto res=std::make_shared<IronArray>();
                            for(auto&item:**ap)res->push_back(callValue(args[0],{item}));
                            return IronValue::makeArr(res);
                        });
                    }
                }
                if(auto*op=std::get_if<std::shared_ptr<IronObject>>(&obj->data)){
                    // Check if it's a class instance — try methods from class registry
                    auto classMarker=(*op)->find("__class__");
                    if(classMarker!=(*op)->end()){
                        auto*cn=std::get_if<std::string>(&classMarker->second->data);
                        if(cn){
                            auto regIt=classRegistry.find(*cn);
                            if(regIt!=classRegistry.end()){
                                auto methodIt=regIt->second.methods.find(node.field);
                                if(methodIt!=regIt->second.methods.end()){
                                    // Return a bound method (captures instance and method)
                                    auto capturedObj=obj;
                                    auto capturedMethod=methodIt->second;
                                    return IronValue::makeNative([this,capturedObj,capturedMethod](std::vector<ValuePtr> args)->ValuePtr{
                                        return callMethod(capturedObj,capturedMethod,args);
                                    });
                                }
                            }
                        }
                    }
                    // Regular field access
                    if(node.field!="__class__"){
                        auto it=(*op)->find(node.field);
                        if(it!=(*op)->end())return it->second;
                    }
                    return IronValue::makeNull();
                }
                throw std::runtime_error("Can't access '."+node.field+"' on that value.");
            }

            if constexpr(std::is_same_v<T,IndexExpr>){
                auto obj=evalExpr(*node.obj,env);auto idx=evalExpr(*node.index,env);
                if(auto*ap=std::get_if<std::shared_ptr<IronArray>>(&obj->data)){
                    if(auto*n=std::get_if<double>(&idx->data)){int i=(int)*n;if(i>=0&&i<(int)(*ap)->size())return (**ap)[i];}
                    return IronValue::makeNull();
                }
                if(auto*op=std::get_if<std::shared_ptr<IronObject>>(&obj->data)){
                    auto it=(*op)->find(idx->toString());return it!=(*op)->end()?it->second:IronValue::makeNull();
                }
                return IronValue::makeNull();
            }
            if constexpr(std::is_same_v<T,CallExpr>){
                auto callee=evalExpr(*node.callee,env);
                std::vector<ValuePtr> args;for(auto&a:node.args)args.push_back(evalExpr(*a,env));
                return callValue(callee,args);
            }

            // ---- v3.0: lambda ----
            if constexpr(std::is_same_v<T,FuncExpr>){
                IronFunc f{node.params,&node.body,&env};
                return IronValue::makeFunc(f);
            }
            // ---- v3.0: ternary ----
            if constexpr(std::is_same_v<T,TernaryExpr>){
                return evalExpr(*node.cond,env)->isTruthy() ? evalExpr(*node.thenE,env) : evalExpr(*node.elseE,env);
            }
            // ---- v3.0: string ops ----
            if constexpr(std::is_same_v<T,SplitExpr>){
                auto s=evalExpr(*node.str,env)->toString();
                auto sep=evalExpr(*node.sep,env)->toString();
                auto arr=std::make_shared<IronArray>();
                if(sep.empty()){for(char c:s)arr->push_back(IronValue::makeStr(std::string(1,c)));return IronValue::makeArr(arr);}
                size_t p=0,f;
                while((f=s.find(sep,p))!=std::string::npos){arr->push_back(IronValue::makeStr(s.substr(p,f-p)));p=f+sep.size();}
                arr->push_back(IronValue::makeStr(s.substr(p)));
                return IronValue::makeArr(arr);
            }
            if constexpr(std::is_same_v<T,JoinExpr>){
                auto sep=evalExpr(*node.sep,env)->toString();
                auto av=evalExpr(*node.arr,env);
                if(auto*ap=std::get_if<std::shared_ptr<IronArray>>(&av->data)){
                    std::string out;for(size_t i=0;i<(*ap)->size();i++){if(i)out+=sep;out+=(**ap)[i]->toString();}
                    return IronValue::makeStr(out);
                }
                return IronValue::makeStr(av->toString());
            }
            if constexpr(std::is_same_v<T,TrimExpr>){
                auto s=evalExpr(*node.str,env)->toString();
                size_t a=s.find_first_not_of(" \t\n\r"),b=s.find_last_not_of(" \t\n\r");
                return IronValue::makeStr(a==std::string::npos?"":s.substr(a,b-a+1));
            }
            if constexpr(std::is_same_v<T,ReplaceExpr>){
                auto s=evalExpr(*node.str,env)->toString();
                auto from=evalExpr(*node.from,env)->toString();
                auto to=evalExpr(*node.to,env)->toString();
                if(from.empty())return IronValue::makeStr(s);
                std::string out;size_t p=0,f;
                while((f=s.find(from,p))!=std::string::npos){out+=s.substr(p,f-p)+to;p=f+from.size();}
                return IronValue::makeStr(out+s.substr(p));
            }
            if constexpr(std::is_same_v<T,IndexOfExpr>){
                auto s=evalExpr(*node.str,env)->toString();
                auto sub=evalExpr(*node.sub,env)->toString();
                auto pos=s.find(sub);
                return IronValue::makeNum(pos==std::string::npos?-1.0:(double)pos);
            }
            if constexpr(std::is_same_v<T,UpperExpr>){
                auto s=evalExpr(*node.str,env)->toString();
                std::transform(s.begin(),s.end(),s.begin(),::toupper);
                return IronValue::makeStr(s);
            }
            if constexpr(std::is_same_v<T,LowerExpr>){
                auto s=evalExpr(*node.str,env)->toString();
                std::transform(s.begin(),s.end(),s.begin(),::tolower);
                return IronValue::makeStr(s);
            }
            if constexpr(std::is_same_v<T,SubstrExpr>){
                auto s=evalExpr(*node.str,env)->toString();
                int from=(int)std::get<double>(evalExpr(*node.from,env)->data);
                int to=(int)std::get<double>(evalExpr(*node.to,env)->data);
                if(from<0)from=0;if(to>(int)s.size())to=(int)s.size();
                return IronValue::makeStr(from>=to?"":s.substr(from,to-from));
            }
            // ---- v3.0: type of ----
            if constexpr(std::is_same_v<T,TypeOfExpr>){
                auto v=evalExpr(*node.val,env);
                if(std::get_if<std::nullptr_t>(&v->data))return IronValue::makeStr("null");
                if(std::get_if<bool>(&v->data))return IronValue::makeStr("bool");
                if(std::get_if<double>(&v->data))return IronValue::makeStr("number");
                if(std::get_if<std::string>(&v->data))return IronValue::makeStr("string");
                if(std::get_if<std::shared_ptr<IronArray>>(&v->data))return IronValue::makeStr("list");
                if(std::get_if<std::shared_ptr<IronObject>>(&v->data))return IronValue::makeStr("dict");
                if(std::get_if<IronFunc>(&v->data)||std::get_if<NativeFunc>(&v->data))return IronValue::makeStr("function");
                return IronValue::makeStr("unknown");
            }
            // ---- v3.0: sort ----
            if constexpr(std::is_same_v<T,SortExpr>){
                auto av=evalExpr(*node.arr,env);
                if(auto*ap=std::get_if<std::shared_ptr<IronArray>>(&av->data)){
                    auto copy=std::make_shared<IronArray>(**ap);
                    if(node.key){
                        // Field name shorthand: key is a StringLit (not callable) → extract field
                        auto keyVal=evalExpr(*node.key,env);
                        if(auto*field=std::get_if<std::string>(&keyVal->data)){
                            // sort people by age  →  key is the string "age"
                            std::stable_sort(copy->begin(),copy->end(),[&](const ValuePtr&a,const ValuePtr&b){
                                ValuePtr ka=IronValue::makeNull(),kb=IronValue::makeNull();
                                if(auto*oa=std::get_if<std::shared_ptr<IronObject>>(&a->data)){auto it=(*oa)->find(*field);if(it!=(*oa)->end())ka=it->second;}
                                if(auto*ob=std::get_if<std::shared_ptr<IronObject>>(&b->data)){auto it=(*ob)->find(*field);if(it!=(*ob)->end())kb=it->second;}
                                auto*na=std::get_if<double>(&ka->data),*nb=std::get_if<double>(&kb->data);
                                if(na&&nb)return *na<*nb;
                                return ka->toString()<kb->toString();
                            });
                        } else {
                            // Lambda key: sort people by function(x) return x.score end
                            std::stable_sort(copy->begin(),copy->end(),[&](const ValuePtr&a,const ValuePtr&b){
                                auto ka=callValue(keyVal,{a}),kb=callValue(keyVal,{b});
                                auto*na=std::get_if<double>(&ka->data),*nb=std::get_if<double>(&kb->data);
                                if(na&&nb)return *na<*nb;
                                return ka->toString()<kb->toString();
                            });
                        }
                    } else {
                        std::stable_sort(copy->begin(),copy->end(),[](const ValuePtr&a,const ValuePtr&b){
                            auto*na=std::get_if<double>(&a->data),*nb=std::get_if<double>(&b->data);
                            if(na&&nb)return *na<*nb;
                            return a->toString()<b->toString();
                        });
                    }
                    return IronValue::makeArr(copy);
                }
                return av;
            }
            // ---- v3.0: json of / parse json ----
            if constexpr(std::is_same_v<T,JsonOfExpr>){
                return IronValue::makeStr(ironToJson(evalExpr(*node.val,env)));
            }
            if constexpr(std::is_same_v<T,ParseJsonExpr>){
                auto s=evalExpr(*node.str,env)->toString();
                size_t p=0;return jsonToIron(s,p);
            }
            // ---- v3.1: fetch / run ----
            if constexpr(std::is_same_v<T,FetchExpr>) return evalFetch(node,env);
            if constexpr(std::is_same_v<T,RunExpr>)   return evalRun(node,env);
            if constexpr(std::is_same_v<T,AskExpr>){
                std::string prompt=evalExpr(*node.prompt,env)->toString();
                if(!prompt.empty())std::cout<<prompt<<" ";
                std::string input;std::getline(std::cin,input);
                return IronValue::makeStr(input);
            }

            return IronValue::makeNull();
        },expr.node);
    }

    // ================================================================
    //  v3.1 — HTTP engine (raw sockets, no libcurl dependency)
    // ================================================================
    struct ParsedUrl {
        std::string scheme,host,path;
        int port;
    };
    static ParsedUrl parseUrl(const std::string& url){
        ParsedUrl p;
        size_t s=url.find("://");
        if(s==std::string::npos)throw std::runtime_error("Bad URL: "+url);
        p.scheme=url.substr(0,s);
        std::string rest=url.substr(s+3);
        size_t sl=rest.find('/');
        std::string hostport=sl==std::string::npos?rest:rest.substr(0,sl);
        p.path=sl==std::string::npos?"/":rest.substr(sl);
        size_t col=hostport.find(':');
        if(col!=std::string::npos){p.host=hostport.substr(0,col);p.port=std::stoi(hostport.substr(col+1));}
        else{p.host=hostport;p.port=(p.scheme=="https"?443:80);}
        return p;
    }
    struct HttpResponse { std::string body; int status; };
    static HttpResponse httpRequest(const std::string& method,const std::string& rawUrl,
                                    const std::string& body,const std::unordered_map<std::string,std::string>& extraHeaders,
                                    int redirectsLeft=8){
        if(redirectsLeft==0)throw std::runtime_error("Too many HTTP redirects");
        auto u=parseUrl(rawUrl);
        if(u.scheme=="https")throw std::runtime_error("HTTPS requires libcurl — use http:// or install libcurl4-openssl-dev");
        // open socket
        struct addrinfo hints{},*res=nullptr;
        hints.ai_family=AF_INET;hints.ai_socktype=SOCK_STREAM;
        std::string portStr=std::to_string(u.port);
        if(getaddrinfo(u.host.c_str(),portStr.c_str(),&hints,&res)!=0||!res)
            throw std::runtime_error("Can't resolve host: "+u.host);
        int fd=socket(res->ai_family,res->ai_socktype,res->ai_protocol);
        if(fd<0){freeaddrinfo(res);throw std::runtime_error("Socket error");}
        if(connect(fd,res->ai_addr,res->ai_addrlen)<0){
            freeaddrinfo(res);close(fd);
            throw std::runtime_error("Can't connect to "+u.host+":"+portStr);
        }
        freeaddrinfo(res);
        // build request
        std::string req=method+" "+u.path+" HTTP/1.1\r\n";
        req+="Host: "+u.host+"\r\n";
        req+="Connection: close\r\n";
        req+="User-Agent: Ironwood/3.1\r\n";
        for(auto&[k,v]:extraHeaders)req+=k+": "+v+"\r\n";
        if(!body.empty()){
            req+="Content-Length: "+std::to_string(body.size())+"\r\n";
            if(extraHeaders.find("Content-Type")==extraHeaders.end())
                req+="Content-Type: application/x-www-form-urlencoded\r\n";
        }
        req+="\r\n"+body;
        // send
        size_t sent=0;
        while(sent<req.size()){
            ssize_t n=send(fd,req.c_str()+sent,req.size()-sent,0);
            if(n<=0){close(fd);throw std::runtime_error("Send failed");}
            sent+=n;
        }
        // receive
        std::string raw;
        char buf[4096];
        ssize_t n;
        while((n=recv(fd,buf,sizeof(buf),0))>0)raw.append(buf,n);
        close(fd);
        // parse status line
        size_t nl=raw.find("\r\n");
        if(nl==std::string::npos)throw std::runtime_error("Bad HTTP response");
        std::string statusLine=raw.substr(0,nl);
        int status=0;
        {size_t sp=statusLine.find(' ');if(sp!=std::string::npos)status=std::stoi(statusLine.substr(sp+1,3));}
        // parse headers
        std::string lower;lower.resize(raw.size());
        std::transform(raw.begin(),raw.end(),lower.begin(),::tolower);
        size_t headerEnd=raw.find("\r\n\r\n");
        if(headerEnd==std::string::npos)return {raw,status};
        std::string headerBlock=raw.substr(0,headerEnd);
        std::string rawBody=raw.substr(headerEnd+4);
        // redirect?
        if(status>=300&&status<400){
            size_t li=headerBlock.find("\r\nlocation:");
            if(li!=std::string::npos){
                size_t le=headerBlock.find("\r\n",li+2);
                std::string loc=headerBlock.substr(li+13,(le==std::string::npos?headerBlock.size():le)-(li+13));
                while(!loc.empty()&&(loc.back()==' '||loc.back()=='\r'))loc.pop_back();
                if(!loc.empty()&&loc[0]=='/')loc=u.scheme+"://"+u.host+(u.port==80?"":":"+portStr)+loc;
                return httpRequest("GET",loc,"",{},redirectsLeft-1);
            }
        }
        // chunked transfer decoding
        std::string lowerHeaders=lower.substr(0,headerEnd);
        if(lowerHeaders.find("transfer-encoding: chunked")!=std::string::npos){
            std::string decoded;
            size_t p=0;
            while(p<rawBody.size()){
                size_t crlf=rawBody.find("\r\n",p);
                if(crlf==std::string::npos)break;
                size_t chunkSize=std::stoul(rawBody.substr(p,crlf-p),nullptr,16);
                if(chunkSize==0)break;
                p=crlf+2;
                decoded+=rawBody.substr(p,chunkSize);
                p+=chunkSize+2;
            }
            return {decoded,status};
        }
        return {rawBody,status};
    }

    // ================================================================
    //  v3.1 — Subprocess via popen
    // ================================================================
    static std::pair<std::string,int> runCommand(const std::string& cmd){
        FILE* pipe=popen((cmd+" 2>&1").c_str(),"r");
        if(!pipe)throw std::runtime_error("Can't run command: "+cmd);
        std::string out;char buf[256];
        while(fgets(buf,sizeof(buf),pipe))out+=buf;
        int code=pclose(pipe);
        return {out,WIFEXITED(code)?WEXITSTATUS(code):1};
    }

    // ================================================================
    //  v3.1 — Eval: FetchExpr + RunExpr
    // ================================================================
    ValuePtr evalFetch(const FetchExpr& node,Env& env){
        auto url=evalExpr(*node.url,env)->toString();
        std::string method="GET",body;
        std::unordered_map<std::string,std::string> headers;
        if(node.opts){
            auto opts=evalExpr(*node.opts,env);
            if(auto*op=std::get_if<std::shared_ptr<IronObject>>(&opts->data)){
                auto get=[&](const std::string&k)->std::string{
                    auto it=(*op)->find(k);return it!=(*op)->end()?it->second->toString():"";
                };
                if(auto m=get("method");!m.empty()){method=m;for(auto&c:method)c=::toupper(c);}
                body=get("body");
                // headers sub-dict
                auto hit=(*op)->find("headers");
                if(hit!=(*op)->end()){
                    if(auto*hp=std::get_if<std::shared_ptr<IronObject>>(&hit->second->data))
                        for(auto&[k,v]:**hp)headers[k]=v->toString();
                }
            }
        }
        try{
            auto resp=httpRequest(method,url,body,headers);
            auto obj=std::make_shared<IronObject>();
            (*obj)["body"]  =IronValue::makeStr(resp.body);
            (*obj)["status"]=IronValue::makeNum(resp.status);
            (*obj)["ok"]    =IronValue::makeBool(resp.status>=200&&resp.status<300);
            return IronValue::makeObj(obj);
        }catch(std::exception&e){
            auto obj=std::make_shared<IronObject>();
            (*obj)["body"]  =IronValue::makeStr(e.what());
            (*obj)["status"]=IronValue::makeNum(0);
            (*obj)["ok"]    =IronValue::makeBool(false);
            return IronValue::makeObj(obj);
        }
    }
    ValuePtr evalRun(const RunExpr& node,Env& env){
        auto cmd=evalExpr(*node.cmd,env)->toString();
        auto[output,code]=runCommand(cmd);
        auto obj=std::make_shared<IronObject>();
        (*obj)["output"]=IronValue::makeStr(output);
        (*obj)["code"]  =IronValue::makeNum(code);
        (*obj)["ok"]    =IronValue::makeBool(code==0);
        return IronValue::makeObj(obj);
    }

    // ---- v3.1 JSON helpers ----
    std::string ironToJson(ValuePtr v){
        if(std::get_if<std::nullptr_t>(&v->data))return "null";
        if(auto*b=std::get_if<bool>(&v->data))return *b?"true":"false";
        if(auto*n=std::get_if<double>(&v->data)){
            if(*n==std::floor(*n)&&std::abs(*n)<1e15)return std::to_string((long long)*n);
            std::ostringstream o;o<<*n;return o.str();
        }
        if(auto*s=std::get_if<std::string>(&v->data)){
            std::string out="\"";
            for(char c:*s){if(c=='"')out+="\\\"";else if(c=='\\')out+="\\\\";else if(c=='\n')out+="\\n";else if(c=='\t')out+="\\t";else out+=c;}
            return out+"\"";
        }
        if(auto*ap=std::get_if<std::shared_ptr<IronArray>>(&v->data)){
            std::string out="[";
            for(size_t i=0;i<(*ap)->size();i++){if(i)out+=",";out+=ironToJson((**ap)[i]);}
            return out+"]";
        }
        if(auto*op=std::get_if<std::shared_ptr<IronObject>>(&v->data)){
            std::string out="{";bool first=true;
            for(auto&[k,val]:**op){
                if(k=="__class__")continue;
                if(!first)out+=",";out+="\""+k+"\":"+ironToJson(val);first=false;
            }
            return out+"}";
        }
        return "null";
    }
    static void skipJsonWs(const std::string& s,size_t& p){while(p<s.size()&&std::isspace(s[p]))p++;}
    ValuePtr jsonToIron(const std::string& s,size_t& p){
        skipJsonWs(s,p);
        if(p>=s.size())return IronValue::makeNull();
        char c=s[p];
        if(c=='"'){
            p++;std::string out;
            while(p<s.size()&&s[p]!='"'){
                if(s[p]=='\\'&&p+1<s.size()){p++;switch(s[p]){case'n':out+='\n';break;case't':out+='\t';break;default:out+=s[p];}}
                else out+=s[p];p++;
            }
            if(p<s.size())p++;
            return IronValue::makeStr(out);
        }
        if(c=='['){
            p++;auto arr=std::make_shared<IronArray>();skipJsonWs(s,p);
            while(p<s.size()&&s[p]!=']'){
                arr->push_back(jsonToIron(s,p));skipJsonWs(s,p);
                if(p<s.size()&&s[p]==',')p++;
            }
            if(p<s.size())p++;return IronValue::makeArr(arr);
        }
        if(c=='{'){
            p++;auto obj=std::make_shared<IronObject>();skipJsonWs(s,p);
            while(p<s.size()&&s[p]!='}'){
                size_t kp=p;auto key=jsonToIron(s,kp);p=kp;skipJsonWs(s,p);
                if(p<s.size()&&s[p]==':')p++;
                (*obj)[key->toString()]=jsonToIron(s,p);skipJsonWs(s,p);
                if(p<s.size()&&s[p]==',')p++;
            }
            if(p<s.size())p++;return IronValue::makeObj(obj);
        }
        if(s.substr(p,4)=="null"){p+=4;return IronValue::makeNull();}
        if(s.substr(p,4)=="true"){p+=4;return IronValue::makeBool(true);}
        if(s.substr(p,5)=="false"){p+=5;return IronValue::makeBool(false);}
        // number
        size_t start=p;if(s[p]=='-')p++;
        while(p<s.size()&&(std::isdigit(s[p])||s[p]=='.'||s[p]=='e'||s[p]=='E'||s[p]=='+'||s[p]=='-'))p++;
        try{return IronValue::makeNum(std::stod(s.substr(start,p-start)));}catch(...){return IronValue::makeNull();}
    }

    ValuePtr callValue(ValuePtr callee,std::vector<ValuePtr> args){
        if(auto*f=std::get_if<NativeFunc>(&callee->data))return (*f)(args);
        if(auto*f=std::get_if<IronFunc>(&callee->data)){
            Env fe;fe.parent=f->closure;
            for(size_t i=0;i<f->params.size();i++)fe.define(f->params[i],i<args.size()?args[i]:IronValue::makeNull());
            try{execBlock(*f->body,fe);}catch(ReturnSignal&r){return r.value;}
            return IronValue::makeNull();
        }
        throw std::runtime_error("That's not a function — can't call it.");
    }

    void assignLvalue(const Expr& target,ValuePtr val,Env& env){
        std::visit([&](auto& node){
            using T=std::decay_t<decltype(node)>;
            if constexpr(std::is_same_v<T,VarExpr>) env.assign(node.name,val);
            else if constexpr(std::is_same_v<T,IndexExpr>){
                auto obj=evalExpr(*node.obj,env);auto idx=evalExpr(*node.index,env);
                if(auto*ap=std::get_if<std::shared_ptr<IronArray>>(&obj->data))if(auto*n=std::get_if<double>(&idx->data))(**ap)[(int)*n]=val;
                if(auto*op=std::get_if<std::shared_ptr<IronObject>>(&obj->data))(**op)[idx->toString()]=val;
            }
            else if constexpr(std::is_same_v<T,MemberExpr>){
                auto obj=evalExpr(*node.obj,env);
                if(auto*op=std::get_if<std::shared_ptr<IronObject>>(&obj->data))(**op)[node.field]=val;
            }
        },target.node);
    }

    // ---- Execute statement ----
    void execStmt(const Stmt& stmt,Env& env){
        std::visit([&](auto& node){
            using T=std::decay_t<decltype(node)>;

            if constexpr(std::is_same_v<T,LetStmt>) env.define(node.name,evalExpr(*node.init,env));
            else if constexpr(std::is_same_v<T,SetStmt>){auto val=evalExpr(*node.value,env);assignLvalue(*node.target,val,env);}
            else if constexpr(std::is_same_v<T,AddToStmt>){
                auto val=evalExpr(*node.value,env);
                auto av=evalExpr(*node.target,env);
                if(auto*ap=std::get_if<std::shared_ptr<IronArray>>(&av->data))(*ap)->push_back(val);
                else throw std::runtime_error("Can't add to that — it's not a list.");
            }
            else if constexpr(std::is_same_v<T,SayStmt>) std::cout<<evalExpr(*node.expr,env)->toString()<<"\n";
            else if constexpr(std::is_same_v<T,AskStmt>){
                std::string prompt=evalExpr(*node.prompt,env)->toString();
                if(!prompt.empty())std::cout<<prompt<<" ";
                std::string input;std::getline(std::cin,input);
                try{env.assign(node.varName,IronValue::makeStr(input));}catch(...){env.define(node.varName,IronValue::makeStr(input));}
            }
            else if constexpr(std::is_same_v<T,PauseStmt>){
                std::cout<<"[Press Enter to continue...]";
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
            }
            else if constexpr(std::is_same_v<T,IfStmt>){
                Env ie;ie.parent=&env;
                if(evalExpr(*node.cond,env)->isTruthy())execBlock(node.thenBody,ie);
                else execBlock(node.elseBody,ie);
            }
            else if constexpr(std::is_same_v<T,WhileStmt>){
                while(evalExpr(*node.cond,env)->isTruthy()){
                    Env le;le.parent=&env;
                    try{execBlock(node.body,le);}catch(BreakSignal&){break;}catch(ContinueSignal&){continue;}
                }
            }
            else if constexpr(std::is_same_v<T,ForStmt>){
                auto iter=evalExpr(*node.iterable,env);
                std::vector<ValuePtr> items;
                if(auto*ap=std::get_if<std::shared_ptr<IronArray>>(&iter->data))items=**ap;
                else if(auto*sp=std::get_if<std::string>(&iter->data))for(char c:*sp)items.push_back(IronValue::makeStr(std::string(1,c)));
                for(auto&item:items){
                    Env le;le.parent=&env;le.define(node.var,item);
                    try{execBlock(node.body,le);}catch(BreakSignal&){break;}catch(ContinueSignal&){continue;}
                }
            }
            else if constexpr(std::is_same_v<T,BreakStmt>)    throw BreakSignal{};
            else if constexpr(std::is_same_v<T,ContinueStmt>) throw ContinueSignal{};
            else if constexpr(std::is_same_v<T,ReturnStmt>)   throw ReturnSignal{evalExpr(*node.value,env)};
            else if constexpr(std::is_same_v<T,FuncStmt>){
                IronFunc f{node.params,&node.body,&env};
                env.define(node.name,IronValue::makeFunc(f));
            }
            else if constexpr(std::is_same_v<T,CallStmt>) evalExpr(*node.call,env);
            else if constexpr(std::is_same_v<T,GetStmt>){env.define(node.alias,loadModule(node.path));}
            else if constexpr(std::is_same_v<T,ExprStmt>) evalExpr(*node.expr,env);

            // ---- v2.0: class definition ----
            else if constexpr(std::is_same_v<T,ClassStmt>){
                ClassDef cd;
                cd.name=node.name;
                cd.definitionEnv=&env;
                for(auto&s:node.body){
                    if(auto*let=std::get_if<LetStmt>(&s->node))
                        cd.fields.push_back({let->name,let->init.get()});
                    else if(auto*fn=std::get_if<FuncStmt>(&s->node)){
                        IronFunc f{fn->params,&fn->body,&env};
                        cd.methods[fn->name]=f;
                    }
                }
                classRegistry[node.name]=std::move(cd);
            }

            // ---- v2.0: try / catch ----
            else if constexpr(std::is_same_v<T,TryStmt>){
                Env tryEnv;tryEnv.parent=&env;
                try{
                    execBlock(node.body,tryEnv);
                } catch(ReturnSignal&){throw;   // always propagate control flow
                } catch(BreakSignal&){throw;
                } catch(ContinueSignal&){throw;
                } catch(ThrowSignal& ts){
                    Env ce;ce.parent=&env;
                    ce.define(node.catchVar,IronValue::makeStr(ts.message));
                    execBlock(node.catchBody,ce);
                } catch(std::exception& e){
                    Env ce;ce.parent=&env;
                    ce.define(node.catchVar,IronValue::makeStr(e.what()));
                    execBlock(node.catchBody,ce);
                }
            }

            // ---- v2.0: throw ----
            else if constexpr(std::is_same_v<T,ThrowStmt>){
                auto val=evalExpr(*node.value,env);
                throw ThrowSignal{val->toString()};
            }

            // ---- v2.0 Scratch-style: write <content> to file <path> ----
            else if constexpr(std::is_same_v<T,WriteFileStmt>){
                auto p=evalExpr(*node.path,env)->toString();
                auto c=evalExpr(*node.content,env)->toString();
                std::ofstream f(p);
                if(!f)throw ThrowSignal{"Can't write to file: "+p};
                f<<c;
            }
            // ---- v2.0 Scratch-style: append <content> to file <path> ----
            else if constexpr(std::is_same_v<T,AppendFileStmt>){
                auto p=evalExpr(*node.path,env)->toString();
                auto c=evalExpr(*node.content,env)->toString();
                std::ofstream f(p,std::ios::app);
                if(!f)throw ThrowSignal{"Can't append to file: "+p};
                f<<c;
            }

        },stmt.node);
    }

    void execBlock(const StmtList& stmts,Env& env){for(auto&s:stmts)execStmt(*s,env);}

    // ---- Standard Library + User Modules ----
    ValuePtr loadModule(const std::string& name){
        // v3.0: load a .irw file as a module
        if(name.size()>4 && name.substr(name.size()-4)==".irw"){
            std::ifstream f(name);
            if(!f)throw std::runtime_error("Can't open module: "+name);
            std::string src((std::istreambuf_iterator<char>(f)),{});
            Lexer lex(src);auto toks=lex.tokenize();
            Parser par(std::move(toks));
            moduleAsts.push_back(par.parse()); // persist AST — IronFunc.body ptrs depend on it
            StmtList& prog=moduleAsts.back();
            moduleEnvs.emplace_back();
            Env& modEnv=moduleEnvs.back();
            modEnv.parent=&globalEnv;
            execBlock(prog,modEnv);
            auto obj=std::make_shared<IronObject>();
            for(auto&[k,v]:modEnv.vars)(*obj)[k]=v;
            return IronValue::makeObj(obj);
        }
        auto obj=std::make_shared<IronObject>();
        if(name=="stdlib"||name=="std"){
            // math
            auto math=std::make_shared<IronObject>();
            (*math)["abs"]   =IronValue::makeNative([](std::vector<ValuePtr>a){return IronValue::makeNum(std::abs(std::get<double>(a[0]->data)));});
            (*math)["floor"] =IronValue::makeNative([](std::vector<ValuePtr>a){return IronValue::makeNum(std::floor(std::get<double>(a[0]->data)));});
            (*math)["ceil"]  =IronValue::makeNative([](std::vector<ValuePtr>a){return IronValue::makeNum(std::ceil(std::get<double>(a[0]->data)));});
            (*math)["sqrt"]  =IronValue::makeNative([](std::vector<ValuePtr>a){return IronValue::makeNum(std::sqrt(std::get<double>(a[0]->data)));});
            (*math)["random"]=IronValue::makeNative([](std::vector<ValuePtr>){return IronValue::makeNum((double)rand()/RAND_MAX);});
            (*math)["pow"]   =IronValue::makeNative([](std::vector<ValuePtr>a){return IronValue::makeNum(std::pow(std::get<double>(a[0]->data),std::get<double>(a[1]->data)));});
            (*obj)["math"]=IronValue::makeObj(math);
            // io
            auto io=std::make_shared<IronObject>();
            (*io)["alert"]  =IronValue::makeNative([](std::vector<ValuePtr>a){std::cout<<"[ALERT] "<<(a.empty()?"":a[0]->toString())<<"\n";return IronValue::makeNull();});
            (*io)["prompt"] =IronValue::makeNative([](std::vector<ValuePtr>a){if(!a.empty())std::cout<<a[0]->toString()<<" ";std::string s;std::getline(std::cin,s);return IronValue::makeStr(s);});
            (*io)["confirm"]=IronValue::makeNative([](std::vector<ValuePtr>a){if(!a.empty())std::cout<<a[0]->toString()<<" (y/n) ";std::string s;std::getline(std::cin,s);return IronValue::makeBool(s=="y"||s=="Y"||s=="yes");});
            (*obj)["io"]=IronValue::makeObj(io);
            (*obj)["add"]=IronValue::makeNative([](std::vector<ValuePtr>a){return IronValue::makeNum(std::get<double>(a[0]->data)+std::get<double>(a[1]->data));});
        }
        return IronValue::makeObj(obj);
    }

    void registerGlobals(const std::vector<std::string>& userArgs){
        globalEnv.define("parseInt",  IronValue::makeNative([](std::vector<ValuePtr>a)->ValuePtr{if(a.empty())return IronValue::makeNull();try{return IronValue::makeNum((double)(long long)std::stod(a[0]->toString()));}catch(...){return IronValue::makeNull();}}));
        globalEnv.define("parseFloat",IronValue::makeNative([](std::vector<ValuePtr>a)->ValuePtr{if(a.empty())return IronValue::makeNull();try{return IronValue::makeNum(std::stod(a[0]->toString()));}catch(...){return IronValue::makeNull();}}));
        globalEnv.define("toString",  IronValue::makeNative([](std::vector<ValuePtr>a)->ValuePtr{if(a.empty())return IronValue::makeStr("");return IronValue::makeStr(a[0]->toString());}));
        globalEnv.define("len",       IronValue::makeNative([](std::vector<ValuePtr>a)->ValuePtr{if(a.empty())return IronValue::makeNum(0);if(auto*s=std::get_if<std::string>(&a[0]->data))return IronValue::makeNum(s->size());if(auto*ar=std::get_if<std::shared_ptr<IronArray>>(&a[0]->data))return IronValue::makeNum((*ar)->size());return IronValue::makeNum(0);}));
        // v3.0: args list from command line
        auto argsArr=std::make_shared<IronArray>();
        for(auto&a:userArgs)argsArr->push_back(IronValue::makeStr(a));
        globalEnv.define("args",IronValue::makeArr(argsArr));
        // math globally
        auto math=std::make_shared<IronObject>();
        (*math)["abs"]   =IronValue::makeNative([](std::vector<ValuePtr>a){return IronValue::makeNum(std::abs(std::get<double>(a[0]->data)));});
        (*math)["floor"] =IronValue::makeNative([](std::vector<ValuePtr>a){return IronValue::makeNum(std::floor(std::get<double>(a[0]->data)));});
        (*math)["ceil"]  =IronValue::makeNative([](std::vector<ValuePtr>a){return IronValue::makeNum(std::ceil(std::get<double>(a[0]->data)));});
        (*math)["sqrt"]  =IronValue::makeNative([](std::vector<ValuePtr>a){return IronValue::makeNum(std::sqrt(std::get<double>(a[0]->data)));});
        (*math)["random"]=IronValue::makeNative([](std::vector<ValuePtr>){return IronValue::makeNum((double)rand()/RAND_MAX);});
        (*math)["pow"]   =IronValue::makeNative([](std::vector<ValuePtr>a){return IronValue::makeNum(std::pow(std::get<double>(a[0]->data),std::get<double>(a[1]->data)));});
        globalEnv.define("math",IronValue::makeObj(math));
    }
public:
    Interpreter(const std::vector<std::string>& userArgs={}){registerGlobals(userArgs);}
    void run(const StmtList& program){execBlock(program,globalEnv);}
};

// ============================================================
//  MAIN
// ============================================================

int main(int argc,char** argv){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2),&wsaData);
#endif
    srand((unsigned)time(nullptr));
    if(argc<2){std::cerr<<"Usage: ironwood <file.irw> [args...]\n";return 1;}
    std::ifstream file(argv[1]);
    if(!file){std::cerr<<"Can't open file: "<<argv[1]<<"\n";return 1;}
    std::string source((std::istreambuf_iterator<char>(file)),{});
    std::vector<std::string> userArgs;
    for(int i=2;i<argc;i++)userArgs.push_back(argv[i]);
    try{
        Lexer lexer(source);auto tokens=lexer.tokenize();
        Parser parser(std::move(tokens));auto program=parser.parse();
        Interpreter interp(userArgs);interp.run(program);
    }catch(const std::exception&e){
        std::cerr<<"\n--- Ironwood Error ---\n"<<e.what()<<"\n";
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
