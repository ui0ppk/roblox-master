/**
 * ScriptSyntaxHighlighter.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "ScriptSyntaxHighlighter.h"

// Roblox Studio Headers
#include "RobloxScriptDoc.h"
#include "AuthoringSettings.h"
#include "QtUtilities.h"

static bool IsAWordChar(int ch) {
	return ch >= 0x80 ||
	       (isalnum(ch) || ch == '.' || ch == '_');
}

static bool IsADigit(unsigned int ch) {
	return (ch >= '0') && (ch <= '9');
}

static bool IsAWordStart(int ch) {
	return ch >= 0x80 ||
	       (isalpha(ch) || ch == '_');
}

static bool IsANumberChar(int ch) {
	// Not exactly following number definition (several dots are seen as OK, etc.)
	// but probably enough in most cases.
	return (ch < 0x80) &&
	        (isdigit(ch) || toupper(ch) == 'E' ||
	        ch == '.' || ch == '-' || ch == '+' ||
	        (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'));
}

static bool IsLuaOperator(int ch) {
	if (ch >= 0x80 || isalnum(ch)) {
		return false;
	}
	// '.' left out as it is used to make up numbers
	if (ch == '*' || ch == '/' || ch == '-' || ch == '+' ||
		ch == '(' || ch == ')' || ch == '=' ||
		ch == '{' || ch == '}' || ch == '~' ||
		ch == '[' || ch == ']' || ch == ';' ||
		ch == '<' || ch == '>' || ch == ',' ||
		ch == '.' || ch == '^' || ch == '%' || ch == ':' ||
		ch == '#') {
		return true;
	}
	return false;
}

// Test for [=[ ... ]=] delimiters, returns 0 if it's only a [ or ],
// return 1 for [[ or ]], returns >=2 for [=[ or ]=] and so on.
// The maximum number of '=' characters allowed is 254.
static int LongDelimCheck(const QString &text, int currentPos) {
	int sep = 1;
	while (currentPos+sep < text.size() && text[currentPos+sep] == QChar('=') && sep < 0xFF)
		sep++;
	if (currentPos+sep < text.size() && text[currentPos+sep] == text[currentPos])
		return sep;
	return 0;
}

ScriptSyntaxHighlighter::ScriptSyntaxHighlighter(QTextDocument *parent)
: QSyntaxHighlighter(parent)
, m_startSegPos(0)
, m_nestLevel(0)
, m_sepCount(0)
, m_lexState(RBX_LUA_DEFAULT)
{
	initData();
}

void ScriptSyntaxHighlighter::initData()
{
	m_keywordPatterns  << "and" << "break" << "do" << "else"
		<< "elseif" << "end" << "false" << "for"
		<< "function" << "if" << "in" << "local" 
		<< "nil" << "not" << "or" << "repeat" 
		<< "return" << "then" << "true" << "until" << "while"
		<< "_ALERT" << "_ERRORMESSAGE" << "_INPUT" << "_PROMPT" 
		<< "_OUTPUT _STDERR" << "_STDIN" << "_STDOUT" << "call" 
		<< "dostring" << "foreach" << "foreachi" << "getn"
		<< "globals" << "newtype" << "rawget" << "rawset" 
		<< "require" << "sort" << "tinsert" << "tremove"
		<< "_G" << "getfenv" << "getmetatable" << "ipairs"
		<< "loadlib" << "next" << "pairs" << "pcall" << "rawegal" 
		<< "rawget" << "rawset" << "require" << "setfenv"
		<< "setmetatable" << "xpcall" << "string" << "table" 
		<< "math" << "coroutine" << "io" << "os" << "debug"
		<< "abs" << "acos" << "asin" << "atan" << "atan2" 
		<< "ceil" << "cos" << "deg" << "exp" << "floor"
		<< "format" << "frexp" << "gsub" << "ldexp" << "log" 
		<< "log10" << "max" << "min" << "mod" << "rad"
		<< "random" << "randomseed" << "sin" << "sqrt" << "strbyte" 
		<< "strchar" << "strfind" << "strlen" << "strlower" 
		<< "strrep" << "strsub" << "strupper" << "tan"
        << "openfile" << "closefile" << "readfrom" << "writeto" << "appendto"
		<< "remove" << "rename" << "flush" << "seek" << "tmpfile" 
		<< "tmpname" << "read" << "write" << "clock" << "date"
		<< "difftime" << "execute" << "exit" << "getenv" << "setlocale" << "time";
	
	m_foldStarts << "function" << "if" << "do" << "repeat" << "[[" << "[==[" << "{" << "(";
	m_foldEnds << "end" << "until" << "]]" << "]==]" << "}" << ")";
}

void ScriptSyntaxHighlighter::setLexState(RBXLuaLexState lexState, int currentPos)
{
	if(currentPos < m_startSegPos)
		return;

	QTextCharFormat format;
	switch(m_lexState)
	{
        case RBX_LUA_COMMENT:
        case RBX_LUA_COMMENTLINE:
            format = m_commentFormat;
            break;
        case RBX_LUA_NUMBER:
            format = m_numberFormat;
            break;
        case RBX_LUA_PREPROCESSOR:
            format = m_preprocessorFormat;
            break;
        case RBX_LUA_OPERATOR:
            format = m_operatorFormat;
            break;
        case RBX_LUA_STRING:
        case RBX_LUA_STRINGEOL:
        case RBX_LUA_CHARACTER:
        case RBX_LUA_LITERALSTRING:
            format = m_stringFormat;
            break;
        case RBX_LUA_WORD:
            format = m_keywordFormat;
            break;
        default:
            format = m_defaultFormat;
            break;
	}

	setFormat(m_startSegPos, currentPos-m_startSegPos, format);
	m_lexState = lexState;
	
	m_startSegPos = currentPos;
}

bool ScriptSyntaxHighlighter::checkApplyFoldState(const QString &keyword)
{
	if(keyword.isNull() || keyword.isEmpty())
		return false;

	int foldState = 0;
	if(m_foldStarts.contains(keyword))
		foldState++;
	else if(m_foldEnds.contains(keyword))
		foldState--;
		
	RBXTextUserData* pUserData = dynamic_cast<RBXTextUserData*>(currentBlockUserData());
	if(foldState != 0)
	{
		if(!pUserData)
			pUserData = new RBXTextUserData();

		foldState += pUserData->getFoldState();
		pUserData->setFoldState(foldState);
		setCurrentBlockUserData(pUserData);
	}

	return (foldState != 0);
}

void ScriptSyntaxHighlighter::highlightBlock(const QString &text)
{
	RBXTextUserData* pUserData = dynamic_cast<RBXTextUserData*>(currentBlockUserData());
	if(!pUserData)
	{
		pUserData = new RBXTextUserData();
		setCurrentBlockUserData(pUserData);
	}

	if(pUserData)
	{
		pUserData->setFoldState(0);
	}

	if(text.isNull() || text.isEmpty())
	{
		if(pUserData)
			pUserData->setFoldState(0);
	}

	if(currentBlock().blockNumber() == 0)
	{
		if(text.startsWith('#'))
		{
			setFormat(0, text.length(), m_commentFormat);
			return;
		}

		// Initialize lex state to default
		m_lexState = RBX_LUA_DEFAULT;
	}

	m_startSegPos = 0;
	bool hasFold = false;
	
	if(previousBlockState() > -1)
		m_lexState = RBXLuaLexState(previousBlockState());

	if (m_lexState == RBX_LUA_LITERALSTRING || m_lexState == RBX_LUA_COMMENT) 
	{
		QTextBlock previousBlock = currentBlock().previous();
		if(previousBlock.isValid())
		{
			RBXTextUserData* pPreviousUserData = dynamic_cast<RBXTextUserData*>(previousBlock.userData());
			if(pPreviousUserData)
			{
				m_nestLevel = pPreviousUserData->getLineState() >> 8;
				m_sepCount = pPreviousUserData->getLineState() & 0xFF;
			}
		}
	}

	for(int ii=0; ii < text.size(); ii++)
	{
		// Determine if the current state should terminate.
		if (m_lexState == RBX_LUA_OPERATOR) 
		{
			if(ii > 0)
			{
				if(text[ii-1] == QChar('{'))
					hasFold |= checkApplyFoldState("{");
				else if(text[ii-1] == QChar('}'))
					hasFold |= checkApplyFoldState("}");
				else if(text[ii-1] == QChar('('))
					hasFold |= checkApplyFoldState("(");
				else if(text[ii-1] == QChar(')'))
					hasFold |= checkApplyFoldState(")");
			}
			
			setLexState(RBX_LUA_DEFAULT, ii);			
		} 
		else if (m_lexState == RBX_LUA_NUMBER) 
		{
			// Stop the number definition on non-numerical non-dot non-eE non-sign non-hexdigit char
			if (!IsANumberChar(text[ii].toAscii())) 
			{
				setLexState(RBX_LUA_DEFAULT, ii);	
			} 
			else if (text[ii] == QChar('-') || text[ii] == QChar('+')) 
			{
				if (text[ii-1] != QChar('E') && text[ii-1] != QChar('e'))
					setLexState(RBX_LUA_DEFAULT, ii);	
            }
		} 
		else if (m_lexState == RBX_LUA_IDENTIFIER) 
		{
			if (!IsAWordChar(text[ii].toAscii()) || (text[ii] == QChar('.')))
			{
				QString keyword = text.mid(m_startSegPos, ii-m_startSegPos);
				while(keyword.startsWith(QChar(' ')) || keyword.startsWith(QChar('\t')))
				{
					keyword.remove(0, 1);
					m_startSegPos++;
				}
				if(m_keywordPatterns.contains(keyword))
				{
					m_lexState = RBX_LUA_WORD;
					hasFold |= checkApplyFoldState(keyword);
				}
				setLexState(RBX_LUA_DEFAULT, ii);				
			}
		} 
		else if (m_lexState == RBX_LUA_COMMENTLINE || m_lexState == RBX_LUA_PREPROCESSOR) 
		{
			if (ii == text.size()-1) 
				setLexState(RBX_LUA_DEFAULT, ++ii);
		} 
		else if (m_lexState == RBX_LUA_STRING) 
		{
			if (text[ii] == QChar('\\'))
			{
				if(ii+1 < text.size())
				{
					QChar next = text[ii+1];
					if (next == QChar('\"') || next == QChar('\'') || next == QChar('\\'))					
						ii++;
				}
			} 
			else if (text[ii] == QChar('\"'))
			{
				setLexState(RBX_LUA_DEFAULT, ++ii);
			} 
			else if (ii == text.size() -1) 
			{
				m_lexState = RBX_LUA_STRINGEOL;
				setLexState(RBX_LUA_DEFAULT, ++ii);				
			}
		} 
		else if (m_lexState == RBX_LUA_CHARACTER) 
		{
			if (text[ii] == QChar('\\')) 
			{
				if(ii+1 < text.size())
				{
					if (text[ii+1] == QChar('\"') || text[ii+1] == QChar('\'') || text[ii+1] == QChar('\\'))
					{
						++ii;
					}
				}
			} 
			else if (text[ii] == QChar('\'')) 
			{
				setLexState(RBX_LUA_DEFAULT, ++ii);				
			} 
			else if (ii == text.size() -1) 
			{
				m_lexState = RBX_LUA_STRINGEOL;
				setLexState(RBX_LUA_DEFAULT, ++ii);				
			}
		} 
		else if (m_lexState == RBX_LUA_LITERALSTRING || m_lexState == RBX_LUA_COMMENT) 
		{
			if (text[ii] == QChar('[')) 
			{
				int sep = LongDelimCheck(text, ii);
				if (sep == 1 && m_sepCount == 1) {    // [[-only allowed to nest
					m_nestLevel++;
					ii++;
                    hasFold |= checkApplyFoldState("[[");
				}
				else if(sep > 1)
				{
					hasFold |= checkApplyFoldState("[==[");
				}
			} 
			else if (text[ii] == QChar(']'))
			{
				int sep = LongDelimCheck(text, ii);
				if (sep == 1 && m_sepCount == 1) {    // un-nest with ]]-only
					m_nestLevel--;
					ii++;
                    hasFold |= checkApplyFoldState("]]");
					if ( m_nestLevel == 0 )
						setLexState(RBX_LUA_DEFAULT, ++ii);
				} 
				else if (sep > 1)
				{
					hasFold |= checkApplyFoldState("]==]");

					if(sep == m_sepCount) {   // ]=]-style delim
						ii += sep;
						setLexState(RBX_LUA_DEFAULT, ++ii);		
					}
				}
			}
		}

		// Determine if a new state should be entered.
		if (m_lexState == RBX_LUA_DEFAULT) 
		{
			if(ii >= text.size())
				break;

			if (IsADigit(text[ii].toAscii()) || (text[ii] == QChar('.') && (ii+1 < text.size()) && IsADigit(text[ii+1].toAscii()))) 
			{
				m_lexState = RBX_LUA_NUMBER;
				if (text[ii] == QChar('0') && (ii+1 < text.size()) && text[ii+1].toUpper() == QChar('X')) 
				{
					ii++;
				}
			} 
			else if (IsAWordStart(text[ii].toAscii())) 
			{
				m_lexState = RBX_LUA_IDENTIFIER;
			} 
			else if (text[ii] == QChar('\"')) 
			{
				m_lexState = RBX_LUA_STRING;
			} 
			else if (text[ii] == QChar('\'')) 
			{
				m_lexState = RBX_LUA_CHARACTER;
			} 
			else if (text[ii] == QChar('['))
			{
				m_sepCount = LongDelimCheck(text, ii);
				if (m_sepCount == 0) 
				{
					m_lexState = RBX_LUA_OPERATOR;
				} 
				else 
				{
					m_nestLevel = 1;
					hasFold |= checkApplyFoldState("[==[");
					setLexState(RBX_LUA_LITERALSTRING, ii);
					ii += m_sepCount;
				}
			} 
			else if (text[ii] == QChar('-') && (ii+1 < text.size()) && text[ii+1] == QChar('-'))
			{
				setLexState(RBX_LUA_COMMENTLINE, ii);
				QString tmpStr = text.mid(ii, 3);
				if (tmpStr == "--[") 
				{
					ii += 2;
					m_sepCount = LongDelimCheck(text, ii);
					if (m_sepCount > 0) 
					{
						m_nestLevel = 1;
						m_lexState = RBX_LUA_COMMENT;
						hasFold |= checkApplyFoldState("[[");
						ii += m_sepCount;
					}
				} 
				else 
				{
					ii++;
				}
			} 
			else if (ii == 0 && text[ii] == QChar('$')) 
			{
				m_lexState = RBX_LUA_PREPROCESSOR;	// Obsolete since Lua 4.0, but still in old code
			} 
			else if (IsLuaOperator(text[ii].toAscii())) 
			{
				m_lexState = RBX_LUA_OPERATOR;
			}
		}
	}

	// Final check for termination, if any
	int finalIndex = text.size();
	if (m_lexState == RBX_LUA_OPERATOR)
	{
		if(finalIndex > 0)
		{
			if(text[finalIndex-1] == QChar('{'))
				hasFold |= checkApplyFoldState("{");
			else if(text[finalIndex-1] == QChar('}'))
				hasFold |= checkApplyFoldState("}");
			else if(text[finalIndex-1] == QChar('('))
				hasFold |= checkApplyFoldState("(");
			else if(text[finalIndex-1] == QChar(')'))
				hasFold |= checkApplyFoldState(")");
		}
		setLexState(RBX_LUA_DEFAULT, finalIndex);
	}
	else if (m_lexState == RBX_LUA_NUMBER || m_lexState == RBX_LUA_COMMENTLINE || m_lexState == RBX_LUA_PREPROCESSOR)		
	{
		setLexState(RBX_LUA_DEFAULT, finalIndex);			
	}
	else if(m_lexState == RBX_LUA_STRING || m_lexState == RBX_LUA_CHARACTER)
	{
        if (finalIndex > 0)
        {
            if(text[finalIndex-1] == QChar('\\'))
                setLexState(m_lexState, finalIndex);
            else
                setLexState(RBX_LUA_DEFAULT, finalIndex);
        }
	}
	else if (m_lexState == RBX_LUA_IDENTIFIER) 
	{
		QString keyword = text.mid(m_startSegPos, finalIndex-m_startSegPos);
		while(keyword.startsWith(QChar(' ')) || keyword.startsWith(QChar('\t')))
		{
			keyword.remove(0, 1);
			m_startSegPos++;
		}				
		if(m_keywordPatterns.contains(keyword))
		{
			m_lexState = RBX_LUA_WORD;
			hasFold |= checkApplyFoldState(keyword);
		}
		setLexState(RBX_LUA_DEFAULT, finalIndex);		
	}
	else
	{
		setLexState(m_lexState, text.size());
	}

	if(pUserData)
	{
		if(!hasFold)
			pUserData->setFoldState(0);

		if (m_lexState == RBX_LUA_LITERALSTRING || m_lexState == RBX_LUA_COMMENT) 
		{
			pUserData->setLineState((m_nestLevel << 8) | m_sepCount);
		}
	}

	setCurrentBlockState(m_lexState);
	setCurrentBlockUserData(pUserData);

	if(currentBlock() == document()->lastBlock())
	{
		m_startSegPos = 0;
		m_nestLevel = 0;
		m_sepCount = 0;
		m_lexState = RBX_LUA_DEFAULT;
	}
}

void ScriptSyntaxHighlighter::onPropertyChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor)
{
    m_keywordFormat = QTextCharFormat();
    
    if ( !pDescriptor || pDescriptor->category == sScriptColorsCategoryName )
    {
        m_keywordFormat.setForeground(QtUtilities::toQColor(AuthoringSettings::singleton().editorKeywordColor));
        m_operatorFormat.setForeground(QtUtilities::toQColor(AuthoringSettings::singleton().editorOperatorColor));
        m_numberFormat.setForeground(QtUtilities::toQColor(AuthoringSettings::singleton().editorNumberColor));
        m_stringFormat.setForeground(QtUtilities::toQColor(AuthoringSettings::singleton().editorStringColor));
        m_commentFormat.setForeground(QtUtilities::toQColor(AuthoringSettings::singleton().editorCommentColor));
        m_preprocessorFormat.setForeground(QtUtilities::toQColor(AuthoringSettings::singleton().editorPreprocessorColor));
        m_defaultFormat.setForeground(QtUtilities::toQColor(AuthoringSettings::singleton().editorTextColor));
    }
}

void ScriptSyntaxHighlighter::setFont(const QFont& font)
{
    m_keywordFormat.setFont(font);
    m_operatorFormat.setFont(font);
    m_numberFormat.setFont(font);
    m_stringFormat.setFont(font);
    m_commentFormat.setFont(font);
    m_preprocessorFormat.setFont(font);
    m_defaultFormat.setFont(font);
                                
    m_keywordFormat.setFontWeight(QFont::Bold);
}
