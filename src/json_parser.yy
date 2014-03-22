/* This file is part of QJSon
  *
  * Copyright (C) 2008 Flavio Castelli <flavio.castelli@gmail.com>
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
  * Boston, MA 02110-1301, USA.
  */

%skeleton "lalr1.cc"
%defines
%define "parser_class_name" "json_parser"

%code requires{
  #include "parser_p.h"
  #include "json_scanner.h"
  #include "qjson_debug.h"

  #include <QtCore/QByteArray>
  #include <QtCore/QMap>
  #include <QtCore/QString>
  #include <QtCore/QVariant>

  #include <limits>

  class JSonScanner;

  namespace QJson {
    class Parser;
  }

  #define YYERROR_VERBOSE 1
  
  Q_DECLARE_METATYPE(QVector<QVariant>*)
  Q_DECLARE_METATYPE(QVariantMap*)
}

%parse-param { QJson::ParserPrivate* driver }
%lex-param { QJson::ParserPrivate* driver }

%locations

%debug
%error-verbose

%token END 0 "end of file"

%token CURLY_BRACKET_OPEN 1 "{"
%token CURLY_BRACKET_CLOSE 2 "}"
%token SQUARE_BRACKET_OPEN 3 "["
%token SQUARE_BRACKET_CLOSE 4 "]"
%token COLON 5 ":"
%token COMMA 6 ","

%token NUMBER 7 "number"
%token TRUE_VAL 8 "true"
%token FALSE_VAL 9 "false"
%token NULL_VAL 10 "null"
%token STRING 11 "string"

%token INVALID 12 "invalid"

// define the initial token
%start start

%%

// grammar rules

start: data {
              driver->m_result = $1;
              qjsonDebug() << "json_parser - parsing finished";
            };

data: value { $$ = $1; }
      | error
          {
            qCritical()<< "json_parser - syntax error found, "
                    << "forcing abort, Line" << @$.begin.line << "Column" << @$.begin.column;
            YYABORT;
          }
      | END;

object: CURLY_BRACKET_OPEN CURLY_BRACKET_CLOSE {
          $$ = QVariant(QVariantMap());
        }
     |  CURLY_BRACKET_OPEN members CURLY_BRACKET_CLOSE {
          QVariantMap* map = $2.value<QVariantMap*>();
          $$ = QVariant(*map);
          delete map;
     };

members: STRING COLON value {
          QVariantMap* pair = new QVariantMap();
          pair->insert($1.toString(), $3);
          $$.setValue<QVariantMap* >(pair);
        }
      |  members COMMA STRING COLON value {
            $$.value<QVariantMap*>()->insert($3.toString(), $5);
         };

array:  SQUARE_BRACKET_OPEN SQUARE_BRACKET_CLOSE {
          $$ = QVariant(QVariantList());
        }
    |   SQUARE_BRACKET_OPEN values SQUARE_BRACKET_CLOSE { 
          QVector<QVariant>* list = $2.value<QVector<QVariant>* >();
          $$ = QVariant(list->toList());
          delete list;
        };

values: value {
          QVector<QVariant>* list = new QVector<QVariant>(1);
          list->replace(0, $1);
          $$.setValue(list);
        }
     |  values COMMA value {
          $$.value<QVector<QVariant>* >()->append($3);
        };

value: STRING
    |  NUMBER
    |  TRUE_VAL
    |  FALSE_VAL
    |  NULL_VAL
    |  object
    |  array;

%%

int yy::yylex(YYSTYPE *yylval, yy::location *yylloc, QJson::ParserPrivate* driver)
{
  JSonScanner* scanner = driver->m_scanner;
  yylval->clear();
  int ret = scanner->yylex(yylval, yylloc);

  qjsonDebug() << "json_parser::yylex - calling scanner yylval==|"
           << yylval->toByteArray() << "|, ret==|" << QString::number(ret) << "|";
  
  return ret;
}

void yy::json_parser::error (const yy::location& yyloc, const std::string& error)
{
  /*qjsonDebug() << yyloc.begin.line;
  qjsonDebug() << yyloc.begin.column;
  qjsonDebug() << yyloc.end.line;
  qjsonDebug() << yyloc.end.column;*/
  qjsonDebug() << "json_parser::error [line" << yyloc.end.line << "] -" << error.c_str() ;
  driver->setError(QString::fromLatin1(error.c_str()), yyloc.end.line);
}
