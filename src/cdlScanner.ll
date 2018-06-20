
%{ /* -*- C++ -*- */
/*
 * cdlScanner.yy
 *
 * Copyright 2014-2018 D. Mitch Bailey  cvc at shuharisystem dot com
 *
 * This file is part of cvc.
 *
 * cvc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cvc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cvc.  If not, see <http://www.gnu.org/licenses/>.
 *
 * You can download cvc from https://github.com/d-m-bailey/cvc.git
*/
#include "Cvc.hh"

# include "CCdlParserDriver.hh"
#include "cdlParser.hh"

     
// Work around an incompatibility in flex (at least versions
// 2.5.31 through 2.5.33): it generates code that does
// not conform to C89.  See Debian bug 333231
// <http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=333231>.
# undef yywrap
# define yywrap() 1
     
// The location of the current token.
static yy::location scanner_location;

int cdlLineCount = 0;
int cdlWarningCount = 0;

typedef yy::CCdlParser::token token;

#define YY_USER_ACTION yylloc->columns(yyleng);

%}

%s READING

TEXT			[[:alnum:][:punct:]]

/* %option noyywrap batch debug noinput */
%option noyywrap batch noinput 
/* bison-bridge bison-locations */


%%

%{
	yylloc->step();
%}

<INITIAL>^\.[Ss][Uu][Bb][Cc][Kk][Tt]/[[:space:]]	{ 
		BEGIN(READING);
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found SUBCKT\n" << endl;
#endif
		return( token::SUBCKT ); 
	};

<INITIAL>^\.[Ee][Nn][Dd][Ss]/[[:space:]] { 
		BEGIN(READING);
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found ENDS\n" << endl;
#endif
		return( token::ENDS ); 
	};

<INITIAL>^[Xx]{TEXT}* { 
		BEGIN(READING);
		yylval->charPtr = cdlCircuitList.cdlText.SetTextAddress(yytext);
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found subcircuit " << *(yylval->charPtr) << "\n" << endl;
#endif
		return( token::SUBCIRCUIT ); 
	}; 

<INITIAL>^[Mm]{TEXT}* { 
		BEGIN(READING);
		yylval->charPtr = cdlCircuitList.cdlText.SetTextAddress(yytext);
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found mosfet " << *(yylval->charPtr) << "\n" << endl;
#endif
		return( token::MOSFET ); 
	}; 

<INITIAL>^[Dd]{TEXT}* { 
		BEGIN(READING);
		yylval->charPtr = cdlCircuitList.cdlText.SetTextAddress(yytext);
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found diode " << *(yylval->charPtr) << "\n" << endl;
#endif
		return( token::DIODE ); 
	}; 

<INITIAL>^[Cc]{TEXT}* { 
		BEGIN(READING);
		yylval->charPtr = cdlCircuitList.cdlText.SetTextAddress(yytext);
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found capacitor " << *(yylval->charPtr) << "\n" << endl;
#endif
		return( token::CAPACITOR ); 
	}; 

<INITIAL>^[Rr]{TEXT}* { 
		BEGIN(READING);
		yylval->charPtr = cdlCircuitList.cdlText.SetTextAddress(yytext);
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found resistor " << *(yylval->charPtr) << "\n" << endl;
#endif
		return( token::RESISTOR ); 
	}; 

<INITIAL>^[Qq]{TEXT}* { 
		BEGIN(READING);
		yylval->charPtr = cdlCircuitList.cdlText.SetTextAddress(yytext);
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found bipolar " << *(yylval->charPtr) << "\n" << endl;
#endif
		return( token::BIPOLAR ); 
	}; 

<INITIAL>^[Ll]{TEXT}*	{ 
		BEGIN(READING);
		yylval->charPtr = cdlCircuitList.cdlText.SetTextAddress(yytext);
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found inductor " << *(yylval->charPtr) << "\n" << endl;
#endif
		return( token::INDUCTOR ); 
	}; 
	
<INITIAL>^\+	{
		/* continuation */ 
//		cdlLineCount++;
		fprintf(stderr, "** ERROR: Unexpected continuation at line %d **\n", yylloc->begin.line);
		cdlCircuitList.errorCount++;
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found unexpected continuation\n" << endl;
#endif
	};

<*>^[[:blank:]]+/{TEXT}	{
		fprintf(stderr, "** ERROR: Unexpected leading blanks at line %d **\n", yylloc->begin.line);
		cdlCircuitList.errorCount++;
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found unexpected leading blank\n" << endl;
#endif
	};

<INITIAL>{TEXT}+	{ 
		fprintf(stderr, "** ERROR: Unexpected text %s at line %d column %d **\n", yytext, yylloc->begin.line, yylloc->begin.column);
		cdlCircuitList.errorCount++;
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found unexpected text " << yytext << "\n" << endl;
#endif
	};

<READING>{TEXT}+	{ 
		yylval->charPtr = cdlCircuitList.cdlText.SetTextAddress(yytext);
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found string " << *(yylval->charPtr) << "\n" << endl;
#endif
		return( token::STRING ); 
	};

<*>^\*.*	{ 
		/* comment */ 
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found comment\n" << endl;
#endif
	};

<*>ÅO[[:blank:]]*$	{ 
		/* blank line */ 
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found blank line\n" << endl;
#endif
	};

<INITIAL>\n	{
		cdlLineCount++;
		yylloc->lines(1); yylloc->step();
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found EOL outside subcircuit\n" << endl;
#endif
	};
	
<READING>\n\+	{
		/* continuation */ 
		cdlLineCount++;
		yylloc->lines(1); yylloc->step();
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found continuation\n" << endl;
#endif
	};

<READING>\n/\*	{
		/* skip comments */ 
		cdlLineCount++;
		yylloc->lines(1); yylloc->step();
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found comment in continuation\n" << endl;
#endif
	};

<READING>\n/[[:space:]]	{
		/* skip blank lines */ 
		cdlLineCount++;
		yylloc->lines(1); yylloc->step();
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found possible blank line in continuation\n" << endl;
#endif
	};

<READING>\n	{
		cdlLineCount++;
		yylloc->lines(1); yylloc->step();
		BEGIN(INITIAL);
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found EOL\n" << endl;
#endif
		return( token::EOL ); 
	};
	
<*><<EOF>>	{
#ifdef CDL_FLEX_DEBUG
		cout << "FLEX DEBUG: found EOF\n" << endl;
#endif
		if ( cdlCircuitList.errorCount > 0 ) {
			fprintf(stderr, "** ERROR: Unexpected character or syntax in CDL file %s**\n", driver.filename.c_str());
//			exit(1);
		}
		return( token::CDL_EOF );
	};
	
[[:space:]] {
		/* no token */
		yylloc->step();
	};	
	
. { 
		cdlCircuitList.errorCount++;
//		fprintf(stderr, "** ERROR: unexpected character '%s' at line %d\n", yytext, cdlLineCount); 
		fprintf(stderr, "** ERROR: unexpected character '%s' at line %d column %d\n", yytext, yylloc->begin.line, yylloc->begin.column); 
	};

%%
