/* This file is part of QJson
 *
 * Copyright (C) 2013 Silvio Moioli <silvio@moioli.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software Foundation.
 * 
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110yy::json_parser::token::INVALID301, USA.
 */

/* Flex output settings */
%option 8bit c++ full warn
%option noyywrap nounistd
%option noinput nounput noyy_push_state noyy_pop_state noyy_top_state noyy_scan_buffer noyy_scan_bytes noyy_scan_string noyyget_extra noyyset_extra noyyget_leng noyyget_text noyyget_lineno noyyset_lineno noyyget_in noyyset_in noyyget_out noyyset_out noyyget_lval noyyset_lval noyyget_lloc noyyset_lloc noyyget_debug noyyset_debug
%option yyclass="JSonScanner"
%option outfile="json_scanner.cc"

%{
  #include "json_scanner.h"
  #include "json_parser.hh"
  
  #define YY_USER_INIT if(m_allowSpecialNumbers) { \
    BEGIN(ALLOW_SPECIAL_NUMBERS); \
  }
%}

/* Exclusive subscanners for strings and escaped hex sequences */
%x QUOTMARK_OPEN HEX_OPEN

/* Extra-JSON rules active iff m_allowSpecialNumbers is true */
%s ALLOW_SPECIAL_NUMBERS

%%

 /* Whitespace */
[\v\f\t ]+    {
                m_yylloc->columns(yyleng);
              }

[\r\n]+       { 
                m_yylloc->lines(yyleng);
              }


 /* Special values */
true          { 
                m_yylloc->columns(yyleng);
                *m_yylval = QVariant(true);
                return yy::json_parser::token::TRUE_VAL;
              }
                
false         {
                m_yylloc->columns(yyleng);
                *m_yylval = QVariant(false);
                return yy::json_parser::token::FALSE_VAL;
              }

null          {
                m_yylloc->columns(yyleng);
                *m_yylval = QVariant();
                return yy::json_parser::token::NULL_VAL;
              }
 
 
 /* Numbers */
[0-9]         |
[1-9][0-9]+   {
                m_yylloc->columns(yyleng);
                *m_yylval = QVariant(strtoull(yytext, NULL, 10));
                if (errno == ERANGE) {
                    qCritical() << "Number is out of range: " << yytext;
                    return yy::json_parser::token::INVALID;
                }
                return yy::json_parser::token::NUMBER;
              }

-[0-9]        |
-[1-9][0-9]+  {
                m_yylloc->columns(yyleng);
                *m_yylval = QVariant(strtoll(yytext, NULL, 10));
                if (errno == ERANGE) {
                    qCritical() << "Number is out of range: " << yytext;
                    return yy::json_parser::token::INVALID;
                }
                return yy::json_parser::token::NUMBER;
              }

-?(([0-9])|([1-9][0-9]+))(\.[0-9]+)?([Ee][+\-]?[0-9]+)? {
                m_yylloc->columns(yyleng);
                *m_yylval = QVariant(strtod(yytext, NULL));
                if (errno == ERANGE) {
                    qCritical() << "Number is out of range: " << yytext;
                    return yy::json_parser::token::INVALID;
                }
                return yy::json_parser::token::NUMBER;
              }

 /* Strings */              
\"            {
                m_yylloc->columns(yyleng);
                BEGIN(QUOTMARK_OPEN);
              }
              
<QUOTMARK_OPEN>{
  \\\"          {
                  m_currentString.append(QLatin1String("\""));
                }
                
  \\\\          {
                  m_currentString.append(QLatin1String("\\"));
                }
                
  \\\/          {
                  m_currentString.append(QLatin1String("/"));
                }
                
  \\b           {
                   m_currentString.append(QLatin1String("\b"));
                }
                
  \\f           {
                  m_currentString.append(QLatin1String("\f"));
                }
                
  \\n           {
                  m_currentString.append(QLatin1String("\n"));
                }
                
  \\r           {
                  m_currentString.append(QLatin1String("\r"));
                }
                
  \\t           {
                  m_currentString.append(QLatin1String("\t"));
                }
                
  \\u           {
                  BEGIN(HEX_OPEN);
                }
                
  [^\"\\]+      {
                  m_currentString.append(QString::fromUtf8(yytext));
                }

  \\            {
                  // ignore
                }
                
  \"            {
                  m_yylloc->columns(yyleng);
                  *m_yylval = QVariant(m_currentString);
                  m_currentString.clear();
                  BEGIN(INITIAL);
                  return yy::json_parser::token::STRING;
                }
}

<HEX_OPEN>{
  [0-9A-Fa-f]{4} {
                    QString hexDigits = QString::fromUtf8(yytext, yyleng);
                    bool ok;
                    ushort hexDigit1 = hexDigits.left(2).toShort(&ok, 16);
                    ushort hexDigit2 = hexDigits.right(2).toShort(&ok, 16);    
                    m_currentString.append(QChar(hexDigit2, hexDigit1));
                    BEGIN(QUOTMARK_OPEN);
                 }
                 
  .|\n           {
                    qCritical() << "Invalid hex string";
                    m_yylloc->columns(yyleng);
                    *m_yylval = QVariant(QLatin1String(""));
                    BEGIN(QUOTMARK_OPEN);
                    return yy::json_parser::token::INVALID;
                 }
}



 /* "Compound type" related tokens */              
:             {
                m_yylloc->columns(yyleng);
                return yy::json_parser::token::COLON;
              }

,             {
                m_yylloc->columns(yyleng);
                return yy::json_parser::token::COMMA;
              }

\[            {
                m_yylloc->columns(yyleng);
                return yy::json_parser::token::SQUARE_BRACKET_OPEN;
              }

\]            {
                m_yylloc->columns(yyleng);
                return yy::json_parser::token::SQUARE_BRACKET_CLOSE;
              }

\{            {
                m_yylloc->columns(yyleng);
                return yy::json_parser::token::CURLY_BRACKET_OPEN;
              }

\}            {
                m_yylloc->columns(yyleng);
                return yy::json_parser::token::CURLY_BRACKET_CLOSE;
              }


 /* Extra-JSON numbers */
<ALLOW_SPECIAL_NUMBERS>{
  (?i:nan)      {
                  m_yylloc->columns(yyleng);
                  *m_yylval = QVariant(std::numeric_limits<double>::quiet_NaN());
                  return yy::json_parser::token::NUMBER;
                }

  [Ii]nfinity   {
                    m_yylloc->columns(yyleng);
                    *m_yylval = QVariant(std::numeric_limits<double>::infinity());
                    return yy::json_parser::token::NUMBER;
                }

  -[Ii]nfinity  {
                    m_yylloc->columns(yyleng);
                    *m_yylval = QVariant(-std::numeric_limits<double>::infinity());
                    return yy::json_parser::token::NUMBER;
                }
}

 /* If all else fails */
.             {
                m_yylloc->columns(yyleng);
                return yy::json_parser::token::INVALID;
              }

<<EOF>>       return yy::json_parser::token::END;
