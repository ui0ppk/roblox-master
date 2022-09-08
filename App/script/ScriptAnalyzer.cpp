#include "stdafx.h"
#include "script/ScriptAnalyzer.h"

#include "Lua/lua.hpp"
#include "util/ProtectedString.h"

#include "lstate.h"
#include "lopcodes.h"
#include "lvm.h"
#include "ldebug.h"

#include "script/ScriptContext.h"
#include "script/Script.h"
#include "script/LuaInstanceBridge.h"
#include "script/LuaArguments.h"

#include "rbx/DenseHash.h"

FASTFLAGVARIABLE(StudioVariableIntellesense, false)
FASTFLAGVARIABLE(DebugScriptAnalyzer, false)

FASTINTVARIABLE(ScriptAnalyzerIgnoreWarnings, 0)

namespace RBX
{
	using namespace ScriptAnalyzer;

    namespace ScriptParser
    {
        class Allocator
        {
        public:
            Allocator()
                : root(static_cast<Page*>(operator new(sizeof(Page))))
                , offset(0)
            {
                root->next = NULL;
            }

            ~Allocator()
            {
                Page* page = root;

                while (page)
                {
                    Page* next = page->next;

                    operator delete(page);

                    page = next;
                }
            }

            void* allocate(size_t size)
            {
                // pointer-align all allocations
                size = (size + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
                
                if (offset + size <= sizeof(root->data))
                {
                    void* result = root->data + offset;
                    offset += size;
                    return result;
                }

                // allocate new page
                void* pageData = operator new(offsetof(Page, data) + std::max(sizeof(root->data), size));

                Page* page = static_cast<Page*>(pageData);

                page->next = root;

                root = page;
                offset = size;

                return page->data;
            }

        private:
            struct Page
            {
                Page* next;

                char data[8192];
            };
            
            Page* root;
            unsigned int offset;
        };
    }
}

void* operator new(size_t size, RBX::ScriptParser::Allocator& alloc)
{
    return alloc.allocate(size);
}

void* operator new[](size_t size, RBX::ScriptParser::Allocator& alloc)
{
    return alloc.allocate(size);
}

void operator delete(void*, RBX::ScriptParser::Allocator& alloc)
{
}

void operator delete[](void*, RBX::ScriptParser::Allocator& alloc)
{
}

namespace RBX
{
    namespace ScriptParser
    {
        class Error: public std::exception
        {
        public:
            Error(const Location& location, const char* format, ...) RBX_PRINTF_ATTR(3, 4)
                : location(location)
            {
                va_list args;
                va_start(args, format);
                message = vformat(format, args);
                va_end(args);
            }
            
            virtual ~Error() throw()
            {
            }
            
            virtual const char* what() const throw()
            {
                return message.c_str();
            }
            
            const Location& getLocation() const
            {
                return location;
            }
            
        private:
            std::string message;
            Location location;
        };
        
        const char* kReserved[] =
        {
            "and", "break", "do", "else", "elseif",
            "end", "false", "for", "function", "if",
            "in", "local", "nil", "not", "or", "repeat",
            "return", "then", "true", "until", "while"
        };
        
        struct Lexeme
        {
            enum Type
            {
                Eof = 0,
                
                // 1..255 means actual character values
                Char_END = 256,
                
                Equal,
                LessEqual,
                GreaterEqual,
                NotEqual,
                Dot2,
                Dot3,
                String,
                Number,
                Name,
                
                Reserved_BEGIN,
                ReservedAnd = Reserved_BEGIN,
                ReservedBreak,
                ReservedDo,
                ReservedElse,
                ReservedElseif,
                ReservedEnd,
                ReservedFalse,
                ReservedFor,
                ReservedFunction,
                ReservedIf,
                ReservedIn,
                ReservedLocal,
                ReservedNil,
                ReservedNot,
                ReservedOr,
                ReservedRepeat,
                ReservedReturn,
                ReservedThen,
                ReservedTrue,
                ReservedUntil,
                ReservedWhile,
                Reserved_END
            };

            Type type;
            Location location;

            union
            {
                const std::string* data; // String, Number
                const char* name; // Name
            };

            Lexeme(const Location& location, Type type)
                : type(type)
                , location(location)
            {
            }

            Lexeme(const Location& location, char character)
                : type(static_cast<Type>(static_cast<unsigned char>(character)))
                , location(location)
            {
            }

            Lexeme(const Location& location, Type type, const std::string* data)
                : type(type)
                , location(location)
                , data(data)
            {
                RBXASSERT(type == String || type == Number);
            }

            Lexeme(const Location& location, Type type, const char* name)
                : type(type)
                , location(location)
                , name(name)
            {
                RBXASSERT(type == Name);
            }
            
            std::string toString() const
            {
                switch (type)
                {
                case Eof:
                    return "<eof>";
                        
                case Equal:
                    return "'=='";
                        
                case LessEqual:
                    return "'<='";
                        
                case GreaterEqual:
                    return "'>='";
                        
                case NotEqual:
                    return "'~='";
                        
                case Dot2:
                    return "'..'";
                        
                case Dot3:
                    return "'...'";
                        
                case String:
                    return format("\"%s\"", data->c_str());
                        
                case Number:
                    return format("'%s'", data->c_str());
                        
                case Name:
                    return format("'%s'", name);
                        
                default:
                    if (type < Char_END)
                        return format("'%c'", type);
                    else if (type >= Reserved_BEGIN && type < Reserved_END)
                        return format("'%s'", kReserved[type - Reserved_BEGIN]);
                    else
                        return "<unknown>";
                }
            }
        };
        
        struct AstName
        {
            const char* value;
            
            AstName()
                : value(NULL)
            {
            }
            
            explicit AstName(const char* value)
                : value(value)
            {
            }
            
            bool operator==(const AstName& rhs) const
            {
                return value == rhs.value;
            }
            
            bool operator!=(const AstName& rhs) const
            {
                return value != rhs.value;
            }
            
            bool operator==(const char* rhs) const
            {
                return strcmp(value, rhs) == 0;
            }
            
            bool operator!=(const char* rhs) const
            {
                return strcmp(value, rhs) != 0;
            }
        };
        
        size_t hash_value(const AstName& value)
        {
            return boost::hash_value(value.value);
        }
        
        class AstNameTable
        {
        public:
            AstNameTable(Allocator& allocator)
                : data("")
                , allocator(allocator)
            {
            }
            
            AstName addStatic(const char* name, Lexeme::Type type = Lexeme::Name)
            {
                AstNameTable::Entry entry = { AstName(name), type };
                
                RBXASSERT(!data.contains(name));
                data[name] = entry;

                return entry.value;
            }
            
            std::pair<AstName, Lexeme::Type> getOrAddWithType(const char* name)
            {
                const Entry* entry = data.find(name);
                
                if (entry)
                {
                    return std::make_pair(entry->value, entry->type);
                }
                else
                {
                    size_t nameLength = strlen(name);
                    
                    char* nameData = new (allocator) char[nameLength + 1];
                    memcpy(nameData, name, nameLength + 1);
                    
                    Entry newEntry = { AstName(nameData), Lexeme::Name };
                    data[nameData] = newEntry;
                    
                    return std::make_pair(newEntry.value, newEntry.type);
                }
            }
            
            AstName getOrAdd(const char* name)
            {
                return getOrAddWithType(name).first;
            }
            
        private:
            struct Entry
            {
                AstName value;
                Lexeme::Type type;
            };
            
            DenseHashMap<const char*, Entry, Reflection::StringHashPredicate, Reflection::StringEqualPredicate> data;
            
            Allocator& allocator;
        };
        
        class Lexer
        {
        public:
            Lexer(const char* buffer, size_t bufferSize, AstNameTable& names, Allocator& allocator)
                : buffer(buffer)
                , bufferSize(bufferSize)
                , offset(0)
                , line(0)
                , lineOffset(0)
                , lexeme(Location(Position(0, 0), 0), Lexeme::Eof)
                , names(names)
                , allocator(allocator)
            {
                BOOST_STATIC_ASSERT(sizeof(kReserved) / sizeof(kReserved[0]) == Lexeme::Reserved_END - Lexeme::Reserved_BEGIN);
                
                for (int i = Lexeme::Reserved_BEGIN; i < Lexeme::Reserved_END; ++i)
                    names.addStatic(kReserved[i - Lexeme::Reserved_BEGIN], static_cast<Lexeme::Type>(i));

                // read first lexeme
                next();
            }

            const Lexeme& next()
            {
				// consume whitespace or comments before the token
				while (isSpace(peekch()) || (peekch(0) == '-' && peekch(1) == '-'))
				{
					if (peekch(0) == '-')
					{
						consume();
						consume();

						skipCommentBody();
					}
					else
					{
						while (isSpace(peekch()))
							consume();
					}
				}

                lexeme = readNext();
                
                return lexeme;
            }
            
            const Lexeme& current() const
            {
                return lexeme;
            }
            
        private:
            static bool isNewline(char ch)
            {
                return ch == '\n';
            }

            static bool isSpace(char ch)
            {
                return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
            }
            
            static bool isAlpha(char ch)
            {
                return static_cast<unsigned int>(ch - 'A') < 26 || static_cast<unsigned int>(ch - 'a') < 26;
            }

            static bool isDigit(char ch)
            {
                return static_cast<unsigned int>(ch - '0') < 10;
            }

            static char unescape(char ch)
            {
                switch (ch)
                {
                case 'a': return '\a';
                case 'b': return '\b';
                case 'f': return '\f';
                case 'n': return '\n';
                case 'r': return '\r';
                case 't': return '\t';
                case 'v': return '\v';
                default: return ch;
                }
            }

            char peekch() const
            {
                return (offset < bufferSize) ? buffer[offset] : 0;
            }

            char peekch(unsigned int lookahead) const
            {
                return (offset + lookahead < bufferSize) ? buffer[offset + lookahead] : 0;
            }

            Position position() const
            {
                return Position(line, offset - lineOffset);
            }

            void consume()
            {
                if (isNewline(buffer[offset]))
                {
                    line++;
                    lineOffset = offset + 1;
                }

                offset++;
            }

            void skipCommentBody()
            {
                if (peekch() == '[')
                {
                    int sep = skipLongSeparator();

                    if (sep >= 0)
                    {
                        Position start = position();
                        
                        if (!readLongString(scratchData, sep))
                            throw Error(Location(start, position()), "Unfinished long comment");
                        
                        return;
                    }
                }

                // fall back to single-line comment
                while (peekch() != 0 && !isNewline(peekch()))
                    consume();
            }

            // Given a sequence [===[ or ]===], returns:
            // 1. number of equal signs (or 0 if none present) between the brackets
            // 2. -1 if this is not a long comment/string separator
            // 3. -N if this is a malformed separator
            // Does *not* consume the closing brace.
            int skipLongSeparator()
            {
                char start = peekch();

                RBXASSERT(start == '[' || start == ']');
                consume();

                int count = 0;

                while (peekch() == '=')
                {
                    consume();
                    count++;
                }

                return (start == peekch()) ? count : (-count) - 1;
            }

            bool readLongString(std::string& data, int sep)
            {
                data.clear();

                // skip (second) [
                RBXASSERT(peekch() == '[');
                consume();

                // skip first newline
                if (isNewline(peekch()))
                    consume();

                unsigned int startOffset = offset;

                while (peekch())
                {
                    if (peekch() == ']')
                    {
                        if (skipLongSeparator() == sep)
                        {
                            RBXASSERT(peekch() == ']');
                            consume(); // skip (second) ]

                            data.assign(buffer + startOffset, buffer + offset - sep - 2);
                            return true;
                        }
                    }
                    else
                    {
                        consume();
                    }
                }

                return false;
            }

            char readEscapedChar(const Position& start)
            {
                switch (peekch())
                {
                    case '\n':
                        consume();
                        return '\n';

                    case '\r':
                        consume();
                        if (peekch() == '\n')
                            consume();
                        
                        return '\n';

                    case 0:
                        throw Error(Location(start, position()), "Unfinished string");
                        
                    default:
                    {
                        if (isDigit(peekch()))
                        {
                            int code = 0;
                            int i = 0;
                            
                            do
                            {
                                code = 10*code + (peekch() - '0');
                                consume();
                            }
                            while (++i < 3 && isDigit(peekch()));
                            
                            if (code > UCHAR_MAX)
                                throw Error(Location(start, position()), "Escape sequence too large");
                            
                            return static_cast<char>(code);
                        }
                        else
                        {
                            char result = unescape(peekch());
                            consume();

                            return result;
                        }
                    }
                }
            }

