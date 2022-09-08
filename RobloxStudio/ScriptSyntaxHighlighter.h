/**
 * ScriptSyntaxHighlighter.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

//this will make sure that we can use qt with boost (else moc compiler errors)
#ifndef QT_NO_KEYWORDS
    #define QT_NO_KEYWORDS
#endif

// Qt Headers
#include <QSyntaxHighlighter>

// Roblox Headers
#include "reflection/Property.h"

class QTextDocument;
class QStringList;
class QTextCharFormat;

enum RBXLuaLexState
{
    RBX_LUA_DEFAULT,
    RBX_LUA_COMMENT,
    RBX_LUA_COMMENTLINE,
    RBX_LUA_NUMBER,
    RBX_LUA_WORD,
    RBX_LUA_STRING,
    RBX_LUA_CHARACTER,
    RBX_LUA_LITERALSTRING,
    RBX_LUA_PREPROCESSOR,
    RBX_LUA_OPERATOR,
    RBX_LUA_IDENTIFIER,
    RBX_LUA_STRINGEOL
};

class ScriptSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:

    ScriptSyntaxHighlighter(QTextDocument* parent = 0);

    void onPropertyChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor);
    void setFont(const QFont& font);

protected:

    void highlightBlock(const QString& text);

private:

    void initData();

    void setLexState(
        RBXLuaLexState lexState,
        int            currentPos);

    bool checkApplyFoldState(const QString& keyword);

    QStringList     m_keywordPatterns;
    QStringList     m_foldStarts;
    QStringList     m_foldEnds;

    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_operatorFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_preprocessorFormat;
    QTextCharFormat m_defaultFormat;

    int             m_startSegPos;
    int             m_nestLevel;
    int             m_sepCount;

    RBXLuaLexState m_lexState;
};