            void readString(std::string& data)
            {
                Position start = position();
                
                char delimiter = peekch();
                RBXASSERT(delimiter == '\'' || delimiter == '"');
                consume();
                
                data.clear();
                
                while (peekch() != delimiter)
                {
                    switch (peekch())
                    {
                    case 0:
                    case '\r':
                    case '\n':
                        throw Error(Location(start, position()), "Unfinished string");
                            
                    case '\\':
                        consume();
                        data += readEscapedChar(start);
                        break;

                    default:
                        data += peekch();
                        consume();
                    }
                }
                
                consume();
            }
            
            void readNumber(std::string& data, unsigned int startOffset)
            {
                RBXASSERT(isDigit(peekch()));

                // This function does not do the number parsing - it only skips a number-like pattern.
                // It uses the same logic as Lua stock lexer; the resulting string is later converted
                // to a number with proper verification.
                do
                {
                    consume();
                }
                while (isDigit(peekch()) || peekch() == '.');
                
                if (peekch() == 'e' || peekch() == 'E')
                {
                    consume();
                    
                    if (peekch() == '+' || peekch() == '-')
                        consume();
                }
                
                while (isAlpha(peekch()) || isDigit(peekch()) || peekch() == '_')
                    consume();
                
                data.assign(buffer + startOffset, buffer + offset);
            }
            
            Lexeme readNext()
            {
                Position start = position();

                switch (peekch())
                {
                case 0:
                    return Lexeme(Location(start, 0), Lexeme::Eof);

                case '-':
					consume();

					return Lexeme(Location(start, 1), '-');

                case '[':
                    {
                        int sep = skipLongSeparator();

                        if (sep >= 0)
                        {
                            if (!readLongString(scratchData, sep))
                                throw Error(Location(start, position()), "Unfinished long string");

                            return Lexeme(Location(start, position()), Lexeme::String, &scratchData);
                        }
                        else if (sep == -1)
                            return Lexeme(Location(start, 1), '[');
                        else
                            throw Error(Location(start, position()), "Invalid long string delimiter");
                    }

                case '=':
                    {
                        consume();

                        if (peekch() == '=')
                        {
                            consume();
                            return Lexeme(Location(start, 2), Lexeme::Equal);
                        }
                        else
                            return Lexeme(Location(start, 1), '=');
                    }

                case '<':
                    {
                        consume();

                        if (peekch() == '=')
                        {
                            consume();
                            return Lexeme(Location(start, 2), Lexeme::LessEqual);
                        }
                        else
                            return Lexeme(Location(start, 1), '<');
                    }

                case '>':
                    {
                        consume();

                        if (peekch() == '=')
                        {
                            consume();
                            return Lexeme(Location(start, 2), Lexeme::GreaterEqual);
                        }
                        else
                            return Lexeme(Location(start, 1), '>');
                    }

                case '~':
                    {
                        consume();

                        if (peekch() == '=')
                        {
                            consume();
                            return Lexeme(Location(start, 2), Lexeme::NotEqual);
                        }
                        else
                            return Lexeme(Location(start, 1), '~');
                    }

                case '"':
                case '\'':
                    readString(scratchData);

                    return Lexeme(Location(start, position()), Lexeme::String, &scratchData);

                case '.':
                    consume();

                    if (peekch() == '.')
                    {
                        consume();

                        if (peekch() == '.')
                        {
                            consume();

                            return Lexeme(Location(start, 3), Lexeme::Dot3);
                        }
                        else
                            return Lexeme(Location(start, 2), Lexeme::Dot2);
                    }
                    else
                    {
                        if (isDigit(peekch()))
                        {
                            readNumber(scratchData, offset - 1);

                            return Lexeme(Location(start, position()), Lexeme::Number, &scratchData);
                        }
                        else
                            return Lexeme(Location(start, 1), '.');
                    }

                default:
                    if (isDigit(peekch()))
                    {
                        readNumber(scratchData, offset);

                        return Lexeme(Location(start, position()), Lexeme::Number, &scratchData);
                    }
                    else if (isAlpha(peekch()) || peekch() == '_')
                    {
                        unsigned int startOffset = offset;

                        do consume();
                        while (isAlpha(peekch()) || isDigit(peekch()) || peekch() == '_');

                        scratchData.assign(buffer + startOffset, buffer + offset);
                        
                        std::pair<AstName, Lexeme::Type> name = names.getOrAddWithType(scratchData.c_str());
                        
                        if (name.second == Lexeme::Name)
                            return Lexeme(Location(start, position()), Lexeme::Name, name.first.value);
                        else
                            return Lexeme(Location(start, position()), name.second);
                    }
                    else
                    {
                        char ch = peekch();
                        consume();

                        return Lexeme(Location(start, 1), ch);
                    }
                }
            }

            const char* buffer;
            size_t bufferSize;

            unsigned int offset;

            unsigned int line;
            unsigned int lineOffset;

            std::string scratchData;
            
            Lexeme lexeme;
            
            AstNameTable& names;
            
            Allocator& allocator;
        };
        
        int gAstRttiIndex = 0;
        
        template <typename T> struct AstRtti
        {
            BOOST_STATIC_ASSERT(boost::is_class<T>::value);
            
            static const int value;
        };
        
        template <typename T> const int AstRtti<T>::value = ++gAstRttiIndex;
        
        #define ASTRTTI(Class) virtual int getClassIndex() const { return AstRtti<Class>::value; }
        
        template <typename T> struct AstArray
        {
            T* data;
            size_t size;
        };
        
        struct AstLocal
        {
            AstName name;
            Location location;
            AstLocal* shadow;
            unsigned int functionDepth;
            
            AstLocal(const AstName& name, const Location& location, AstLocal* shadow, unsigned int functionDepth)
                : name(name)
                , location(location)
                , shadow(shadow)
                , functionDepth(functionDepth)
            {
            }
        };

        class AstVisitor
        {
        public:
            virtual ~AstVisitor() {}
            
            virtual bool visit(class AstExpr* node) { return true; }
            
            virtual bool visit(class AstExprGroup* node) { return visit((class AstExpr*)node); }
            virtual bool visit(class AstExprConstantNil* node) { return visit((class AstExpr*)node); }
            virtual bool visit(class AstExprConstantBool* node) { return visit((class AstExpr*)node); }
            virtual bool visit(class AstExprConstantNumber* node) { return visit((class AstExpr*)node); }
            virtual bool visit(class AstExprConstantString* node) { return visit((class AstExpr*)node); }
            virtual bool visit(class AstExprLocal* node) { return visit((class AstExpr*)node); }
            virtual bool visit(class AstExprGlobal* node) { return visit((class AstExpr*)node); }
            virtual bool visit(class AstExprVarargs* node) { return visit((class AstExpr*)node); }
            virtual bool visit(class AstExprCall* node) { return visit((class AstExpr*)node); }
            virtual bool visit(class AstExprIndexName* node) { return visit((class AstExpr*)node); }
            virtual bool visit(class AstExprIndexExpr* node) { return visit((class AstExpr*)node); }
            virtual bool visit(class AstExprFunction* node) { return visit((class AstExpr*)node); }
            virtual bool visit(class AstExprTable* node) { return visit((class AstExpr*)node); }
            virtual bool visit(class AstExprUnary* node) { return visit((class AstExpr*)node); }
            virtual bool visit(class AstExprBinary* node) { return visit((class AstExpr*)node); }
            
            virtual bool visit(class AstStat* node) { return true; }
            
            virtual bool visit(class AstStatBlock* node) { return visit((class AstStat*)node); }
            virtual bool visit(class AstStatIf* node) { return visit((class AstStat*)node); }
            virtual bool visit(class AstStatWhile* node) { return visit((class AstStat*)node); }
            virtual bool visit(class AstStatRepeat* node) { return visit((class AstStat*)node); }
            virtual bool visit(class AstStatBreak* node) { return visit((class AstStat*)node); }
            virtual bool visit(class AstStatReturn* node) { return visit((class AstStat*)node); }
            virtual bool visit(class AstStatExpr* node) { return visit((class AstStat*)node); }
            virtual bool visit(class AstStatLocal* node) { return visit((class AstStat*)node); }
            virtual bool visit(class AstStatFor* node) { return visit((class AstStat*)node); }
            virtual bool visit(class AstStatForIn* node) { return visit((class AstStat*)node); }
            virtual bool visit(class AstStatAssign* node) { return visit((class AstStat*)node); }
        };
        
        class AstNode
        {
        public:
            explicit AstNode(const Location& location): location(location) {}
            virtual ~AstNode() {}
            
            virtual void visit(AstVisitor* visitor) = 0;
            
            virtual int getClassIndex() const = 0;
            
            template <typename T> bool is() const { return getClassIndex() == AstRtti<T>::value; }
            template <typename T> T* as() { return getClassIndex() == AstRtti<T>::value ? static_cast<T*>(this) : NULL; }
            
            Location location;
        };

        class AstExpr: public AstNode
        {
        public:
            explicit AstExpr(const Location& location): AstNode(location) {}
        };

        class AstStat: public AstNode
        {
        public:
            explicit AstStat(const Location& location): AstNode(location) {}
        };
        
        class AstExprGroup: public AstExpr
        {
        public:
            ASTRTTI(AstExprGroup)
            
            explicit AstExprGroup(const Location& location, AstExpr* expr)
                : AstExpr(location)
                , expr(expr)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                if (visitor->visit(this))
                    expr->visit(visitor);
            }
            
            AstExpr* expr;
        };
        
        class AstExprConstantNil: public AstExpr
        {
        public:
            ASTRTTI(AstExprConstantNil)
            
            explicit AstExprConstantNil(const Location& location)
                : AstExpr(location)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                visitor->visit(this);
            }
        };
        
        class AstExprConstantBool: public AstExpr
        {
        public:
            ASTRTTI(AstExprConstantBool)
            
            AstExprConstantBool(const Location& location, bool value)
                : AstExpr(location)
                , value(value)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                visitor->visit(this);
            }
            
            bool value;
        };
        
        class AstExprConstantNumber: public AstExpr
        {
        public:
            ASTRTTI(AstExprConstantNumber)
            
            AstExprConstantNumber(const Location& location, double value)
                : AstExpr(location)
                , value(value)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                visitor->visit(this);
            }
            
            double value;
        };
        
        class AstExprConstantString: public AstExpr
        {
        public:
            ASTRTTI(AstExprConstantString)
            
            AstExprConstantString(const Location& location, const AstArray<char>& value)
                : AstExpr(location)
                , value(value)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                visitor->visit(this);
            }
            
            AstArray<char> value;
        };
        
        class AstExprLocal: public AstExpr
        {
        public:
            ASTRTTI(AstExprLocal)
            
            AstExprLocal(const Location& location, AstLocal* local, bool upvalue)
                : AstExpr(location)
                , local(local)
                , upvalue(upvalue)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                visitor->visit(this);
            }
            
            AstLocal* local;
            bool upvalue;
        };
        
        class AstExprGlobal: public AstExpr
        {
        public:
            ASTRTTI(AstExprGlobal)
            
            AstExprGlobal(const Location& location, const AstName& name)
                : AstExpr(location)
                , name(name)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                visitor->visit(this);
            }
            
            AstName name;
        };
        
        class AstExprVarargs: public AstExpr
        {
        public:
            ASTRTTI(AstExprVarargs)
            
            AstExprVarargs(const Location& location)
                : AstExpr(location)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                visitor->visit(this);
            }
        };
        
        class AstExprCall: public AstExpr
        {
        public:
            ASTRTTI(AstExprCall)
            
            AstExprCall(const Location& location, AstExpr* func, const AstArray<AstExpr*>& args, bool self)
                : AstExpr(location)
                , func(func)
                , args(args)
                , self(self)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                if (visitor->visit(this))
                {
                    func->visit(visitor);
                    
                    for (size_t i = 0; i < args.size; ++i)
                        args.data[i]->visit(visitor);
                }
            }
            
            AstExpr* func;
            AstArray<AstExpr*> args;
            bool self;
        };
        
        class AstExprIndexName: public AstExpr
        {
        public:
            ASTRTTI(AstExprIndexName)
 
            AstExprIndexName(const Location& location, AstExpr* expr, const AstName& index, const Location& indexLocation)
                : AstExpr(location)
                , expr(expr)
                , index(index)
                , indexLocation(indexLocation)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                if (visitor->visit(this))
                    expr->visit(visitor);
            }
            
            AstExpr* expr;
            AstName index;
            Location indexLocation;
        };
        
        class AstExprIndexExpr: public AstExpr
        {
        public:
            ASTRTTI(AstExprIndexExpr)
 
            AstExprIndexExpr(const Location& location, AstExpr* expr, AstExpr* index)
                : AstExpr(location)
                , expr(expr)
                , index(index)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                if (visitor->visit(this))
                {
                    expr->visit(visitor);
                    index->visit(visitor);
                }
            }
            
            AstExpr* expr;
            AstExpr* index;
        };
        
        class AstExprFunction: public AstExpr
        {
        public:
            ASTRTTI(AstExprFunction)
 
            AstExprFunction(const Location& location, AstLocal* self, const AstArray<AstLocal*>& args, bool vararg, AstStat* body)
                : AstExpr(location)
                , self(self)
                , args(args)
                , vararg(vararg)
                , body(body)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                if (visitor->visit(this))
                    body->visit(visitor);
            }
            
            AstLocal* self;
            AstArray<AstLocal*> args;
            bool vararg;
            
            AstStat* body;
        };
        
        class AstExprTable: public AstExpr
        {
        public:
            ASTRTTI(AstExprTable)
 
            AstExprTable(const Location& location, const AstArray<AstExpr*>& pairs)
                : AstExpr(location)
                , pairs(pairs)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                if (visitor->visit(this))
                {
                    for (size_t i = 0; i < pairs.size; ++i)
                        if (pairs.data[i])
                            pairs.data[i]->visit(visitor);
                }
            }
            
            AstArray<AstExpr*> pairs;
        };
        
        class AstExprUnary: public AstExpr
        {
        public:
            ASTRTTI(AstExprUnary)
 
            enum Op
            {
                Not,
                Minus,
                Len
            };
 
            AstExprUnary(const Location& location, Op op, AstExpr* expr)
                : AstExpr(location)
                , op(op)
                , expr(expr)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                if (visitor->visit(this))
                    expr->visit(visitor);
            }
            
            Op op;
            AstExpr* expr;
        };
        
        class AstExprBinary: public AstExpr
        {
        public:
            ASTRTTI(AstExprBinary)
 
            enum Op
            {
                Add,
                Sub,
                Mul,
                Div,
                Mod,
                Pow,
                Concat,
                CompareNe,
                CompareEq,
                CompareLt,
                CompareLe,
                CompareGt,
                CompareGe,
                And,
                Or
            };
 
            AstExprBinary(const Location& location, Op op, AstExpr* left, AstExpr* right)
                : AstExpr(location)
                , op(op)
                , left(left)
                , right(right)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                if (visitor->visit(this))
                {
                    left->visit(visitor);
                    right->visit(visitor);
                }
            }
            
            Op op;
            AstExpr* left;
            AstExpr* right;
        };

        class AstStatBlock: public AstStat
        {
        public:
            ASTRTTI(AstStatBlock)
 
            AstStatBlock(const Location& location, const AstArray<AstStat*>& body)
                : AstStat(location)
                , body(body)
            {
            }

            virtual void visit(AstVisitor* visitor)
            {
                if (visitor->visit(this))
                {
                    for (size_t i = 0; i < body.size; ++i)
                        body.data[i]->visit(visitor);
                }
            }
            
            AstArray<AstStat*> body;
        };
        
        class AstStatIf: public AstStat
        {
        public:
            ASTRTTI(AstStatIf)
 
            AstStatIf(const Location& location, AstExpr* condition, AstStat* thenbody, AstStat* elsebody)
                : AstStat(location)
                , condition(condition)
                , thenbody(thenbody)
                , elsebody(elsebody)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                if (visitor->visit(this))
                {
                    condition->visit(visitor);
                    thenbody->visit(visitor);
                    
                    if (elsebody)
                        elsebody->visit(visitor);
                }
            }
            
            AstExpr* condition;
            AstStat* thenbody;
            AstStat* elsebody;
        };
        
        class AstStatWhile: public AstStat
        {
        public:
            ASTRTTI(AstStatWhile)
 
            AstStatWhile(const Location& location, AstExpr* condition, AstStat* body)
                : AstStat(location)
                , condition(condition)
                , body(body)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                if (visitor->visit(this))
                {
                    condition->visit(visitor);
                    body->visit(visitor);
                }
            }
            
            AstExpr* condition;
            AstStat* body;
        };
        
        class AstStatRepeat: public AstStat
        {
        public:
            ASTRTTI(AstStatRepeat)
 
            AstStatRepeat(const Location& location, AstExpr* condition, AstStat* body)
                : AstStat(location)
                , condition(condition)
                , body(body)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                if (visitor->visit(this))
                {
                    condition->visit(visitor);
                    body->visit(visitor);
                }
            }
            
            AstExpr* condition;
            AstStat* body;
        };

        class AstStatBreak: public AstStat
        {
        public:
            ASTRTTI(AstStatBreak)
 
            AstStatBreak(const Location& location)
                : AstStat(location)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                visitor->visit(this);
            }
        };
        
        class AstStatReturn: public AstStat
        {
        public:
            ASTRTTI(AstStatReturn)
 
            AstStatReturn(const Location& location, const AstArray<AstExpr*>& list)
                : AstStat(location)
                , list(list)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                if (visitor->visit(this))
                {
                    for (size_t i = 0; i < list.size; ++i)
                        list.data[i]->visit(visitor);
                }
            }

            AstArray<AstExpr*> list;
        };
        
        class AstStatExpr: public AstStat
        {
        public:
            ASTRTTI(AstStatExpr)
 
            AstStatExpr(const Location& location, AstExpr* expr)
                : AstStat(location)
                , expr(expr)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                if (visitor->visit(this))
                    expr->visit(visitor);
            }

            AstExpr* expr;
        };
        
        class AstStatLocal: public AstStat
        {
        public:
            ASTRTTI(AstStatLocal)
 
            AstStatLocal(const Location& location, const AstArray<AstLocal*>& vars, const AstArray<AstExpr*>& values)
                : AstStat(location)
                , vars(vars)
                , values(values)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                if (visitor->visit(this))
                {
                    for (size_t i = 0; i < values.size; ++i)
                        values.data[i]->visit(visitor);
                }
            }
            
            AstArray<AstLocal*> vars;
            AstArray<AstExpr*> values;
        };
        
        class AstStatFor: public AstStat
        {
        public:
            ASTRTTI(AstStatFor)
 
            AstStatFor(const Location& location, AstLocal* var, AstExpr* from, AstExpr* to, AstExpr* step, AstStat* body)
                : AstStat(location)
                , var(var)
                , from(from)
                , to(to)
                , step(step)
                , body(body)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                if (visitor->visit(this))
                {
                    from->visit(visitor);
                    to->visit(visitor);
                    
                    if (step)
                        step->visit(visitor);
                    
                    body->visit(visitor);
                }
            }
            
            AstLocal* var;
            AstExpr* from;
            AstExpr* to;
            AstExpr* step;
            AstStat* body;
        };
        
        class AstStatForIn: public AstStat
        {
        public:
            ASTRTTI(AstStatForIn)
 
            AstStatForIn(const Location& location, const AstArray<AstLocal*>& vars, const AstArray<AstExpr*>& values, AstStat* body)
                : AstStat(location)
                , vars(vars)
                , values(values)
                , body(body)
            {
            }
            
            AstArray<AstLocal*> vars;
            AstArray<AstExpr*> values;
            AstStat* body;
            
            virtual void visit(AstVisitor* visitor)
            {
                if (visitor->visit(this))
                {
                    for (size_t i = 0; i < values.size; ++i)
                        values.data[i]->visit(visitor);
                    
                    body->visit(visitor);
                }
            }
        };
        
        class AstStatAssign: public AstStat
        {
        public:
            ASTRTTI(AstStatAssign)
 
            AstStatAssign(const Location& location, const AstArray<AstExpr*>& vars, const AstArray<AstExpr*>& values)
                : AstStat(location)
                , vars(vars)
                , values(values)
            {
            }
            
            virtual void visit(AstVisitor* visitor)
            {
                if (visitor->visit(this))
                {
                    for (size_t i = 0; i < vars.size; ++i)
                        vars.data[i]->visit(visitor);
                    
                    for (size_t i = 0; i < values.size; ++i)
                        values.data[i]->visit(visitor);
                }
            }
            
            AstArray<AstExpr*> vars;
            AstArray<AstExpr*> values;
        };
        
        template <typename T> class TempVector
        {
        public:
            explicit TempVector(std::vector<T>& storage)
                : storage(storage)
                , offset(storage.size())
            {
            }
            
            ~TempVector()
            {
                RBXASSERT(storage.size() >= offset);
                storage.erase(storage.begin() + offset, storage.end());
            }
            
            const T& operator[](size_t index) const
            {
                return storage[offset + index];
            }
            
            const T& front() const
            {
                return storage[offset];
            }
            
            const T& back() const
            {
                return storage.back();
            }
            
            bool empty() const
            {
                return storage.size() == offset;
            }
            
            size_t size() const
            {
                return storage.size() - offset;
            }
            
            void push_back(const T& item)
            {
                storage.push_back(item);
            }
            
        private:
            std::vector<T>& storage;
            unsigned int offset;
        };
        
        class Parser
        {
        public:
            static AstStat* parse(const char* buffer, size_t bufferSize, AstNameTable& names, Allocator& allocator)
            {
                Parser p(buffer, bufferSize, names, allocator);
                
                return p.parseChunk();
            }
            
        private:
            struct Name;
            
            Parser(const char* buffer, size_t bufferSize, AstNameTable& names, Allocator& allocator)
                : lexer(buffer, bufferSize, names, allocator)
                , allocator(allocator)
                , localMap(AstName())
            {
                Function top;
                top.vararg = true;
                
                functionStack.push_back(top);

                nameSelf = names.addStatic("self");
            }
            
            bool blockFollow(const Lexeme& l)
            {
                return
                    l.type == Lexeme::Eof ||
                    l.type == Lexeme::ReservedElse ||
                    l.type == Lexeme::ReservedElseif ||
                    l.type == Lexeme::ReservedEnd ||
                    l.type == Lexeme::ReservedUntil;
            }
            
            AstStat* parseChunk()
            {
                AstStat* result = parseBlock();
                
                expect(Lexeme::Eof);
                
                return result;
            }
            
        	// chunk ::= {stat [`;']} [laststat [`;']]
        	// block ::= chunk
            AstStat* parseBlock()
            {
                unsigned int localsBegin = saveLocals();
                
                AstStat* result = parseBlockNoScope();
                
                restoreLocals(localsBegin);
                
                return result;
            }
            
            AstStat* parseBlockNoScope()
            {
                TempVector<AstStat*> body(scratchStat);
                
                while (!blockFollow(lexer.current()))
                {
                    std::pair<AstStat*, bool> stat = parseStat();
                    
                    if (lexer.current().type == ';')
                        lexer.next();
                    
                    body.push_back(stat.first);
                    
                    if (stat.second)
                        break;
                }
                
                Location location =
                    body.empty()
                    ? lexer.current().location
                    : Location(body.front()->location, body.back()->location);
                
                return new (allocator) AstStatBlock(location, copy(body));
            }
            
        	// stat ::=
                // varlist `=' explist |
                // functioncall |
                // do block end |
                // while exp do block end |
                // repeat block until exp |
                // if exp then block {elseif exp then block} [else block] end |
                // for Name `=' exp `,' exp [`,' exp] do block end |
                // for namelist in explist do block end |
                // function funcname funcbody |
                // local function Name funcbody |
                // local namelist [`=' explist]
            // laststat ::= return [explist] | break
            std::pair<AstStat*, bool> parseStat()
            {
                switch (lexer.current().type)
                {
                case Lexeme::ReservedIf:
                    return std::make_pair(parseIf(), false);
                case Lexeme::ReservedWhile:
                    return std::make_pair(parseWhile(), false);
                case Lexeme::ReservedDo:
                    return std::make_pair(parseDo(), false);
                case Lexeme::ReservedFor:
                    return std::make_pair(parseFor(), false);
                case Lexeme::ReservedRepeat:
                    return std::make_pair(parseRepeat(), false);
                case Lexeme::ReservedFunction:
                    return std::make_pair(parseFunctionStat(), false);
                case Lexeme::ReservedLocal:
                    return std::make_pair(parseLocal(), false);
                case Lexeme::ReservedReturn:
                    return std::make_pair(parseReturn(), true);
                case Lexeme::ReservedBreak:
                    return std::make_pair(parseBreak(), true);
                default:
                    return std::make_pair(parseAssignmentOrCall(), false);
                }
            }
            
            // if exp then block {elseif exp then block} [else block] end
            AstStat* parseIf()
            {
                Location start = lexer.current().location;
                
                lexer.next(); // if / elseif
                
                AstExpr* cond = parseExpr();
                
				Lexeme matchThenElse = lexer.current();
                expect(Lexeme::ReservedThen);
                lexer.next();
                
                AstStat* thenbody = parseBlock();
                
                AstStat* elsebody = NULL;
                Location end = start;
                
                if (lexer.current().type == Lexeme::ReservedElseif)
                {
                    elsebody = parseIf();
                    end = elsebody->location;
                }
                else
                {
                    if (lexer.current().type == Lexeme::ReservedElse)
                    {
                        matchThenElse = lexer.current();
                        lexer.next();
                        
                        elsebody = parseBlock();
						if (FFlag::StudioVariableIntellesense)
							elsebody->location.begin = matchThenElse.location.end;
                    }
                    
                    end = lexer.current().location;
                    
					expectMatch(Lexeme::ReservedEnd, matchThenElse);
                    lexer.next();
                }

                return new (allocator) AstStatIf(Location(start, end), cond, thenbody, elsebody);
            }
            
            // while exp do block end
            AstStat* parseWhile()
            {
                Location start = lexer.current().location;
                
                lexer.next(); // while
                
                AstExpr* cond = parseExpr();
                
				Lexeme matchDo = lexer.current();
                expect(Lexeme::ReservedDo);
                lexer.next();
                
                functionStack.back().loopDepth++;
                
                AstStat* body = parseBlock();
                
                functionStack.back().loopDepth--;
                
                Location end = lexer.current().location;
                
                expectMatch(Lexeme::ReservedEnd, matchDo);
                lexer.next();
                
                return new (allocator) AstStatWhile(Location(start, end), cond, body);
            }
            
            // repeat block until exp
            AstStat* parseRepeat()
            {
                Location start = lexer.current().location;
                
				Lexeme matchRepeat = lexer.current();
                lexer.next(); // repeat
                
                unsigned int localsBegin = saveLocals();
                
                functionStack.back().loopDepth++;
                
                AstStat* body = parseBlockNoScope();
                
                functionStack.back().loopDepth--;
                
                expectMatch(Lexeme::ReservedUntil, matchRepeat);
                lexer.next();
                
                AstExpr* cond = parseExpr();
                
                restoreLocals(localsBegin);
                
                return new (allocator) AstStatRepeat(Location(start, cond->location), cond, body);
            }
            
            // do block end
            AstStat* parseDo()
            {
				Lexeme matchDo = lexer.current();
                lexer.next(); // do
                
                AstStat* body = parseBlock();
                
                expectMatch(Lexeme::ReservedEnd, matchDo);
                lexer.next();
                
                return body;
            }
            
            // break
            AstStat* parseBreak()
            {
                if (functionStack.back().loopDepth > 0)
                {
                    Location start = lexer.current().location;
                
                    lexer.next(); // break
                    
                    return new (allocator) AstStatBreak(start);
                }
                else
                    throw Error(lexer.current().location, "No loop to break");
            }
            
            // for Name `=' exp `,' exp [`,' exp] do block end |
            // for namelist in explist do block end |
            AstStat* parseFor()
            {
                Location start = lexer.current().location;
                
                lexer.next(); // for
                
                Name varname = parseName();
                
                if (lexer.current().type == '=')
                {
                    lexer.next();
                    
                    AstExpr* from = parseExpr();
                    
                    expect(',');
                    lexer.next();
                    
                    AstExpr* to = parseExpr();
                    
                    AstExpr* step = NULL;
                    
                    if (lexer.current().type == ',')
                    {
                        lexer.next();
                        
                        step = parseExpr();
                    }
                    
                    Lexeme matchDo = lexer.current();
                    expect(Lexeme::ReservedDo);
                    lexer.next();
                    
                    unsigned int localsBegin = saveLocals();
                    
                    AstLocal* var = pushLocal(varname);
                    
                    functionStack.back().loopDepth++;
                    
                    AstStat* body = parseBlock();
                    
                    functionStack.back().loopDepth--;
                    
                    restoreLocals(localsBegin);
                    
                    Location end = lexer.current().location;
                    
                    expectMatch(Lexeme::ReservedEnd, matchDo);
                    lexer.next();
                    
                    return new (allocator) AstStatFor(Location(start, end), var, from, to, step, body);
                }
                else
                {
                    TempVector<Name> names(scratchName);
                    names.push_back(varname);
                    
                    if (lexer.current().type == ',')
                    {
                        lexer.next();
                        
                        parseNameList(names);
                    }
                    
                    expect(Lexeme::ReservedIn);
                    lexer.next();
                    
                    TempVector<AstExpr*> values(scratchExpr);
                    parseExprList(values);
                    
                    Lexeme matchDo = lexer.current();
                    expect(Lexeme::ReservedDo);
                    lexer.next();
                    
                    unsigned int localsBegin = saveLocals();
                    
                    TempVector<AstLocal*> vars(scratchLocal);
                    
                    for (size_t i = 0; i < names.size(); ++i)
                        vars.push_back(pushLocal(names[i]));
                    
                    functionStack.back().loopDepth++;
                    
                    AstStat* body = parseBlock();
                    
                    functionStack.back().loopDepth--;
                    
                    restoreLocals(localsBegin);
                    
                    Location end = lexer.current().location;
                    
                    expectMatch(Lexeme::ReservedEnd, matchDo);
                    lexer.next();
                    
                    return new (allocator) AstStatForIn(Location(start, end), copy(vars), copy(values), body);
                }
            }
            
            // function funcname funcbody |
            // funcname ::= Name {`.' Name} [`:' Name]
            AstStat* parseFunctionStat()
            {
                Location start = lexer.current().location;
                
				Lexeme matchFunction = lexer.current();
                lexer.next();
                
                // parse funcname into a chain of indexing operators
                AstExpr* expr = parseNameExpr();
                
                while (lexer.current().type == '.')
                {
                    lexer.next();
                    
                    Name name = parseName();
                    
                    expr = new (allocator) AstExprIndexName(Location(start, name.location), expr, name.name, name.location);
                }
                
                // finish with :
                bool hasself = false;
                
                if (lexer.current().type == ':')
                {
                    lexer.next();
                    
                    Name name = parseName();
                    
                    expr = new (allocator) AstExprIndexName(Location(start, name.location), expr, name.name, name.location);
                    
                    hasself = true;
                }
                
                AstExpr* body = parseFunctionBody(hasself, matchFunction);
                
                return new (allocator) AstStatAssign(Location(start, body->location), copy(&expr, 1), copy(&body, 1));
            }
            
            // local function Name funcbody |
            // local namelist [`=' explist]
            AstStat* parseLocal()
            {
                Location start = lexer.current().location;
                
                lexer.next(); // local
                
                if (lexer.current().type == Lexeme::ReservedFunction)
                {
					Lexeme matchFunction = lexer.current();
                    lexer.next();
                    
                    Name name = parseName();
                    
                    AstLocal* var = pushLocal(name);
                    
                    AstExpr* body = parseFunctionBody(false, matchFunction);
                    
                    return new (allocator) AstStatLocal(Location(start, body->location), copy(&var, 1), copy(&body, 1));
                }
                else
                {
                    TempVector<Name> names(scratchName);
                    parseNameList(names);
                    
                    TempVector<AstLocal*> vars(scratchLocal);

                    TempVector<AstExpr*> values(scratchExpr);
                    
                    if (lexer.current().type == '=')
                    {
                        lexer.next();
                    
                        parseExprList(values);
                    }
                    
					for (size_t i = 0; i < names.size(); ++i)
						vars.push_back(pushLocal(names[i]));
                    
                    Location end = values.empty() ? names.back().location : values.back()->location;
                    
                    return new (allocator) AstStatLocal(Location(start, end), copy(vars), copy(values));
                }
            }
            
            // return [explist]
            AstStat* parseReturn()
            {
                Location start = lexer.current().location;
                
                lexer.next();
                
                TempVector<AstExpr*> list(scratchExpr);
                
                if (!blockFollow(lexer.current()) && lexer.current().type != ';')
                    parseExprList(list);
                
                Location end = list.empty() ? start : list.back()->location;
                
                return new (allocator) AstStatReturn(Location(start, end), copy(list));
            }
            
            // varlist `=' explist |
            // functioncall |
            AstStat* parseAssignmentOrCall()
            {
                AstExpr* expr = parsePrimaryExpr();
                
                if (expr->is<AstExprCall>())
                    return new (allocator) AstStatExpr(expr->location, expr);
                else
                    return parseAssignment(expr);
            }
            
            bool isExprVar(AstExpr* expr)
            {
                return
                    expr->is<AstExprLocal>() ||
                    expr->is<AstExprGlobal>() ||
                    expr->is<AstExprIndexExpr>() ||
                    expr->is<AstExprIndexName>();
            }
            
            AstStat* parseAssignment(AstExpr* initial)
            {
                // The initial expr has to be a var
                if (!isExprVar(initial))
                    throw Error(initial->location, "Syntax error: expression must be a variable or a field");
                
                TempVector<AstExpr*> vars(scratchExpr);
                vars.push_back(initial);
                
                while (lexer.current().type == ',')
                {
                    lexer.next();
                    
                    AstExpr* expr = parsePrimaryExpr();
                    
                    if (!isExprVar(expr))
                        throw Error(expr->location, "Syntax error: expression must be a variable or a field");
                    
                    vars.push_back(expr);
                }
                
                expect('=');
                lexer.next();
                
                TempVector<AstExpr*> values(scratchExprAux);
                parseExprList(values);
                
                return new (allocator) AstStatAssign(Location(initial->location, values.back()->location), copy(vars), copy(values));
            }
            
            // funcbody ::= `(' [parlist] `)' block end
            // parlist ::= namelist [`,' `...'] | `...'
            AstExprFunction* parseFunctionBody(bool hasself, const Lexeme& matchFunction)
            {
                Location start = lexer.current().location;
                
				Lexeme matchParen = lexer.current();
                expect('(');
                lexer.next();
                
                TempVector<Name> args(scratchName);
                bool vararg = false;
                
                if (lexer.current().type != ')')
                    vararg = parseNameList(args, /* allowDot3= */ true);
                
                expectMatch(')', matchParen);
                lexer.next();
                
                unsigned int localsBegin = saveLocals();
                
                AstLocal* self = NULL;
                
                if (hasself)
                {
                    self = pushLocal(Name(nameSelf, start));
                }
                
                TempVector<AstLocal*> vars(scratchLocal);
                
                for (size_t i = 0; i < args.size(); ++i)
                    vars.push_back(pushLocal(args[i]));
                
                Function fun;
                fun.vararg = vararg;

                functionStack.push_back(fun);
                
                AstStat* body = parseBlock();
                
                functionStack.pop_back();
                
                restoreLocals(localsBegin);
                
                Location end = lexer.current().location;
                
                expectMatch(Lexeme::ReservedEnd, matchFunction);
                lexer.next();
                
                return new (allocator) AstExprFunction(Location(start, end), self, copy(vars), vararg, body);
            }
            
            // explist ::= {exp `,'} exp
            void parseExprList(TempVector<AstExpr*>& result)
            {
                result.push_back(parseExpr());
                
                while (lexer.current().type == ',')
                {
                    lexer.next();
                    
                    result.push_back(parseExpr());
                }
            }
            
            // namelist ::= Name {`,' Name}
            bool parseNameList(TempVector<Name>& result, bool allowDot3 = false)
            {
                if (lexer.current().type == Lexeme::Dot3 && allowDot3)
                {
                    lexer.next();
                    
                    return true;
                }
                
                result.push_back(parseName());
                
                while (lexer.current().type == ',')
                {
                    lexer.next();
                    
                    if (lexer.current().type == Lexeme::Dot3 && allowDot3)
                    {
                        lexer.next();
                        
                        return true;
                    }
                    else
                    {
                        result.push_back(parseName());
                    }
                }
                
                return false;
            }
            
            static boost::optional<AstExprUnary::Op> parseUnaryOp(const Lexeme& l)
            {
                if (l.type == Lexeme::ReservedNot)
                    return AstExprUnary::Not;
                else if (l.type == '-')
                    return AstExprUnary::Minus;
                else if (l.type == '#')
                    return AstExprUnary::Len;
                else
                    return boost::optional<AstExprUnary::Op>();
            }
            
            static boost::optional<AstExprBinary::Op> parseBinaryOp(const Lexeme& l)
            {
                if (l.type == '+')
                    return AstExprBinary::Add;
                else if (l.type == '-')
                    return AstExprBinary::Sub;
                else if (l.type == '*')
                    return AstExprBinary::Mul;
                else if (l.type == '/')
                    return AstExprBinary::Div;
                else if (l.type == '%')
                    return AstExprBinary::Mod;
                else if (l.type == '^')
                    return AstExprBinary::Pow;
                else if (l.type == Lexeme::Dot2)
                    return AstExprBinary::Concat;
                else if (l.type == Lexeme::NotEqual)
                    return AstExprBinary::CompareNe;
                else if (l.type == Lexeme::Equal)
                    return AstExprBinary::CompareEq;
                else if (l.type == '<')
                    return AstExprBinary::CompareLt;
                else if (l.type == Lexeme::LessEqual)
                    return AstExprBinary::CompareLe;
                else if (l.type == '>')
                    return AstExprBinary::CompareGt;
                else if (l.type == Lexeme::GreaterEqual)
                    return AstExprBinary::CompareGe;
                else if (l.type == Lexeme::ReservedAnd)
                    return AstExprBinary::And;
                else if (l.type == Lexeme::ReservedOr)
                    return AstExprBinary::Or;
                else
                    return boost::optional<AstExprBinary::Op>();
            }
            
            struct BinaryOpPriority
            {
                unsigned char left, right;
            };
            
            // subexpr -> (simpleexp | unop subexpr) { binop subexpr }
            // where `binop' is any binary operator with a priority higher than `limit'
            std::pair<AstExpr*, boost::optional<AstExprBinary::Op> > parseSubExpr(unsigned int limit)
            {
                static const BinaryOpPriority binaryPriority[] =
                {
                    {6, 6}, {6, 6}, {7, 7}, {7, 7}, {7, 7},  // `+' `-' `/' `%'
                    {10, 9}, {5, 4},                 // power and concat (right associative)
                    {3, 3}, {3, 3},                  // equality and inequality
                    {3, 3}, {3, 3}, {3, 3}, {3, 3},  // order
                    {2, 2}, {1, 1}                   // logical (and/or)
                };
                
                const unsigned int unaryPriority = 8;
                
                Location start = lexer.current().location;
                
                AstExpr* expr;
                
                if (boost::optional<AstExprUnary::Op> uop = parseUnaryOp(lexer.current()))
                {
                    lexer.next();
                    
                    AstExpr* subexpr = parseSubExpr(unaryPriority).first;
                    
                    expr = new (allocator) AstExprUnary(Location(start, subexpr->location), uop.get(), subexpr);
                }
                else
                {
                    expr = parseSimpleExpr();
                }
                
                // expand while operators have priorities higher than `limit'
                boost::optional<AstExprBinary::Op> op = parseBinaryOp(lexer.current());
                
                while (op && binaryPriority[op.get()].left > limit)
                {
                    lexer.next();
                    
                    // read sub-expression with higher priority
                    std::pair<AstExpr*, boost::optional<AstExprBinary::Op> > next = parseSubExpr(binaryPriority[op.get()].right);
                    
                    expr = new (allocator) AstExprBinary(Location(start, next.first->location), op.get(), expr, next.first);
                    op = next.second;
                }
                
                return std::make_pair(expr, op);  // return first untreated operator
            }
            
            AstExpr* parseExpr()
            {
                return parseSubExpr(0).first;
            }
            
            // NAME
            AstExpr* parseNameExpr()
            {
                Name name = parseName();
                
                AstLocal* const * value = localMap.find(name.name);
                    
                if (value && *value)
                {
                    AstLocal* local = *value;
                        
                    return new (allocator) AstExprLocal(name.location, local, local->functionDepth != functionStack.size());
                }
                
                return new (allocator) AstExprGlobal(name.location, name.name);
            }
            
            // prefixexp -> NAME | '(' expr ')'
            AstExpr* parsePrefixExpr()
            {
                if (lexer.current().type == '(')
                {
                    Location start = lexer.current().location;
                    
					Lexeme matchParen = lexer.current();
                    lexer.next();
                    
                    AstExpr* expr = parseExpr();
                    
                    Location end = lexer.current().location;
                    
                    expectMatch(')', matchParen);
                    lexer.next();
                    
                    return new (allocator) AstExprGroup(Location(start, end), expr);
                }
                else
                {
                    return parseNameExpr();
                }
            }
            
            // primaryexp -> prefixexp { `.' NAME | `[' exp `]' | `:' NAME funcargs | funcargs }
            AstExpr* parsePrimaryExpr()
            {
                Location start = lexer.current().location;
                
                AstExpr* expr = parsePrefixExpr();
                
                while (true)
                {
                    if (lexer.current().type == '.')
                    {
                        lexer.next();
                        
                        Name index = parseName();
                        
                        expr = new (allocator) AstExprIndexName(Location(start, index.location), expr, index.name, index.location);
                    }
                    else if (lexer.current().type == '[')
                    {
                        Lexeme matchBracket = lexer.current();
                        lexer.next();
                        
                        AstExpr* index = parseExpr();
                        
                        Location end = lexer.current().location;
                        
                        expectMatch(']', matchBracket);
                        lexer.next();
                        
                        expr = new (allocator) AstExprIndexExpr(Location(start, end), expr, index);
                    }
                    else if (lexer.current().type == ':')
                    {
                        lexer.next();
                        
                        Name index = parseName();
                        AstExpr* func = new (allocator) AstExprIndexName(Location(start, index.location), expr, index.name, index.location);
                        
                        expr = parseFunctionArgs(func, true);
                    }
                    else if (lexer.current().type == '{' || lexer.current().type == '(' || lexer.current().type == Lexeme::String)
                    {
                        expr = parseFunctionArgs(expr, false);
                    }
                    else
                    {
                        break;
                    }
                }
                
                return expr;
            }
 
            
            // simpleexp -> NUMBER | STRING | NIL | true | false | ... | constructor | FUNCTION body | primaryexp
            AstExpr* parseSimpleExpr()
            {
                Location start = lexer.current().location;
                
                if (lexer.current().type == Lexeme::ReservedNil)
                {
                    lexer.next();
                    
                    return new (allocator) AstExprConstantNil(start);
                }
                else if (lexer.current().type == Lexeme::ReservedTrue)
                {
                    lexer.next();
                    
                    return new (allocator) AstExprConstantBool(start, true);
                }
                else if (lexer.current().type == Lexeme::ReservedFalse)
                {
                    lexer.next();
                    
                    return new (allocator) AstExprConstantBool(start, false);
                }
                else if (lexer.current().type == Lexeme::ReservedFunction)
                {
					Lexeme matchFunction = lexer.current();
                    lexer.next();
                    
                    return parseFunctionBody(false, matchFunction);
                }
                else if (lexer.current().type == Lexeme::Number)
                {
                    const char* datap = lexer.current().data->c_str();
                    char* dataend = NULL;
                    
                    double value = strtod(datap, &dataend);

					// maybe a hexadecimal constant?
					if (*dataend == 'x' || *dataend == 'X')
						value = strtoul(datap, &dataend, 16);
                    
                    if (*dataend == 0)
                    {
                        lexer.next();
                    
                        return new (allocator) AstExprConstantNumber(start, value);
                    }
                    else
                        throw Error(lexer.current().location, "Malformed number");
                }
                else if (lexer.current().type == Lexeme::String)
                {
                    AstArray<char> value = copy(*lexer.current().data);
                    
                    lexer.next();
                    
                    return new (allocator) AstExprConstantString(start, value);
                }
                else if (lexer.current().type == Lexeme::Dot3)
                {
                    if (functionStack.back().vararg)
                    {
                        lexer.next();
                        
                        return new (allocator) AstExprVarargs(start);
                    }
                    else
                        throw Error(lexer.current().location, "Cannot use '...' outside a vararg function");
                }
                else if (lexer.current().type == '{')
                {
                    return parseTableConstructor();
                }
                else
                {
                    return parsePrimaryExpr();
                }
            }
            
            // args ::=  `(' [explist] `)' | tableconstructor | String
            AstExprCall* parseFunctionArgs(AstExpr* func, bool self)
            {
                if (lexer.current().type == '(')
                {
                    if (func->location.end.line != lexer.current().location.begin.line)
                        throw Error(lexer.current().location, "Ambiguous syntax: this looks like an argument list for a function call, but could also be a start of new statement");

					Lexeme matchParen = lexer.current();
                    lexer.next();
                    
                    TempVector<AstExpr*> args(scratchExpr);
                    
                    if (lexer.current().type != ')')
                        parseExprList(args);
                    
                    Location end = lexer.current().location;
                    
                    expectMatch(')', matchParen);
                    lexer.next();
                    
                    return new (allocator) AstExprCall(Location(func->location, end), func, copy(args), self);
                }
                else if (lexer.current().type == '{')
                {
                    AstExpr* expr = parseTableConstructor();
                    
                    return new (allocator) AstExprCall(Location(func->location, expr->location), func, copy(&expr, 1), self);
                }
                else if (lexer.current().type == Lexeme::String)
                {
                    AstExpr* expr = new (allocator) AstExprConstantString(lexer.current().location, copy(*lexer.current().data));
                    
                    lexer.next();
                    
                    return new (allocator) AstExprCall(Location(func->location, expr->location), func, copy(&expr, 1), self);
                }
                else
                {
                    throw Error(lexer.current().location, "Expected '(', '{' or <string>, got %s", lexer.current().toString().c_str());
                }
            }

        	// tableconstructor ::= `{' [fieldlist] `}'
        	// fieldlist ::= field {fieldsep field} [fieldsep]
        	// field ::= `[' exp `]' `=' exp | Name `=' exp | exp
        	// fieldsep ::= `,' | `;'
            AstExpr* parseTableConstructor()
            {
                TempVector<AstExpr*> pairs(scratchExpr);
                
                Location start = lexer.current().location;
                
                Lexeme matchBrace = lexer.current();
                expect('{');
                lexer.next();
                
                while (lexer.current().type != '}')
                {
                    if (lexer.current().type == '[')
                    {
						Lexeme matchLocationBracket = lexer.current();
                        lexer.next();
                        
                        AstExpr* key = parseExpr();
                        
                        expectMatch(']', matchLocationBracket);
                        lexer.next();
                        
                        expect('=');
                        lexer.next();
                        
                        AstExpr* value = parseExpr();
                        
                        pairs.push_back(key);
                        pairs.push_back(value);
                    }
                    else
                    {
                        AstExpr* expr = parseExpr();
                        
                        if (lexer.current().type == '=')
                        {
                            lexer.next();
                            
                            AstName name;
                            
                            if (AstExprLocal* e = expr->as<AstExprLocal>())
                                name = e->local->name;
                            else if (AstExprGlobal* e = expr->as<AstExprGlobal>())
                                name = e->name;
                            else
                                throw Error(expr->location, "Expected a name, got a complex expression");
                            
                            AstArray<char> nameString;
                            nameString.data = const_cast<char*>(name.value);
                            nameString.size = strlen(name.value);
                            
                            AstExpr* key = new (allocator) AstExprConstantString(expr->location, nameString);
                            AstExpr* value = parseExpr();

                            pairs.push_back(key);
                            pairs.push_back(value);
                        }
                        else
                        {
                            pairs.push_back(NULL);
                            pairs.push_back(expr);
                        }
                    }
                    
                    if (lexer.current().type == ',' || lexer.current().type == ';')
                        lexer.next();
					else
						expectMatch('}', matchBrace);
                }
                
                Location end = lexer.current().location;
                
                lexer.next();
                
                return new (allocator) AstExprTable(Location(start, end), copy(pairs));
            }
            
            // Name
            Name parseName()
            {
                if (lexer.current().type != Lexeme::Name)
                    throw Error(lexer.current().location, "Expected identifier, got %s", lexer.current().toString().c_str());
                
                Name result(AstName(lexer.current().name), lexer.current().location);
                
                lexer.next();
                
                return result;
            }
            
            AstLocal* pushLocal(const Name& name)
            {
                AstLocal*& local = localMap[name.name];
                
                local = new (allocator) AstLocal(name.name, name.location, local, functionStack.size());
                
                localStack.push_back(local);
                
                return local;
            }
            
            unsigned int saveLocals()
            {
                return localStack.size();
            }
            
            void restoreLocals(unsigned int offset)
            {
                for (size_t i = localStack.size(); i > offset; --i)
                {
                    AstLocal* l = localStack[i - 1];
                    
                    localMap[l->name] = l->shadow;
                }
                
                localStack.resize(offset);
            }
            
            void expect(char value)
			{
                expect(static_cast<Lexeme::Type>(static_cast<unsigned char>(value)));
			}
            
            void expect(Lexeme::Type type)
            {
                if (lexer.current().type != type)
                    throw Error(lexer.current().location, "Expected %s, got %s", Lexeme(Location(Position(0, 0), 0), type).toString().c_str(), lexer.current().toString().c_str());
            }
        
            void expectMatch(char value, const Lexeme& begin)
			{
                expectMatch(static_cast<Lexeme::Type>(static_cast<unsigned char>(value)), begin);
			}

            void expectMatch(Lexeme::Type type, const Lexeme& begin)
            {
                if (lexer.current().type != type)
				{
                    std::string typeString = Lexeme(Location(Position(0, 0), 0), type).toString();

                    if (lexer.current().location.begin.line == begin.location.begin.line)
                        throw Error(lexer.current().location, "Expected %s (to close %s at column %d), got %s", typeString.c_str(), begin.toString().c_str(), begin.location.begin.column + 1, lexer.current().toString().c_str());
                    else
                        throw Error(lexer.current().location, "Expected %s (to close %s at line %d), got %s", typeString.c_str(), begin.toString().c_str(), begin.location.begin.line + 1, lexer.current().toString().c_str());
                }
            }
        
            template <typename T> AstArray<T> copy(const T* data, size_t size)
            {
                AstArray<T> result;
                
                result.data = size ? new (allocator) T[size] : NULL;
                result.size = size;
                
                std::copy(data, data + size, result.data);
                
                return result;
            }

            template <typename T> AstArray<T> copy(const TempVector<T>& data)
            {
                return copy(data.empty() ? NULL : &data[0], data.size());
            }
            
            AstArray<char> copy(const std::string& data)
            {
                AstArray<char> result = copy(data.c_str(), data.size() + 1);
                
                result.size = data.size();
                
                return result;
            }
            
            struct Function
            {
                bool vararg;
                unsigned int loopDepth;

                Function()
                    : vararg(false)
                    , loopDepth(0)
                {
                }
            };
            
            struct Local
            {
                AstLocal* local;
                unsigned int offset;
                
                Local()
                    : local(NULL)
                    , offset(0)
                {
                }
            };
            
            struct Name
            {
                AstName name;
                Location location;
                
                Name(const AstName& name, const Location& location)
                    : name(name)
                    , location(location)
                {
                }
            };

            Lexer lexer;
            Allocator& allocator;

            AstName nameSelf;
            
            std::vector<Function> functionStack;
            
            DenseHashMap<AstName, AstLocal*> localMap;
            std::vector<AstLocal*> localStack;
            
            std::vector<AstStat*> scratchStat;
            std::vector<AstExpr*> scratchExpr;
            std::vector<AstExpr*> scratchExprAux;
            std::vector<Name> scratchName;
            std::vector<AstLocal*> scratchLocal;
        };
        
		RBX_PRINTF_ATTR(4, 5) void emitWarning(ScriptAnalyzer::Result& result, ScriptAnalyzer::WarningCode code, const Location& location, const char* format, ...)
		{
            if (FInt::ScriptAnalyzerIgnoreWarnings & (1 << code))
                return;
            
            va_list args;
            va_start(args, format);
            std::string message = vformat(format, args);
            va_end(args);
            
			Warning warning(code, ScriptAnalyzer::Location(ScriptAnalyzer::Position(location.begin.line, location.begin.column), ScriptAnalyzer::Position(location.end.line, location.end.column)), message);
			result.warnings.push_back(warning);
		}
        
        struct AnalyzerContext
        {
            ScriptAnalyzer::Result* result;
            
            AstStat* root;
            
            AstName placeholder;
            DenseHashMap<AstName, Reflection::Variant> builtinGlobals;
            DenseHashMap<AstName, const char*> deprecatedGlobals;
            
            AnalyzerContext(ScriptAnalyzer::Result* result, AstStat* root)
                : result(result)
                , root(root)
                , builtinGlobals(AstName())
                , deprecatedGlobals(AstName())
            {
            }

			void fillNames(AstNameTable& names)
			{
				placeholder = names.getOrAdd("_");
			}
            
            void fillBuiltinGlobals(AstNameTable& names, lua_State* L)
            {
                lua_pushvalue(L, LUA_GLOBALSINDEX);
                lua_pushnil(L);
                
                while (lua_next(L, -2))
                {
                    if (const char* name = lua_tostring(L, -2))
                    {
                        int top = lua_gettop(L);

                        Reflection::Variant value;

                        try
                        {
                            Lua::LuaArguments::get(L, -1, value, true);
                        }
                        catch (RBX::base_exception&)
                        {
                            // swallow the exception - mainly happens if you have tables with non-string keys in the environment
                        }

						lua_settop(L, top);
                        
                        builtinGlobals[names.getOrAdd(name)] = value;
                    }
                    
                    lua_pop(L, 1);
                }
                
                lua_pop(L, 1);
            }
            
            void fillDeprecatedGlobals(AstNameTable& names)
            {
                // Global names with inconsistent case
                deprecatedGlobals[names.getOrAdd("Game")] = "game";
                deprecatedGlobals[names.getOrAdd("Workspace")] = "workspace";
                
                // Global functions with inconsistent case
                deprecatedGlobals[names.getOrAdd("Delay")] = "delay";
                deprecatedGlobals[names.getOrAdd("ElapsedTime")] = "elapsedTime";
                deprecatedGlobals[names.getOrAdd("Spawn")] = "spawn";
                deprecatedGlobals[names.getOrAdd("Wait")] = "wait";
                
                // Global functions that should not exist
                deprecatedGlobals[names.getOrAdd("DebuggerManager")] = NULL;
                deprecatedGlobals[names.getOrAdd("PluginManager")] = NULL;
                deprecatedGlobals[names.getOrAdd("printidentity")] = NULL;
                deprecatedGlobals[names.getOrAdd("Stats")] = NULL;
                deprecatedGlobals[names.getOrAdd("stats")] = NULL;
                deprecatedGlobals[names.getOrAdd("Version")] = NULL;
                deprecatedGlobals[names.getOrAdd("version")] = NULL;
                deprecatedGlobals[names.getOrAdd("settings")] = NULL;
                
                // Lua globals that don't work (security) but are still defined
                deprecatedGlobals[names.getOrAdd("load")] = NULL;
                deprecatedGlobals[names.getOrAdd("dofile")] = NULL;
                deprecatedGlobals[names.getOrAdd("loadfile")] = NULL;
            }
        };
        
        struct AnalyzerPass
        {
            const char* name;
            
            void (*process)(AnalyzerContext& context);
        };

		class AnalyzerPassIntellesenseLocal: AstVisitor
		{
		public:
			
			static void process (AnalyzerContext& context)
			{
				AnalyzerPassIntellesenseLocal pass;
				pass.context = &context;
				pass.firstResult = true;
				
				context.root->visit(&pass);

				pass.report();
			}

		private:
			AnalyzerContext* context;
			
			ScriptAnalyzer::IntellesenseResult result;

			ScriptAnalyzer::IntellesenseResult* currentBlock;
			
			ScriptAnalyzer::Location lastLocation;

			int positionInBlock;

			bool firstResult;


			void report()
			{
				if (!firstResult)
					context->result->intellesenseAnalysis = result.children;
			}

			virtual bool visit(AstStatLocal* node)
			{
				ScriptAnalyzer::IntellesenseResult localResult;

				localResult.name = node->vars.data[0]->name.value;
				localResult.location = node->location;
				
				localResult.isLocal = true;
				localResult.isFunction = false;

				currentBlock->children[positionInBlock] = localResult;
				return true;
			}

			virtual bool visit(AstStat* node)
			{
				lastLocation = node->location;
				return true;
			}

			virtual bool visit(AstExprGlobal* node)
			{
				if (context->builtinGlobals.find(node->name) || !currentBlock->children[positionInBlock].name.empty())
					return false;

				ScriptAnalyzer::IntellesenseResult globalResult;

				globalResult.name = node->name.value;
				globalResult.location = node->location;

				globalResult.isLocal = false;
				globalResult.isFunction = false;
								
				currentBlock->children[positionInBlock] = globalResult;
				return true;
			}

			virtual bool visit(AstExprCall* node)
			{
				return false;
			}

			virtual bool visit(AstExprFunction* node)
			{				
				if (AstStatBlock* blockNode = dynamic_cast<AstStatBlock*>(node->body))
				{					
					ScriptAnalyzer::IntellesenseResult* localResult = &currentBlock->children[positionInBlock];
					localResult->isFunction = true;
					localResult->location = node->location;

					int numberOfLocalVars = (int)node->args.size;

					localResult->children.resize(blockNode->body.size + numberOfLocalVars);

					for (int i = 0; i < numberOfLocalVars; ++i)
					{
						localResult->children[i].name = node->args.data[i]->name.value;
						localResult->children[i].location = node->args.data[i]->location;
						localResult->children[i].isLocal = true;
					}

					for (size_t i = 0; i < blockNode->body.size; ++i)
					{
						positionInBlock = i + numberOfLocalVars;
						currentBlock = localResult;
						blockNode->body.data[i]->visit(this);
					}
				}
				return false;	
			}
			
			virtual bool visit(AstStatIf* node)
			{
				if (visit((class AstStat*)node))
				{
					node->condition->visit(this);

					ScriptAnalyzer::IntellesenseResult* localResult = &currentBlock->children[positionInBlock];
					localResult->location = lastLocation;

					int sizeOfBlock = node->elsebody ? 2 : 1;

					currentBlock = localResult;
					
					currentBlock->children.resize(sizeOfBlock);
					positionInBlock = 0;

					if (node->elsebody)
						lastLocation.end = node->elsebody->location.begin;

					node->thenbody->visit(this);

					if (node->elsebody)
					{
						lastLocation.begin = node->elsebody->location.begin;
						lastLocation.end = localResult->location.end;

						currentBlock = localResult;
						positionInBlock = 1;
						node->elsebody->visit(this);
					}
				}

				return false;
			}
			
			virtual bool visit(AstStatFor* node)
			{
				if (AstStatBlock* blockNode = dynamic_cast<AstStatBlock*>(node->body))
				{
					ScriptAnalyzer::IntellesenseResult* localResult = &currentBlock->children[positionInBlock];

					localResult->location = node->location;
					localResult->children.resize(blockNode->body.size + 1);
					
					localResult->children[0].name = node->var->name.value;
					localResult->children[0].location = node->var->location;
					localResult->children[0].isLocal = true;
					
					for (size_t i = 0; i < blockNode->body.size; ++i)
					{
						positionInBlock = i + 1;
						currentBlock = localResult;
						blockNode->body.data[i]->visit(this);
					}
				}
				
				return false;
			}

			virtual bool visit(AstStatForIn* node)
			{
				if (AstStatBlock* blockNode = dynamic_cast<AstStatBlock*>(node->body))
				{
					ScriptAnalyzer::IntellesenseResult* localResult = &currentBlock->children[positionInBlock];

					localResult->location = node->location;

					int numberOfLocalVars = (int)node->vars.size;

					localResult->children.resize(blockNode->body.size + numberOfLocalVars);

					for (int i = 0; i < numberOfLocalVars; ++i)
					{
						localResult->children[i].name = node->vars.data[i]->name.value;
						localResult->children[i].location = node->vars.data[i]->location;
						localResult->children[i].isLocal = true;
					}

					for (size_t i = 0; i < blockNode->body.size; ++i)
					{
						positionInBlock = i + numberOfLocalVars;
						currentBlock = localResult;
						blockNode->body.data[i]->visit(this);
					}
				}
				return false;
			}

			virtual bool visit(AstStatBlock* node)
			{
				ScriptAnalyzer::IntellesenseResult* localResult;
				
				if (firstResult)
				{
					result.location = node->location;
					localResult = &result;
					firstResult = false;
				}
				else
				{
					ScriptAnalyzer::IntellesenseResult blockResult;
					blockResult.location = lastLocation;
					currentBlock->children[positionInBlock] = blockResult;
					localResult = &currentBlock->children[positionInBlock];
				}

				localResult->children.resize(node->body.size);

				for (size_t i = 0; i < node->body.size; ++i)
				{
					positionInBlock = i;
					currentBlock = localResult;
					node->body.data[i]->visit(this);
				}
				return false;				
			}

		};
        
        class AnalyzerPassWarnGlobalLocal: AstVisitor
        {
        public:
            static void process(AnalyzerContext& context)
            {
                AnalyzerPassWarnGlobalLocal pass;
                pass.context = &context;
                
                for (DenseHashMap<AstName, Reflection::Variant>::const_iterator it = context.builtinGlobals.begin(); it != context.builtinGlobals.end(); ++it)
					pass.globals[*it].builtin = true;
                
				for (DenseHashMap<AstName, const char*>::const_iterator it = context.deprecatedGlobals.begin(); it != context.deprecatedGlobals.end(); ++it)
					pass.globals[*it].deprecated = &it.getItem().value;
                
                context.root->visit(&pass);
                
                pass.report();
            }
            
        private:
            struct Global
            {
                AstExprGlobal* firstRef;
                AstExprFunction* onlyFunctionRef;
                bool assigned;
                bool builtin;
                const char* const * deprecated;
                
                Global()
                    : firstRef(NULL)
                    , onlyFunctionRef(NULL)
                    , assigned(false)
                    , builtin(false)
                    , deprecated(NULL)
                {
                }
            };
            
            AnalyzerContext* context;
            
            DenseHashMap<AstName, Global> globals;
            std::vector<AstExprGlobal*> globalRefs;
            std::vector<AstExprFunction*> functionStack;
            
            AnalyzerPassWarnGlobalLocal()
                : globals(AstName())
            {
            }
            
            void report()
            {
                for (size_t i = 0; i < globalRefs.size(); ++i)
                {
                    AstExprGlobal* gv = globalRefs[i];
                    Global* g = globals.find(gv->name);
                    
                    if (!g || (!g->assigned && !g->builtin))
                        emitWarning(*context->result, ScriptAnalyzer::Warning_UnknownGlobal, gv->location, "Unknown global '%s'", gv->name.value);
					else if (g->deprecated)
                    {
                        if (*g->deprecated)
                            emitWarning(*context->result, ScriptAnalyzer::Warning_DeprecatedGlobal, gv->location, "Global '%s' is deprecated, use '%s' instead", gv->name.value, *g->deprecated);
                        else
                            emitWarning(*context->result, ScriptAnalyzer::Warning_DeprecatedGlobal, gv->location, "Global '%s' is deprecated", gv->name.value);
                    }
                }

                for (DenseHashMap<AstName, Global>::const_iterator it = globals.begin(); it != globals.end(); ++it)
                {
                    const Global& g = it.getItem().value;
                    
					if (g.onlyFunctionRef && g.assigned && g.firstRef->name != context->placeholder)
                        emitWarning(*context->result, ScriptAnalyzer::Warning_GlobalUsedAsLocal, g.firstRef->location, "Global '%s' is only used in the enclosing function; consider changing it to local", g.firstRef->name.value);
                }
            }

            virtual bool visit(AstExprFunction* node)
            {
                functionStack.push_back(node);
                
                node->body->visit(this);
                
                functionStack.pop_back();
                
                return false;
            }

            virtual bool visit(AstExprGlobal* node)
            {
				trackGlobalRef(node);

				if (node->name == context->placeholder)
                    emitWarning(*context->result, ScriptAnalyzer::Warning_Placeholder, node->location, "Placeholder value '_' is read here; consider using a named variable");

                return true;
            }
            
            virtual bool visit(AstExprLocal* node)
            {
                if (node->local->name == context->placeholder)
                    emitWarning(*context->result, ScriptAnalyzer::Warning_Placeholder, node->location, "Placeholder value '_' is read here; consider using a named variable");
                
                return true;
            }
            
            virtual bool visit(AstStatAssign* node)
            {
                for (size_t i = 0; i < node->vars.size; ++i)
                {
                    AstExpr* var = node->vars.data[i];
                    
                    if (AstExprGlobal* gv = var->as<AstExprGlobal>())
					{
                        Global& g = globals[gv->name];

						if (g.builtin)
							emitWarning(*context->result, ScriptAnalyzer::Warning_BuiltinGlobalWrite, gv->location, "Built-in global '%s' is overwritten here; consider using a local or changing the name", gv->name.value);
						else
							g.assigned = true;

						trackGlobalRef(gv);
					}
					else if (var->is<AstExprLocal>())
						;
                    else
                        var->visit(this);
                }
                
                for (size_t i = 0; i < node->values.size; ++i)
                    node->values.data[i]->visit(this);
                
                return false;
            }

			void trackGlobalRef(AstExprGlobal* node)
			{
                AstExprFunction* function = functionStack.empty() ? NULL : functionStack.back();
                
                Global& g = globals[node->name];
                
                globalRefs.push_back(node);
                
				if (!g.firstRef)
                {
                    g.firstRef = node;
					g.onlyFunctionRef = function;
                }
                else
                {
					if (g.onlyFunctionRef != function)
                        g.onlyFunctionRef = NULL;
                }
			}
        };
        
        class AnalyzerPassWarnSameLineStatement: AstVisitor
        {
        public:
            static void process(AnalyzerContext& context)
            {
                AnalyzerPassWarnSameLineStatement pass;
                
                pass.result = context.result;
                pass.lastLine = -1;
                
                context.root->visit(&pass);
            }
            
        private:
            ScriptAnalyzer::Result* result;
            int lastLine;
            
            virtual bool visit(AstStatBlock* node)
            {
                for (size_t i = 1; i < node->body.size; ++i)
                {
                    const Location& last = node->body.data[i - 1]->location;
                    const Location& location = node->body.data[i]->location;
                    
                    if (location.begin.line == last.end.line && location.begin.line != lastLine)
                    {
                        emitWarning(*result, ScriptAnalyzer::Warning_SameLineStatement, location, "A new statement is on the same line");
                        
                        // Warn once per line
                        lastLine = location.begin.line;
                    }
                }
                
                return true;
            }
        };
        
        class AnalyzerPassWarnMultiLineStatement: AstVisitor
        {
        public:
            static void process(AnalyzerContext& context)
            {
                AnalyzerPassWarnMultiLineStatement pass;
                pass.result = context.result;
                
                context.root->visit(&pass);
            }
            
        private:
            ScriptAnalyzer::Result* result;
            
            struct Statement
            {
                Location start;
                unsigned int lastLine;
                bool flagged;
            };
            
            std::vector<Statement> stack;
            
            virtual bool visit(AstExpr* node)
            {
                Statement& top = stack.back();
                
                if (!top.flagged)
                {
                    Location location = node->location;
                
                    if (location.begin.line > top.lastLine)
                    {
                        top.lastLine = location.begin.line;
                        
                        if (location.begin.column <= top.start.begin.column)
                        {
                            emitWarning(*result, ScriptAnalyzer::Warning_MultiLineStatement, location, "Statement spans multiple lines; use indentation to silence");
                            
                            top.flagged = true;
                        }
                    }
                }
                
                return true;
            }
            
            virtual bool visit(AstExprTable* node)
            {
                return false;
            }

			virtual bool visit(AstStatRepeat* node)
			{
				node->body->visit(this);

                return false;
			}
            
            virtual bool visit(AstStatBlock* node)
            {
                for (size_t i = 0; i < node->body.size; ++i)
                {
                    AstStat* stmt = node->body.data[i];

                    Statement s = { stmt->location, stmt->location.begin.line, false };
                    stack.push_back(s);
                    
                    stmt->visit(this);
                    
                    stack.pop_back();
                }
                
                return false;
            }
        };
        
        class AnalyzerPassWarnDotCall: AstVisitor
        {
        public:
            static void process(AnalyzerContext& context)
            {
                AnalyzerPassWarnDotCall pass;
                pass.result = context.result;
                
                context.root->visit(&pass);
            }
            
        private:
            ScriptAnalyzer::Result* result;
            
            virtual bool visit(AstExprCall* node)
            {
                if (!node->self)
                    if (AstExprIndexName* index = node->func->as<AstExprIndexName>())
                        if (!index->expr->is<AstExprLocal>() && !index->expr->is<AstExprGlobal>())
                        {
                            emitWarning(*result, ScriptAnalyzer::Warning_DotCall, node->func->location, "Expected ':' not '.' calling member function %s", index->index.value);
                        }
                
                return true;
            }
        };
        
        class AnalyzerPassWarnUnknownType: AstVisitor
        {
        public:
            static void process(AnalyzerContext& context)
            {
                AnalyzerPassWarnUnknownType pass;
                pass.result = context.result;
                
                context.root->visit(&pass);
            }
            
        private:
            ScriptAnalyzer::Result* result;
            
            struct ClassDescriptorNamePredicate
            {
                bool operator()(const Name& lhs, const Reflection::ClassDescriptor* rhs) const
                {
                    return lhs < rhs->name;
                }
                
                bool operator()(const Reflection::ClassDescriptor* lhs, const Name& rhs) const
                {
                    return lhs->name < rhs;
                }
            };
            
            void validateType(AstExprConstantString* arg, bool creatable)
            {
                const Name& name = Name::lookup(std::string(arg->value.data, arg->value.size));
 
                if (name.empty() || !std::binary_search(Reflection::ClassDescriptor::all_begin(), Reflection::ClassDescriptor::all_end(), name, ClassDescriptorNamePredicate()))
                    emitWarning(*result, ScriptAnalyzer::Warning_UnknownType, arg->location, "Unknown type name '%s'", arg->value.data);
                else if (creatable && !Creatable<Instance>::getCreator(name))
                    emitWarning(*result, ScriptAnalyzer::Warning_UnknownType, arg->location, "Type '%s' is not creatable", arg->value.data);
            }
            
            virtual bool visit(AstExprCall* node)
            {
                if (AstExprIndexName* index = node->func->as<AstExprIndexName>())
                {
                    AstExprConstantString* arg0 = node->args.size > 0 ? node->args.data[0]->as<AstExprConstantString>() : NULL;
                    
                    if (arg0)
                    {
                        if (node->self && index->index == "IsA" && node->args.size == 1)
                        {
                            validateType(arg0, false);
                        }
                        else if (node->self && (index->index == "GetService" || index->index == "FindService") && node->args.size == 1)
                        {
                            AstExprGlobal* g = index->expr->as<AstExprGlobal>();
                            
                            if (g && (g->name == "game" || g->name == "Game"))
                            {
                                validateType(arg0, false);
                            }
                        }
                        else if (!node->self && index->index == "new" && node->args.size <= 2)
                        {
                            AstExprGlobal* g = index->expr->as<AstExprGlobal>();
                            
                            if (g && g->name == "Instance")
                            {
                                validateType(arg0, true);
                            }
                        }
                    }
                }
                
                return true;
            }
        };
        
        class AnalyzerPassWarnLocalShadow: AstVisitor
        {
        public:
            static void process(AnalyzerContext& context)
            {
                AnalyzerPassWarnLocalShadow pass;
                pass.result = context.result;
                
                context.root->visit(&pass);
                
                pass.report();
            }
            
        private:
            ScriptAnalyzer::Result* result;
            
            std::set<AstLocal*> used;
            
            void report()
            {
                for (std::set<AstLocal*>::iterator it = used.begin(); it != used.end(); ++it)
                {
                    AstLocal* local = *it;
                    AstLocal* shadow = getUsedShadow(local);
                    
                    if (shadow)
                        emitWarning(*result, ScriptAnalyzer::Warning_LocalShadow, local->location, "Variable '%s' shadows the previous declaration at line %d", local->name.value, shadow->location.begin.line + 1);
                }
            }
        
            AstLocal* getUsedShadow(AstLocal* local)
            {
                while (local->shadow)
                {
                    if (used.count(local->shadow))
                        return local->shadow;
                    
                    local = local->shadow;
                }
                
                return NULL;
            }
            
            virtual bool visit(AstExprLocal* node)
            {
                used.insert(node->local);
                
                return true;
            }
        };
        
        class AnalyzerPassDataflow: AstVisitor
        {
        public:
            static void process(AnalyzerContext& context)
            {
                AnalyzerPassDataflow pass;
                pass.context = &context;
                
                for (DenseHashMap<AstName, Reflection::Variant>::const_iterator it = context.builtinGlobals.begin(); it != context.builtinGlobals.end(); ++it)
                {
                    const Reflection::Variant& v = it.getItem().value;
                    
                    shared_ptr<Instance> value;
                    if (v.isType<shared_ptr<Instance> >())
                        value = v.cast<shared_ptr<Instance> >();
                    
                    if (value)
                        pass.globals[it->value] = Value(value);
                    else
                        pass.globals[it->value] = Value(Value::Bottom);
                }
                
                context.root->visit(&pass);
                
                for (size_t i = 0; i < pass.tables.size(); ++i)
                {
                    Table& t = pass.tables[i];
                    
                    if (!t.bottom)
                    {
                        for (std::set<AstExprIndexName*>::iterator it = t.reads.begin(); it != t.reads.end(); ++it)
                        {
                            if (t.keys.count((*it)->index.value) == 0)
                            {
                                emitWarning(*context.result, ScriptAnalyzer::Warning_UnknownMember, (*it)->indexLocation, "Can not find member '%s' in table", (*it)->index.value);
                                            
                            }
                        }
                    }
                }
            }
            
        private:
            AnalyzerContext* context;
            
            struct Value
            {
                enum Type
                {
                    Null,
                    Object,
                    Class,
                    Table,
                    Bottom
                };
                
                Type type;
                
                shared_ptr<Instance> instance;
                const Reflection::ClassDescriptor* klass;
                unsigned int table;
                
                Value()
                    : type(Null)
                    , klass(NULL)
                {
                }
                
                explicit Value(Type type)
                    : type(type)
                    , klass(NULL)
                {
                }
                
                explicit Value(const shared_ptr<Instance>& instance)
                    : type(Object)
                    , instance(instance)
                    , klass(&instance->getDescriptor())
                {
                }
                
                explicit Value(const Reflection::ClassDescriptor* klass)
                    : type(Class)
                    , klass(klass)
                {
                }
                
                explicit Value(unsigned int table)
                    : type(Table)
                    , table(table)
                {
                }
            };
            
            struct Table
            {
                std::set<std::string> keys;
                std::set<AstExprIndexName*> reads;
                bool bottom;
                
                Table()
                    : bottom(false)
                {
                }
            };
            
            std::map<std::string, Value> globals;
            std::map<AstLocal*, Value> locals;
            std::map<AstExpr*, Value> evals;
            std::vector<Table> tables;
            
            struct ClassDescriptorNamePredicate
            {
                bool operator()(const Name& lhs, const Reflection::ClassDescriptor* rhs) const
                {
                    return lhs < rhs->name;
                }
                
                bool operator()(const Reflection::ClassDescriptor* lhs, const Name& rhs) const
                {
                    return lhs->name < rhs;
                }
            };
            
            const Reflection::ClassDescriptor* getClassDescriptor(AstExprConstantString* arg)
            {
                const Name& name = Name::lookup(std::string(arg->value.data, arg->value.size));
 
                if (name.empty())
                    return NULL;
                
                Reflection::ClassDescriptor::ClassDescriptors::const_iterator it =
                    std::lower_bound(Reflection::ClassDescriptor::all_begin(), Reflection::ClassDescriptor::all_end(), name, ClassDescriptorNamePredicate());
                
                if (it == Reflection::ClassDescriptor::all_end() || (*it)->name != name)
                    return NULL;
                
                return *it;
            }
            
            Value eval(AstExpr* node)
            {
                std::pair<std::map<AstExpr*, Value>::iterator, bool> p = evals.insert(std::make_pair(node, Value()));
                
                if (p.second)
                    p.first->second = evalExpr(node);
                
                return p.first->second;
            }
            
            Value evalExpr(AstExpr* node)
            {
                if (AstExprGroup* e = node->as<AstExprGroup>())
                    return eval(e->expr);
            
                if (node->is<AstExprConstantNil>())
                    return Value();
                
                if (AstExprLocal* e = node->as<AstExprLocal>())
                    return evalElement(locals, e->local);
                
                if (AstExprGlobal* e = node->as<AstExprGlobal>())
                    return evalElement(globals, std::string(e->name.value));
                
                if (AstExprIndexName* e = node->as<AstExprIndexName>())
                {
                    Value v = eval(e->expr);
                    
                    if (v.type == Value::Object)
                        return evalLookup(v.instance.get(), e->index.value, e->indexLocation);
                    else if (v.type == Value::Class)
                        return evalLookup(v.klass, e->index.value, e->indexLocation);
                    else if (v.type == Value::Table)
                    {
                        tables[v.table].reads.insert(e);
                        
                        return Value(Value::Bottom);
                    }
                    else
                        return Value(Value::Bottom);
                }
                
                if (AstExprIndexExpr* e = node->as<AstExprIndexExpr>())
                {
                    Value v = eval(e->expr);
                    
                    if (v.type == Value::Table)
                        tables[v.table].bottom = true;
                        
                    return Value(Value::Bottom);
                }
                
                if (AstExprCall* e = node->as<AstExprCall>())
                {
                    if (AstExprIndexName* index = e->func->as<AstExprIndexName>())
                    {
                        AstExprConstantString* arg = e->args.size >= 1 ? e->args.data[0]->as<AstExprConstantString>() : NULL;
                        
                        if (arg && !e->self && index->index == "new")
                        {
                            AstExprGlobal* g = index->expr->as<AstExprGlobal>();
                            
                            if (g && g->name == "Instance")
                            {
                                const Reflection::ClassDescriptor* klass = getClassDescriptor(arg);
                                
                                if (klass)
                                    return Value(klass);
                                else
                                    return Value(Value::Bottom);
                            }
                        }
                    }
     
                    return Value(Value::Bottom);
                }
                
                if (AstExprTable* e = node->as<AstExprTable>())
                {
                    unsigned int table = tables.size();
                    tables.push_back(Table());
                    
                    for (size_t i = 0; i < e->pairs.size; i += 2)
                    {
                        AstExpr* key = e->pairs.data[i];
                        AstExprConstantString* kv = key ? key->as<AstExprConstantString>() : NULL;
                        
                        if (kv)
                            tables[table].keys.insert(std::string(kv->value.data, kv->value.size));
                    }
                    
                    return Value(table);
                }
                
                return Value(Value::Bottom);
            }
            
            template <typename Key> Value evalElement(const std::map<Key, Value>& map, const Key& key)
            {
                typename std::map<Key, Value>::const_iterator it = map.find(key);
                
                return it == map.end() ? Value(Value::Bottom) : it->second;
            }
            
            Value evalLookup(const Reflection::ClassDescriptor* klass, const char* name, const Location& location)
            {
                if (klass->findPropertyDescriptor(name))
                    return Value(Value::Bottom);
                
                if (klass->findYieldFunctionDescriptor(name))
                    return Value(Value::Bottom);
                
                if (klass->findFunctionDescriptor(name))
                    return Value(Value::Bottom);
                
                if (klass->findEventDescriptor(name))
                    return Value(Value::Bottom);
                
                if (klass->findCallbackDescriptor(name))
                    return Value(Value::Bottom);
                
                emitWarning(*context->result, ScriptAnalyzer::Warning_UnknownMember, location, "Can not find member '%s' in type '%s'", name, klass->name.c_str());
                
                return Value(Value::Bottom);
            }
 
            Value evalLookup(Instance* object, const char* name, const Location& location)
            {
                if (Reflection::PropertyDescriptor* prop = object->findPropertyDescriptor(name))
                {
                    try
                    {
                        Reflection::Variant value;
                        prop->getVariant(object, value);
                        
                        shared_ptr<Instance> instance;
                        if (value.isType<shared_ptr<Instance> >())
                            instance = value.cast<shared_ptr<Instance> >();
                       	else if (value.isType<shared_ptr<Reflection::DescribedBase> >())
                            instance = shared_dynamic_cast<Instance>(value.cast<shared_ptr<Reflection::DescribedBase> >());
                        
                        return instance ? Value(instance) : Value(Value::Bottom);
                    }
                    catch (std::exception& e)
                    {
                        emitWarning(*context->result, ScriptAnalyzer::Warning_UnknownMember, location, "Can not get member '%s': %s", name, e.what());
                        
                        return Value(Value::Bottom);
                    }
                }
                
                if (object->findYieldFunctionDescriptor(name))
                    return Value(Value::Bottom);
                
                if (object->findFunctionDescriptor(name))
                    return Value(Value::Bottom);
                
                if (object->findSignalDescriptor(name))
                    return Value(Value::Bottom);
                
                if (object->findCallbackDescriptor(name))
                    return Value(Value::Bottom);
                
                if (Instance* child = object->findFirstChildByName(name))
                    return Value(shared_from(child));
                
                emitWarning(*context->result, ScriptAnalyzer::Warning_UnknownMember, location, "Can not find member '%s'", name);
                
                return Value(Value::Bottom);
            }
            
            virtual bool visit(AstExpr* node)
            {
                eval(node);
                
                return true;
            }
            
            virtual bool visit(AstStatLocal* node)
            {
                if (node->vars.size == 1 && node->values.size == 1)
                {
                    locals[node->vars.data[0]] = eval(node->values.data[0]);
                    
                    node->values.data[0]->visit(this);
                    
                    return false;
                }
                
                return true;
            }
            
            virtual bool visit(AstStatAssign* node)
            {
                if (node->vars.size == 1 && node->values.size == 1)
                {
                    AstExpr* e = node->vars.data[0];
                    
                    if (AstExprLocal* l = e->as<AstExprLocal>())
                    {
                        if (locals.count(l->local) == 0)
                            locals[l->local] = eval(node->values.data[0]);
                        else
                            locals[l->local] = Value(Value::Bottom);
                    }
                }
                
                for (size_t i = 0; i < node->vars.size; ++i)
                {
                    AstExpr* e = node->vars.data[i];

                    if (AstExprIndexName* index = e->as<AstExprIndexName>())
                    {
                        Value v = eval(index->expr);
                        
                        if (v.type == Value::Table)
                        {
                            tables[v.table].keys.insert(index->index.value);
                            
                            if (node->vars.size == 1 && node->values.size == 1)
                            {
                                AstExprFunction* f = node->values.data[0]->as<AstExprFunction>();
                                
                                if (f && f->self)
                                {
                                    locals[f->self] = v;
                                }
                            }
                        }
                    }
                }
                
                return true;
            }
        };
        
        struct WarningComparator
        {
            int compare(const ScriptAnalyzer::Position& lhs, const ScriptAnalyzer::Position& rhs) const
            {
                if (lhs.line != rhs.line)
                    return lhs.line < rhs.line ? -1 : 1;
                if (lhs.column != rhs.column)
                    return lhs.column < rhs.column ? -1 : 1;
                return 0;
            }
            
            int compare(const ScriptAnalyzer::Location& lhs, const ScriptAnalyzer::Location& rhs) const
            {
                if (int c = compare(lhs.begin, rhs.begin))
                    return c;
                if (int c = compare(lhs.end, rhs.end))
                    return c;
                return 0;
            }
            
            bool operator()(const ScriptAnalyzer::Warning& lhs, const ScriptAnalyzer::Warning& rhs) const
            {
                if (int c = compare(lhs.location, rhs.location))
                    return c < 0;
                
                return lhs.code < rhs.code;
            }
        };
    }
    
    static Lua::ThreadRef createScriptThread(ScriptContext* sc, shared_ptr<Instance> script)
    {
        Security::Identities identity = RBX::Security::GameScript_;
            
        lua_State* globalState = sc->getGlobalState(identity);
        RBXASSERT_BALLANCED_LUA_STACK(globalState);

        Lua::ThreadRef thread = lua_newthread(globalState);

        // Pop the thread from the stack when we leave
        Lua::ScopedPopper popper(globalState, 1);

        {
            RBXASSERT_BALLANCED_LUA_STACK(thread);
            RBXASSERT(lua_gettop(thread)==0);

            // declare "script" global
            Lua::ObjectBridge::push(thread, script);
            lua_setglobal( thread, "script" ); 

            // balance the thread's stack
            lua_settop(thread, 0);
        }

        return thread;
    }
    
    static void logTiming(const shared_ptr<Instance>& script, const Timer<Time::Precise>& timer, const char* name)
    {
        if (FFlag::DebugScriptAnalyzer)
        {
            double time = timer.delta().msec();
            
            if (time > 0.05)
                StandardOut::singleton()->printf(MESSAGE_OUTPUT, "%s: %s took %.1f msec", script->getFullName().c_str(), name, time);
        }
    }

    ScriptAnalyzer::Result ScriptAnalyzer::analyze(DataModel* dm, shared_ptr<Instance> script, const std::string& code)
    {
        using namespace ScriptParser;
        
        try
        {
            if (!dm) return ScriptAnalyzer::Result();

            RBXASSERT(dm->currentThreadHasWriteLock());

            ScriptContext* sc = ServiceProvider::create<ScriptContext>(dm);

            Lua::ThreadRef L = createScriptThread(sc, script);
            
            Result result;
            
            try
            {
                Timer<Time::Precise> parseTimer;
                
                ScriptParser::Allocator a;
                AstNameTable names(a);
                AstStat* root = Parser::parse(code.c_str(), code.size(), names, a);
                
                logTiming(script, parseTimer, format("Parsing %d Kb", static_cast<int>(code.size() / 1024)).c_str());
                
                AnalyzerContext context(&result, root);
                
				context.fillNames(names);
                context.fillBuiltinGlobals(names, L);
                context.fillDeprecatedGlobals(names);

                std::vector<AnalyzerPass> passes;
                
            #define PASS(name) do { AnalyzerPass p = { #name, name::process }; passes.push_back(p); } while (0)
                
				PASS(AnalyzerPassWarnGlobalLocal);

                PASS(AnalyzerPassWarnMultiLineStatement);
                PASS(AnalyzerPassWarnUnknownType);

				if (FFlag::StudioVariableIntellesense)
					PASS(AnalyzerPassIntellesenseLocal);

                // PASS(AnalyzerPassWarnLocalShadow);
                // PASS(AnalyzerPassWarnDotCall);
                // PASS(AnalyzerPassDataflow);
                
            #undef PASS
                
                for (size_t i = 0; i < (FFlag::StudioVariableIntellesense ? passes.size() : sizeof(passes) / sizeof(passes[0])); ++i)
                {
                    Timer<Time::Precise> passTimer;
                    
                    passes[i].process(context);
                    
                    logTiming(script, passTimer, passes[i].name);
                }
            }
            catch (const ScriptParser::Error& e)
            {
                Error error;
				error.location = e.getLocation();
                error.text = e.what();

                result.error = error;
            }
            
            std::sort(result.warnings.begin(), result.warnings.end(), WarningComparator());
            
            return result;
        }
        catch (RBX::base_exception& e)
        {
            StandardOut::singleton()->printf(MESSAGE_ERROR, "ScriptAnalyzer: unexpected error %s while parsing %s", e.what(), script ? script->getFullName().c_str() : "unknown script");

            return Result();
        }
    }
}
